#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <algorithm>
#include <new>
#include <ctime>
#include <cmath>
#include <esp_arduino_version.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include "CrowPanelBoard.h"
#include "CrowPanelLgfx.h"
#include "CrowPanelRender.h"
#include "CrowPanelTouch.h"
#include "AudioManager.h"
#include "StationManager.h"
#include "RetroRadio.h"
#include "MainScreen.h"
#include "Settings.h"
#include "CrowPanelAccess.h"
#include "SDPlayer.h"
#include "AppServices.h"
#include "CrowPanelConfig.h"
#include "Thema.h"
#include "StringUtils.h"
#include "Logger.h"
#include "WebPortal.h"
#include "FtpService.h"
#include <esp_ota_ops.h>
#include <driver/temperature_sensor.h>
// Die Firmware-Kennung kommt über das Build-Flag FW_VERSION (scripts/fw_version.py).
#ifndef FW_VERSION
#define FW_VERSION "1.0"
#endif
// Befehlseingang für die Entwicklungsskripte unter scripts/: Ansichten, Kontakte, Diagnose, WLAN-Profil und Screenshot, über UART0 (CH340 an der USB-C-Buchse). Der Standardbuild liest keine Befehle. Die Umgebung crowpanel7_dev aktiviert ihn in platformio.ini mit build_src_flags = -DCROWPANEL_USB_FERNSTEUERUNG=1.
#ifndef CROWPANEL_USB_FERNSTEUERUNG
#define CROWPANEL_USB_FERNSTEUERUNG 0
#endif
#if CROWPANEL_USB_FERNSTEUERUNG
#include "ScreenshotUsb.h"
#endif

namespace
{
    constexpr uint8_t DisplayRotation = 0; ///< Lage beim Start, das Panel liegt nativ quer. Die gespeicherte Ausrichtung stellt applyScreen danach ein
    constexpr uint8_t DisplayRotationFlipped = 2; ///< Um 180 Grad gedrehte Lage, CrowPanelRender überträgt in Rotation 0 und 2
    constexpr size_t PsramTestBytes = 1024 * 1024; ///< Größe des PSRAM-Prüfpuffers beim Start
    constexpr uint32_t AudioStartupTimeoutMs = 1000; ///< Wartezeit auf den Start des Audioworkers
    constexpr uint32_t AudioOutputSettlingMs = 100; ///< So lange muss frisches PCM anliegen, bevor der Ausgang freigegeben wird
    constexpr size_t InternalMallocLimit = 1024; ///< malloc/new bis zu dieser Größe zuerst intern, darüber zuerst im PSRAM (Arduino-Vorgabe 4096). Größere Anforderungen zersplittern sonst den knappen internen DMA-Speicher, den WLAN, I2S und Kartentreiber am Stück brauchen
    uint32_t s_stationsRevision = 1; ///< Zähler der Senderliste für die Weboberfläche
    /** @brief Aktive Ansicht. */
    enum class Screen
    {
        Home,
        Radio,
        Settings,
        Sd
    };
    /** @brief Quelle der laufenden Wiedergabe. */
    enum class Source
    {
        None,
        Radio,
        Sd
    };

    /** @brief Legt ein großes, langlebiges Objekt im PSRAM an, damit der knappe interne RAM des S3 für WLAN, TLS, I2S und Kartentreiber bleibt. PSRAM steht schon vor den globalen Konstruktoren bereit (CONFIG_SPIRAM_BOOT_INIT), ohne PSRAM wird das Objekt intern angelegt. Nur für Klassen ohne atomare Elemente und Sperren. */
    template <typename T>
    T &psramObject()
    {
        void *memory = heap_caps_malloc(sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        return *(memory ? new (memory) T() : new T());
    }

    StationManager &s_stations = psramObject<StationManager>();
    VisualizerView s_visualizer;
    RetroRadio &s_radio = psramObject<RetroRadio>();
    MainScreen s_home;
    Settings &s_settings = psramObject<Settings>();
    CrowPanelAccess s_access;
    CrowPanelLgfx s_display;       ///< RGB-Panel, Bildspeicher im PSRAM
    CrowPanelTouch s_touch;        ///< GT911 über Wire
    bool s_touchReady = false;     ///< GT911 hat begin() bestanden

    /** @brief Berührungszustand eines Schleifendurchlaufs, gebildet aus den Meldungen des GT911. */
    struct TouchState
    {
        int16_t x = -1, y = -1;     ///< Letzte Koordinate
        bool pressed = false;       ///< Finger liegt
        bool pressedNow = false;    ///< In diesem Durchlauf aufgesetzt
        bool releasedNow = false;   ///< In diesem Durchlauf losgelassen

        bool isPressed() const
        {
            return pressed;
        }

        bool wasPressed() const
        {
            return pressedNow;
        }

        bool wasReleased() const
        {
            return releasedNow;
        }
    };

    TouchState s_touchState;

    /** @brief Fragt den GT911 ab und leitet Aufsetzen und Loslassen ab. Ohne neue Meldung bleibt der Zustand. Nur im UI-Task. */
    const TouchState &updateTouch()
    {
        s_touchState.pressedNow = s_touchState.releasedNow = false;
        CrowPanelTouch::Point points[CrowPanelTouch::MaxPoints];
        const int count = s_touch.read(points, CrowPanelTouch::MaxPoints);
        // Ohne neue Meldung (-2) oder bei einem Lesefehler (-1) bleibt der bisherige Zustand.
        if (count < 0)
            return s_touchState;
        if (count == 0)
        {
            s_touchState.releasedNow = s_touchState.pressed;
            s_touchState.pressed = false;
            return s_touchState;
        }
        s_touchState.pressedNow = !s_touchState.pressed;
        s_touchState.pressed = true;
        // Der GT911 meldet Panelkoordinaten, um 180 Grad gedreht liegt das Bild gespiegelt darauf.
        const bool flipped = s_display.getRotation() == DisplayRotationFlipped;
        s_touchState.x = flipped ? CrowPanelRender::PanelWidth - 1 - points[0].x : points[0].x;
        s_touchState.y = flipped ? CrowPanelRender::PanelHeight - 1 - points[0].y : points[0].y;
        return s_touchState;
    }

    CrowPanelCornerGesture s_corner;
    CrowPanelTimeWindow s_timeWindow;
    Settings::Sound s_audioSettings;
    SdPlayer &s_sd = psramObject<SdPlayer>();
    AppServices s_services;
#if CROWPANEL_USB_FERNSTEUERUNG
    constexpr uint32_t SerialBaud = 921600; ///< UART0 über den CH340, schnell genug für die Bildschirmaufnahme
#else
    constexpr uint32_t SerialBaud = 115200; ///< UART0 über den CH340, wie das ROM-Log
#endif
#if CROWPANEL_USB_FERNSTEUERUNG
    ScreenshotUsb s_screenshot;
    bool s_captureRequested = false; ///< USB-Befehl c, die Aufnahme folgt nach dem nächsten Bild
    bool s_drawingPaused = false; ///< Diagnose per USB, hält Zeichnen an, Quelle und RGB-Ausgabe laufen weiter
    Stream *s_captureTarget = &Serial; ///< Kanal, über den der Befehl c kam, dorthin geht die Aufnahme
#endif
    WebPortal s_web;
    FtpService s_ftp;
    bool s_ftpReady = false; ///< FTP-Dienst angelegt, eingeschaltet wird er nur per Schalter
    bool s_webReady = false, s_webRestart = false, s_webModernRestart = false;
    uint32_t s_webSettingsId = 0, s_webModernId = 0; ///< s_webModernId: wartender Webauftrag für die Grundfarben von Modern, 0 ohne
    Settings::Request s_webTheme; ///< Speichern des Themas, das nach den Grundfarben desselben Webauftrags folgt. type None ohne
    uint32_t s_appAddress = 0, s_appCapacity = 0;
    Settings::Configuration s_config;
    MainScreen::Status s_homeStatus;
    Settings::Status s_settingsStatus;
    Screen s_screen = Screen::Home;
    Source s_source = Source::None;
    bool s_ready = false, s_servicesReady = false, s_codecReady = false, s_listening = false, s_userMuted = false; ///< Start abgeschlossen, Dienste laufen, Ausgabe vorbereitet, Ausgang freigegeben, vom Benutzer stumm
    bool s_audioAvailable = false; ///< Audioworker beim Start bereit, nur dann darf die Ausgabe nach einem Stopp erneut freigegeben werden
    CrowPanelBoard::HeadphoneDebounce s_headphone; ///< Zustandshelfer ohne physische Kopfhörerbuchse, bleibt am S3 auf „kein Stecker“, nur im UI-Task
    bool s_radioFailed = false, s_sdFailed = false, s_restart = false;
    bool s_touchSuppressed = false; ///< Kontakt, der wegen eines Web-Screenshots bis zum Loslassen übergangen wird
    bool s_homeNavigationContact = false; ///< Nach Navigation auf Press den restlichen Kontakt verbrauchen.
    uint8_t s_volume = AudioManager::DefaultVolume;
    int8_t s_tone[3]{};
    int8_t s_balance = 0;
    uint8_t s_beforeMute = 10;
    int s_selectedStation = 0;
    uint32_t s_audioSequence = 0, s_audioToken = 0, s_radioAck = 0, s_radioStream = 0; ///< Zähler der Quellkennungen, Kennung der aktuellen Wiedergabe sowie die zuletzt vom Radio bestätigte Anforderung und der zugehörige Stream
    uint32_t s_audioReadyAt = 0;
    uint32_t s_sdPlayback = 0, s_sdTrack = 0, s_sdMedia = 0, s_sdAck = 0, s_sdControl = 0, s_sdStop = 0, s_sdRevision = 0;
    uint32_t s_lastStatus = 0;
    uint32_t s_firmwareBytes = 0, s_flashBytes = 0, s_psramBytes = 0; ///< Unveränderliche Geräteangaben, einmalig vor der UI-Schleife ermitteln.
    Settings::BusScan s_i2cInternal = Settings::BusScan::NotChecked; ///< Ergebnis des einmaligen I2C-Startscans (Adressen 0x08..0x77)
    char s_i2cInternalList[64] = {};
    temperature_sensor_handle_t s_chipTemp = nullptr; ///< Interner Temperatursensor des S3, ab dem Start dauerhaft aktiv
    uint32_t s_restartAt = 0, s_wifiWaitingAt = 0;
    constexpr uint32_t ThemeRequestId = UINT32_MAX - 3; ///< Kennung des Speicherauftrags von \@thema, nach Erfolg folgt der Neustart
    constexpr uint32_t ModernRequestId = UINT32_MAX - 4; ///< Kennung des Auftrags von \@modernfarbe, nach Erfolg folgt der Neustart
    char s_sdPath[256]{};
    char s_title[256]{}, s_artist[128]{}, s_album[128]{};
    Settings::Request s_wifiWaiting;
    bool s_wifiPending = false;
    QueueHandle_t s_sdActions = nullptr;
    SdPlayerAction s_priorityAction;
    bool s_havePriority = false;

    /** @brief Liefert den Namen eines Resetgrunds für die Startzeile, nicht aufgeführte Werte heißen UNKNOWN. */
    const char *resetReasonName(esp_reset_reason_t reason)
    {
        switch (reason)
        {
        case ESP_RST_POWERON:
            return "POWERON";
        case ESP_RST_EXT:
            return "EXT";
        case ESP_RST_SW:
            return "SW";
        case ESP_RST_PANIC:
            return "PANIC";
        case ESP_RST_INT_WDT:
            return "INT_WDT";
        case ESP_RST_TASK_WDT:
            return "TASK_WDT";
        case ESP_RST_WDT:
            return "WDT";
        case ESP_RST_DEEPSLEEP:
            return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:
            return "BROWNOUT";
        case ESP_RST_SDIO:
            return "SDIO";
        case ESP_RST_USB:
            return "USB";
        case ESP_RST_JTAG:
            return "JTAG";
        case ESP_RST_EFUSE:
            return "EFUSE";
        case ESP_RST_PWR_GLITCH:
            return "PWR_GLITCH";
        case ESP_RST_CPU_LOCKUP:
            return "CPU_LOCKUP";
        default:
            return "UNKNOWN";
        }
    }

    /** @brief Schaltet die Ausgabe stumm, bevor die Quelle ihre Takte ändern kann. @return Hardware bestätigt. */
    bool muteAudio()
    {
        s_listening = false;
        s_audioReadyAt = 0;
        g_audioManager.setMuted(true);
        const bool muted = CrowPanelBoard::powerDownRadioCodec();
        // Ein bestätigter Ausgabestopp erlaubt den nächsten Quellenstart, sofern der Audioworker bereit ist.
        s_codecReady = s_audioAvailable && muted;
        if (!muted)
        {
            LOG_ERROR("Verstärker nicht abgeschaltet, Quelle gestoppt.");
            g_audioManager.requestStop();
        }
        return muted;
    }

    /** @brief Stummt den Ausgang, entwertet die Quellkennung vor dem asynchronen Stopp und löscht Titeltexte und Signalanzeige. */
    void stopAudio()
    {
        muteAudio();
        s_audioToken = 0;
        s_source = Source::None;
        s_radio.cancelPendingStart();
        g_audioManager.requestStop();
        s_title[0] = s_artist[0] = s_album[0] = 0;
        g_audioManager.consumeAudioData();
        s_visualizer.resetSignal();
    }

    /** @brief Erkennt das tatsächliche Ende eines angeforderten Audiostopps. */
    bool audioStopped()
    {
        const auto state = g_audioManager.getStatus().state;
        return state == AudioManager::State::Idle || state == AudioManager::State::Unavailable || state == AudioManager::State::Error || state == AudioManager::State::Ended;
    }

    /** @brief Vergibt quellenübergreifende Identitäten, damit verspätete Metadaten nicht zum nächsten Titel gehören. */
    uint32_t nextAudioToken()
    {
        if (++s_audioSequence == 0)
            ++s_audioSequence;
        return s_audioSequence;
    }

    /** @brief Öffnet den Ausgang für frisches PCM der aktuellen Quelle nach der I2S-Taktumstellung. Benutzer-Stumm bleibt erhalten. */
    void enableAudio()
    {
        if (s_listening)
            return;
        const auto status = g_audioManager.getStatus();
        if (!s_codecReady || s_userMuted || !s_audioSettings.speakers || !AudioManager::canOutput(status, s_audioToken))
        {
            s_audioReadyAt = 0;
            return;
        }
        const uint32_t now = millis();
        if (!s_audioReadyAt)
        {
            s_audioReadyAt = now;
            return;
        }
        const uint32_t settledMs = now - s_audioReadyAt;
        if (settledMs < AudioOutputSettlingMs)
            return;
        // Der PCM-Abgriff liegt vor dem ersten I2S-Schreiben. Erst Daten und Takte anlaufen lassen, danach die Nullsamplesperre der Ausgabe lösen.
        s_listening = CrowPanelBoard::prepareRadioCodec() && CrowPanelBoard::unmuteRadio(!s_headphone.inserted());
        g_audioManager.setMuted(!s_listening);
        // stopAudio setzt die Freigabebereitschaft für den nächsten Start zurück.
        if (!s_listening)
            stopAudio();
        LOG_INFO_F("AUDIO_AUSGANG Aktiv=%u Pegel=%u/100 PCM-Wartezeit=%lu ms", s_listening, s_volume, static_cast<unsigned long>(settledMs));
    }

    /** @brief Fragt den Boardzustand zyklisch ab. Da der S3 keine Kopfhörerbuchse hat, liefert die Boardebene immer „kein Stecker“ und die Ausgabe bleibt mono. @param now millis(). */
    void updateHeadphone(uint32_t now)
    {
        if (!s_headphone.due(now))
            return;
        bool inserted = false;
        const bool read = CrowPanelBoard::readHeadphone(inserted);
        if (!s_headphone.update(now, read, inserted))
            return;
        g_audioManager.setStereoOutput(CrowPanelBoard::StereoSpeakers || s_headphone.inserted());
        // Bei freigegebenem Ausgang den Lautsprecherzustand nachführen. Ein Fehler schaltet die Ausgabe stumm.
        if (s_listening && !CrowPanelBoard::setSpeaker(!s_headphone.inserted()))
            muteAudio();
        LOG_INFO_F("KOPFHOERER Stecker=%u Ausgabe=%u", s_headphone.inserted(), s_listening);
    }

    /** @brief Übernimmt den Stummwunsch unabhängig von vorübergehenden Quellwechseln und Pausen. */
    bool setUserMuted(bool muted)
    {
        s_userMuted = muted;
        if (muted)
            return muteAudio();
        enableAudio();
        return s_codecReady;
    }

    /** @brief Übernimmt einen Pegel aus jedem Bedienweg, jeder Pegel über 0 wird zugleich Rückkehrpegel für das Entstummen aus 0. @param volume Pegel 0..MaxVolume. */
    void setVolume(uint8_t volume)
    {
        s_volume = volume;
        s_beforeMute = RadioPreferences::returnVolume(volume, s_beforeMute);
        g_audioManager.setVolume(s_volume);
        g_audioManager.setLoudnessVolume(s_volume);
    }

    /** @brief Merkt den zentralen Radiozustand aus allen Bedienwegen ohne I/O, der Dienst fasst tatsächliche Änderungen zusammen. */
    void rememberRadioState()
    {
        if (!s_servicesReady)
            return;
        RadioPreferences value;
        s_visualizer.preferences(value);
        value.station = s_selectedStation;
        value.volume = s_volume;
        value.balance = s_balance;
        value.muted = s_userMuted;
        value.beforeMute = s_beforeMute;
        memcpy(value.tone, s_tone, 3);
        s_services.saveRadioPreferences(value);
    }

    /** @brief Übernimmt den Klangentwurf ohne NVS-Zugriff und erhält die zentrale Stummschaltung. */
    bool applySound(const Settings::Sound &sound)
    {
        if (!g_audioManager.setTuning({sound.eqLow, sound.eqPeak, sound.eqHigh, sound.width, sound.loudness}))
            return false;
        s_audioSettings = sound;
        if (!sound.speakers)
            muteAudio();
        else
            enableAudio();
        return true;
    }

    bool applyScreen(uint8_t brightness, uint8_t rotation, bool autoRotate);

    /** @brief Aktiviert genau einen Bildschirm, Quellen werden beim Verlassen gestoppt. */
    void showScreen(Screen screen)
    {
        stopAudio();
        // Eine ungespeicherte Vorschau der Bildschirmseite und eine nur getestete WLAN-Verbindung enden mit dem Verlassen der Einstellungen.
        if (s_screen == Screen::Settings && screen != Screen::Settings)
        {
            applyScreen(s_config.brightness, s_config.rotation, s_config.autoRotate);
            s_services.restoreWifi();
        }
        s_home.hide();
        s_settings.hide();
        s_sd.hide();
        s_radio.hide();
        s_corner.reset();
        s_screen = screen;
        if (screen == Screen::Home)
            s_home.show();
        else if (screen == Screen::Radio)
            s_radio.show();
        else if (screen == Screen::Settings)
            s_settings.show(s_config.language);
        else
            s_sd.show(s_config.language);
        LOG_INFO_F("ANSICHT=%u", static_cast<unsigned>(screen));
    }

    /** @brief Rechnet die Helligkeit der Bildschirmseite (Prozent) in den Wert von setBrightness (0..255) um. */
    uint8_t backlight(uint8_t percent)
    {
        return static_cast<uint8_t>((std::min<unsigned>(percent, 100u) * 255u + 50u) / 100u);
    }

    void invalidateScreen();

    /** @brief Dreht das Bild und zeichnet Sperre oder aktive Ansicht vollständig neu. @param rotation DisplayRotation oder DisplayRotationFlipped. @param now Millisekunden. */
    void setDisplayRotation(uint8_t rotation, uint32_t now)
    {
        if (s_display.getRotation() == rotation)
            return;
        s_display.setRotation(rotation);
        if (s_access.active())
            s_access.activate(now);
        else
            invalidateScreen();
        LOG_INFO_F("DISPLAY Rotation=%u", rotation);
    }

    /** @brief Übernimmt Helligkeit und Ausrichtung der Bildschirmseite. Einen Lagesensor gibt es am CrowPanel nicht, es gilt immer die feste Ausrichtung. @param brightness Prozent 10..100. @param rotation Gespeicherter Wert Settings::RotationNormal (Rotation 0) oder Settings::RotationFlipped (Rotation 2). @param autoRotate ohne Wirkung, bleibt für das gespeicherte Format. @return false bei ungültigen Werten. */
    bool applyScreen(uint8_t brightness, uint8_t rotation, bool autoRotate)
    {
        (void)autoRotate;
        if (brightness < Settings::BrightnessMin || brightness > Settings::BrightnessMax || (rotation != Settings::RotationNormal && rotation != Settings::RotationFlipped))
            return false;
        if (!s_timeWindow.sleeping())
            s_display.setBrightness(backlight(brightness));
        setDisplayRotation(rotation == Settings::RotationFlipped ? DisplayRotationFlipped : DisplayRotation, millis());
        return true;
    }

    /** @brief Setzt den Schlaf-/Aufweckübergang zentral für Display, Audio und PIN um. Ohne PIN-Sperre öffnet das Aufwecken direkt den MainScreen. */
    void applyWindow(CrowPanelTimeWindow::Event event, uint32_t now, bool contactHeld = false)
    {
        if (event == CrowPanelTimeWindow::Event::None)
            return;
        showScreen(Screen::Home);
        if (s_config.pinLock)
            s_access.activate(now, contactHeld);
        else if (contactHeld)
            s_homeNavigationContact = true; // Der Aufweckkontakt darf keine Kachel öffnen.
        s_display.setBrightness(event == CrowPanelTimeWindow::Event::Sleep ? 0 : backlight(s_config.brightness));
        if (event == CrowPanelTimeWindow::Event::Sleep)
            LOG_INFO("ZEITFENSTER Schlaf");
        else
            LOG_INFO(s_config.pinLock ? "ZEITFENSTER Wach, PIN erforderlich" : "ZEITFENSTER Wach");
    }

    /** @brief Stellt nach einem Modal nur den darunterliegenden Bildschirm wieder her. */
    void invalidateScreen()
    {
        if (s_screen == Screen::Home)
            s_home.invalidate();
        else if (s_screen == Screen::Radio)
            s_radio.invalidate();
        else if (s_screen == Screen::Settings)
            s_settings.invalidate();
        else
            s_sd.invalidate();
    }

    /** @brief Bestätigt Auswahl sofort, der Radiozeiger gibt den späteren Start frei. */
    void radioAction(const RetroRadio::Input &input)
    {
        using A = RetroRadio::Action;
        if (input.action == A::Select)
        {
            Station station;
            s_radioAck = input.requestId;
            s_radioFailed = input.stationsRevision != s_stationsRevision || !s_stations.getStation(input.station, station) || !station.url[0];
            // cancelPendingStart würde die gerade vom Radio angelegte Auswahl entwerten.
            muteAudio();
            g_audioManager.requestStop();
            s_source = Source::None;
            s_audioToken = 0;
            s_title[0] = 0;
            g_audioManager.consumeAudioData();
            s_visualizer.resetSignal();
            if (!s_radioFailed)
                s_selectedStation = input.station;
        }
        else if (input.action == A::Start)
        {
            Station station;
            s_radioStream = input.requestId;
            muteAudio();
            const uint32_t token = nextAudioToken();
            s_radioFailed = !s_codecReady || s_services.wifiBusy() || s_wifiPending || WiFi.status() != WL_CONNECTED || input.station != s_selectedStation || !s_stations.getStation(input.station, station) || !g_audioManager.requestStart(station.url, input.station, token);
            if (!s_radioFailed)
            {
                s_source = Source::Radio;
                s_audioToken = token;
            }
        }
        else if (input.action == A::Mute)
        {
            const bool unmute = s_userMuted || s_volume == 0;
            if (unmute && s_volume == 0)
                setVolume(RadioPreferences::unmuteVolume(s_beforeMute, AudioManager::DefaultVolume));
            setUserMuted(!unmute);
            s_lastStatus = 0;
        }
        else if (input.action == A::Home)
            showScreen(Screen::Home);
        else if (input.action == A::Volume)
        {
            setVolume(constrain(input.value, 0, AudioManager::MaxVolume));
            s_lastStatus = 0;
        }
        else if (input.action == A::Tone && input.control >= 0 && input.control < 3)
        {
            s_tone[input.control] = constrain(input.value, -12, 12);
            g_audioManager.setTone(s_tone[0], s_tone[1], s_tone[2]);
            s_lastStatus = 0;
        }
        else if (input.action == A::Balance)
        {
            s_balance = constrain(input.value, -16, 16);
            g_audioManager.setBalance(s_balance);
            s_lastStatus = 0;
        }
        else if (input.action == A::EyeChanged)
        {
            Settings::Request request;
            request.type = Settings::RequestType::Save;
            request.id = UINT32_MAX - 2;
            request.baselineRevision = s_services.configuration(request.configuration);
            request.configuration.eyeStyle = input.value;
            if (!s_services.request(request))
            {
                s_radio.setEyeStyle(s_config.eyeStyle);
                LOG_ERROR("RADIO Röhrenstil nicht gespeichert: Dienst belegt");
            }
        }
        else if (input.action == A::Reset)
        {
            // Dieselben Vorgaben wie auf einem Gerät ohne gespeicherte Radiowerte: Pegel, Balance, Klang, Modus und Stile.
            const RadioPreferences defaults;
            setVolume(defaults.volume);
            s_balance = defaults.balance;
            s_visualizer.applyPreferences(defaults);
            memcpy(s_tone, defaults.tone, sizeof(s_tone));
            g_audioManager.setBalance(defaults.balance);
            g_audioManager.setTone(defaults.tone[0], defaults.tone[1], defaults.tone[2]);
            s_lastStatus = 0;
        }
        else if (input.action == A::Sleep)
        {
            applyWindow(s_timeWindow.sleepNow(), millis());
        }
    }

    /** @brief Führt externe Senderwahl ebenfalls durch die native Zeigerbewegung. */
    void nextStation(int direction)
    {
        if (s_screen != Screen::Radio)
            showScreen(Screen::Radio);
        const int count = s_stations.getStationCount();
        for (int i = 1; i <= count; ++i)
        {
            const int index = (s_selectedStation + direction * i + count) % count;
            Station station;
            if (s_stations.getStation(index, station) && station.url[0])
            {
                radioAction(s_radio.selectStation(index, millis()));
                return;
            }
        }
    }

    /** @brief Kopiert SD-Aktionen ohne reentrante Statusantwort, Stopp/Stumm/Home haben Vorrang. */
    bool queueSdAction(void *, const SdPlayerAction &action)
    {
        using A = SdPlayerAction::Type;
        if (action.type == A::Stop || action.type == A::Mute || action.type == A::Home)
        {
            s_priorityAction = action;
            s_havePriority = true;
            return true;
        }
        return !s_services.fileLeased() && s_sdActions && xQueueSend(s_sdActions, &action, 0) == pdPASS;
    }

    /** @brief Kopiert SD-Dateiaufträge zum einzigen Dateidienst. */
    bool queueFiles(void *, const SdFileRequest &request)
    {
        return s_servicesReady && s_services.requestFiles(request);
    }

    /** @brief Fordert das Cover des laufenden SD-Titels an. @param event Von audio-lite gemeldete Lage des eingebetteten Bilds, beim Titelstart leer: Der Dienst sucht dann nur das Ordnerbild. */
    void requestSdCover(const AudioManager::CoverEvent &event)
    {
        AppServices::CoverRequest request;
        request.mediaGeneration = s_sdMedia;
        request.playbackId = s_sdPlayback;
        request.trackId = s_sdTrack;
        request.event = event;
        stringutils::safeCopy(request.path, s_sdPath);
        if (!s_services.requestCover(request))
            LOG_ERROR("SD Coverauftrag nicht angenommen");
    }

    /** @brief Verarbeitet höchstens einen angenommenen SD-Auftrag pro Loop. */
    void processSdAction()
    {
        SdPlayerAction action;
        if (s_havePriority)
        {
            action = s_priorityAction;
            s_havePriority = false;
            SdPlayerAction discarded;
            if (action.type != SdPlayerAction::Type::Mute)
                while (xQueueReceive(s_sdActions, &discarded, 0) == pdPASS)
                {
                }
        }
        else if (xQueueReceive(s_sdActions, &action, 0) != pdPASS)
            return;
        using A = SdPlayerAction::Type;
        s_sdFailed = false;
        if (action.type == A::Stop || action.type == A::Home)
        {
            stopAudio();
            s_sdStop = action.requestId;
            if (action.type == A::Home)
                showScreen(Screen::Home);
        }
        else if (action.type == A::Mute || action.type == A::Volume || action.type == A::Balance || action.type == A::Tone || action.type == A::Reset || action.type == A::VisualizerChanged || action.type == A::Sleep)
        {
            RetroRadio::Input input;
            input.value = action.value;
            input.control = action.control;
            input.action = action.type == A::Mute ? RetroRadio::Action::Mute : action.type == A::Volume ? RetroRadio::Action::Volume
                                                                           : action.type == A::Balance  ? RetroRadio::Action::Balance
                                                                           : action.type == A::Tone     ? RetroRadio::Action::Tone
                                                                           : action.type == A::Reset    ? RetroRadio::Action::Reset
                                                                           : action.type == A::Sleep    ? RetroRadio::Action::Sleep
                                                                                                        : RetroRadio::Action::VisualizerChanged;
            radioAction(input);
            s_sdAck = action.requestId;
        }
        else if (action.type == A::RetryMedia)
        {
            s_services.retryMedia();
            s_sdAck = action.requestId;
        }
        else if (action.type == A::PlayFile)
        {
            stopAudio();
            stringutils::safeCopy(s_sdPath, action.path);
            s_sdPlayback = action.playbackId;
            s_sdTrack = action.trackId;
            s_sdMedia = action.mediaGeneration;
            char path[320];
            const int length = snprintf(path, sizeof(path), "/sdcard%s", action.path);
            const auto media = s_services.mediaStatus();
            const uint32_t token = nextAudioToken();
            s_sdFailed = s_services.fileLeased() || !s_codecReady || s_screen != Screen::Sd || media.media != SdServiceStatus::Media::Ready || action.mediaGeneration != media.mediaGeneration || !FileExplorer::validPath(action.path) || !FileExplorer::supportedFile(action.path) || length < 0 || static_cast<size_t>(length) >= sizeof(path) || !g_audioManager.requestFile(path, token);
            s_source = Source::Sd;
            if (!s_sdFailed)
            {
                s_audioToken = token;
                s_sdControl = action.requestId;
                requestSdCover({});
            }
            else
                s_sdAck = action.requestId;
        }
        else
        {
            const bool current = s_source == Source::Sd && action.playbackId == s_sdPlayback && action.trackId == s_sdTrack && action.mediaGeneration == s_sdMedia;
            if (action.type == A::Pause)
                muteAudio();
            const bool accepted = current && (action.type == A::Seek ? g_audioManager.requestSeek(action.positionSeconds, action.requestId) : g_audioManager.requestPause(action.type == A::Pause, action.requestId));
            if (accepted)
                s_sdControl = action.requestId;
            else
            {
                s_sdFailed = true;
                s_sdAck = action.requestId;
            }
        }
    }

    /** @brief Meldet einen echten oder abgewiesenen zentralen Konfigurationsabschluss. */
    void completeSettings(const Settings::Request &request, bool success, Settings::Error error = Settings::Error::Service)
    {
        Settings::Completion c;
        c.type = request.type;
        c.id = request.id;
        c.outcome = success ? Settings::Outcome::Success : Settings::Outcome::Failed;
        c.error = success ? Settings::Error::None : error;
        s_settings.complete(c);
    }

    /** @brief Stoppt Audio vor WLAN-Zugriff, die Zugangsdaten bleiben bis zur Übergabe im begrenzten RAM-Puffer. */
    bool deferWifi(const Settings::Request &request)
    {
        if (!s_servicesReady || s_wifiPending || s_services.wifiBusy())
            return false;
        stopAudio();
        s_wifiWaiting = request;
        s_wifiPending = true;
        s_wifiWaitingAt = millis();
        return true;
    }

    /** @brief Bearbeitet UI-Aufträge und liefert ausschließlich bestätigte Dienstergebnisse zurück. */
    void processSettings()
    {
        Settings::Request request;
        while (s_settings.takeRequest(request))
        {
            using T = Settings::RequestType;
            if (request.type == T::Mute)
                completeSettings(request, setUserMuted(true));
            else if (request.type == T::PreviewSound)
            {
                completeSettings(request, applySound(request.sound), Settings::Error::Invalid);
            }
            else if (request.type == T::PreviewScreen)
            {
                completeSettings(request, applyScreen(request.configuration.brightness, request.configuration.rotation, request.configuration.autoRotate), Settings::Error::Invalid);
            }
            else if (request.type == T::SetVolume)
            {
                setVolume(std::min(request.sound.volume, AudioManager::MaxVolume));
                s_lastStatus = 0;
                completeSettings(request, true);
            }
            else if (request.type == T::PreviewPlayback)
            {
                bool ok = true;
                if (!request.sound.playing)
                    stopAudio();
                else
                {
                    Station station;
                    const uint32_t token = nextAudioToken();
                    muteAudio();
                    ok = s_codecReady && !s_services.wifiBusy() && !s_wifiPending && s_services.networkStatus().connected && s_stations.getStation(s_selectedStation, station) && g_audioManager.requestStart(station.url, s_selectedStation, token);
                    if (ok)
                    {
                        s_source = Source::Radio;
                        s_audioToken = token;
                    }
                }
                s_lastStatus = 0;
                completeSettings(request, ok, Settings::Error::NoNetwork);
            }
            else if (request.type == T::Home)
            {
                completeSettings(request, true);
                showScreen(Screen::Home);
            }
            else if (request.type == T::Restart)
            {
                stopAudio();
                s_restart = true;
                s_restartAt = millis();
                completeSettings(request, true);
            }
            else if (request.type == T::Scan || request.type == T::TestProfile)
            {
                if (!deferWifi(request))
                {
                    if (request.type == T::Scan)
                        s_settings.completeScan(request.id, Settings::Outcome::Failed, nullptr, 0, false, Settings::Error::Busy);
                    else
                        completeSettings(request, false, Settings::Error::Busy);
                }
            }
            else if ((request.type == T::CancelScan || request.type == T::CancelTest) && s_wifiPending && request.targetId == s_wifiWaiting.id)
            {
                s_wifiPending = false;
                mbedtls_platform_zeroize(&s_wifiWaiting, sizeof(s_wifiWaiting));
            }
            else if (!s_servicesReady || !s_services.request(request))
            {
                if (request.type == T::Save || request.type == T::ResetDefaults)
                    s_settings.completeSave(request.id, false, s_config, 0, Settings::Error::Busy);
                else
                    completeSettings(request, false, Settings::Error::Busy);
            }
            mbedtls_platform_zeroize(&request, sizeof(request));
        }
        if (s_wifiPending && (audioStopped() || millis() - s_wifiWaitingAt > 15000))
        {
            const bool submitted = audioStopped() && s_services.request(s_wifiWaiting);
            if (!submitted)
            {
                if (s_wifiWaiting.type == Settings::RequestType::Scan)
                    s_settings.completeScan(s_wifiWaiting.id, Settings::Outcome::Failed, nullptr, 0, false, Settings::Error::Busy);
                else
                    completeSettings(s_wifiWaiting, false, Settings::Error::Busy);
                LOG_PLAIN_F("WIFI_PROFILE ERROR_START");
            }
            s_wifiPending = false;
            mbedtls_platform_zeroize(&s_wifiWaiting, sizeof(s_wifiWaiting));
        }
        static AppServices::Result result;
        while (s_services.takeResult(result))
        {
            using T = Settings::RequestType;
            if (result.type == T::Save || result.type == T::ResetDefaults)
            {
                s_settings.completeSave(result.id, result.outcome == Settings::Outcome::Success, result.config, result.revision, result.error);
                // Der Log-Schalter schaltet nur die Logausgabe, alle anderen Werte bleiben unverändert und werden nicht neu angewendet.
                if (result.outcome == Settings::Outcome::Success && result.changedFields == Settings::SerialLogs)
                {
                    s_config = result.config;
                    Logger::getInstance().setEnabled(s_config.serialLogs);
                }
                // Die PIN-Sperre wirkt sofort: Ausgeschaltet gibt sie einen offenen Sperrbildschirm frei, eingeschaltet meldet sie alle Websitzungen ab, die Browser sperren dann.
                else if (result.outcome == Settings::Outcome::Success && result.changedFields == Settings::PinLock)
                {
                    s_config = result.config;
                    if (s_config.pinLock)
                        s_web.invalidateSessions();
                    else
                    {
                        s_corner.reset();
                        if (s_access.active())
                        {
                            s_access.deactivate();
                            s_home.invalidate();
                        }
                    }
                    LOG_INFO_F("PINSPERRE Schalter=%u", s_config.pinLock);
                }
                // Der Sekundenzeiger wirkt sofort und ohne Neustart, alle anderen Werte und die Websitzungen bleiben unverändert.
                else if (result.outcome == Settings::Outcome::Success && result.changedFields == Settings::SecondHand)
                {
                    s_config = result.config;
                    s_home.setSecondSweep(s_config.secondSweep);
                    LOG_INFO_F("SEKUNDENZEIGER Schleichend=%u", s_config.secondSweep);
                }
                else if (result.outcome == Settings::Outcome::Success)
                {
                    s_config = result.config;
                    s_web.invalidateSessions();
                    s_settings.setConfiguration(s_config, result.revision);
                    s_visualizer.setLanguage(s_config.language);
                    s_sd.setLanguage(s_config.language);
                    s_visualizer.setVuSettings(s_config.vuBallistics, s_config.vuSensitivity);
                    s_radio.setEyeStyle(s_config.eyeStyle);
                    applySound(s_services.sound());
                    applyScreen(s_config.brightness, s_config.rotation, s_config.autoRotate);
                    s_home.setSecondSweep(s_config.secondSweep);
                    Logger::getInstance().setEnabled(s_config.serialLogs);
                    if (s_screen == Screen::Settings)
                        s_settings.show(s_config.language);
                    s_lastStatus = 0;
                }
            }
            else if (result.type == T::Scan)
                s_settings.completeScan(result.id, result.outcome, result.networks, result.count, result.more, result.error);
            else
            {
                Settings::Completion c;
                c.type = result.type;
                c.id = result.id;
                c.outcome = result.outcome;
                c.error = result.error;
                memcpy(c.ipv4, result.ip, sizeof(c.ipv4));
                s_settings.complete(c);
            }
            if (result.id == s_webSettingsId)
            {
                const bool success = result.outcome == Settings::Outcome::Success;
                const int code = success ? 200 : result.error == Settings::Error::Conflict     ? 409
                                             : result.error == Settings::Error::Invalid        ? 400
                                             : result.error == Settings::Error::Authentication ? 403
                                                                                               : 500;
                s_web.complete(result.id, code);
                s_webSettingsId = 0;
                if (success)
                {
                    s_settings.setConfiguration(s_config, result.revision);
                    // Das Umschalten des Sekundenzeigers im Webportal meldet den Browser nicht ab.
                    if (result.changedFields != Settings::SecondHand)
                        s_web.invalidateSessions();
                    if (s_webRestart)
                    {
                        stopAudio();
                        s_restart = true;
                        s_restartAt = millis();
                    }
                }
                s_webRestart = false;
            }
            if (result.id == ThemeRequestId)
            {
                const bool success = result.outcome == Settings::Outcome::Success;
                LOG_PLAIN_F("THEMA_USB %s Revision=%llu", success ? "GESPEICHERT NEUSTART" : "FEHLER", static_cast<unsigned long long>(result.revision));
                if (success)
                {
                    stopAudio();
                    s_restart = true;
                    s_restartAt = millis();
                }
            }
            mbedtls_platform_zeroize(&result, sizeof(result));
        }
        AppServices::ModernResult modern;
        while (s_services.takeModernResult(modern))
            if (modern.id == ModernRequestId && modern.error != Settings::Error::None)
                LOG_PLAIN_F("MODERN_USB FEHLER Grund=%u", static_cast<unsigned>(modern.error));
            else if (modern.id == ModernRequestId)
            {
                LOG_PLAIN_F("MODERN_USB GESPEICHERT NEUSTART Eintrag=%u", modern.stored);
                stopAudio();
                s_restart = true;
                s_restartAt = millis();
            }
            else if (s_webModernId && modern.id == s_webModernId)
            {
                const bool success = modern.error == Settings::Error::None;
                const bool theme = s_webTheme.type == Settings::RequestType::Save;
                s_webModernId = 0;
                if (success)
                    LOG_PLAIN_F("MODERN_WEB GESPEICHERT Eintrag=%u Neustart=%u Thema=%u", modern.stored, s_webModernRestart, theme);
                else
                    LOG_PLAIN_F("MODERN_WEB FEHLER Grund=%u", static_cast<unsigned>(modern.error));
                // Mit Thema beantwortet erst dessen Speichern den Webauftrag und startet höchstens einmal neu.
                if (success && theme && s_services.request(s_webTheme))
                {
                    s_webSettingsId = modern.id;
                    s_webRestart = s_webModernRestart;
                }
                else if (!success)
                    s_web.complete(modern.id, modern.error == Settings::Error::Invalid ? 400 : 500);
                else if (theme)
                    s_web.complete(modern.id, 503);
                else
                {
                    s_web.complete(modern.id, 200);
                    if (s_webModernRestart)
                    {
                        stopAudio();
                        s_restart = true;
                        s_restartAt = millis();
                    }
                }
                s_webModernRestart = false;
                mbedtls_platform_zeroize(&s_webTheme, sizeof(s_webTheme));
                s_webTheme.type = Settings::RequestType::None;
            }
        if (s_restart && audioStopped() && millis() - s_restartAt > 1000)
            ESP.restart();
    }

    /** @brief Meldet laufende oder wartende SD-Wiedergabe, solange vergibt der Koordinator keine Kartensperre. */
    bool sdBusy()
    {
        return s_source == Source::Sd || uxQueueMessagesWaiting(s_sdActions);
    }

    /** @brief Beantwortet eine wartende Anforderung der Kartensperre durch den FTP-Dienst nach derselben Regel wie beim Webportal: keine Sperre bei laufender oder wartender SD-Wiedergabe und keine, solange das Webportal sie hält. */
    void processFtp()
    {
        if (!s_ftpReady)
            return;
        auto &requests = s_ftp.leaseRequests();
        const uint32_t id = requests.pending();
        if (!id)
            return;
        const auto answer = s_services.grantFileLease(FileLease::Owner::Ftp, sdBusy());
        if (requests.answer(id, answer))
            s_services.releaseFileLease(FileLease::Owner::Ftp);
    }

    /** @brief Liefert den Platz, auf dem der Sender von Platz old nach dem Umsortieren steht. @param order Bisheriger Platz je neuem Platz, MAX_STATIONS Einträge. @return Neuer Platz, old unverändert, wenn er außerhalb der Liste liegt (etwa -1 ohne Auswahl). */
    int movedStation(const uint8_t *order, int old)
    {
        for (int i = 0; i < MAX_STATIONS; ++i)
            if (order[i] == old)
                return i;
        return old;
    }

    /** @brief Meldet eine laufende USB-Bildübertragung, die den gemeinsamen Aufnahmeplatz belegt. Ohne USB-Fernsteuerung gibt es keine. */
    bool usbShotActive()
    {
#if CROWPANEL_USB_FERNSTEUERUNG
        return s_screenshot.active();
#else
        return false;
#endif
    }

    /** @brief Führt begrenzte Webaufträge im einzigen UI-/Audiokoordinator aus. */
    void processWeb()
    {
        if (!s_webReady)
            return;
        uint32_t completedId = 0;
        int completedCode = 200;
        static WebPortal::Command command;
        if (s_web.take(command))
        {
            using A = WebPortal::Action;
            int code = 200;
            bool deferred = false;
            if (command.action == A::Settings)
            {
                if (s_webSettingsId || !s_services.request(command.settings))
                    code = 503;
                else
                {
                    s_webSettingsId = command.id;
                    s_webRestart = command.restart;
                    deferred = true;
                }
            }
            else if (command.action == A::Theme)
            {
                // Grundfarben zuerst. Ein Thema im selben Auftrag folgt nach deren Abschluss, ohne Farben geht es direkt an den Dienst.
                const bool colors = command.values[0] != 0;
                if (s_webSettingsId || s_webModernId || !(colors ? s_services.requestModern(command.modern) : s_services.request(command.settings)))
                    code = 503;
                else if (colors)
                {
                    s_webModernId = command.id;
                    s_webModernRestart = command.restart;
                    s_webTheme = command.settings;
                    deferred = true;
                }
                else
                {
                    s_webSettingsId = command.id;
                    s_webRestart = command.restart;
                    deferred = true;
                }
            }
            else if (command.action == A::Screenshot)
                code = usbShotActive() ? 409 : s_web.capture(s_display) ? 200
                                                                        : 503;
            else if (command.action == A::LeaseFiles)
                code = s_services.grantFileLease(FileLease::Owner::Web, sdBusy()) == FileLease::Answer::Granted ? 200 : 409;
            else if (command.action == A::ReleaseFiles)
                s_services.releaseFileLease(FileLease::Owner::Web);
            else if (command.action == A::Reboot)
            {
                stopAudio();
                s_restart = true;
                s_restartAt = millis();
            }
            else if (command.action == A::StationsChanged)
            {
                // Speichern hier im UI-Task, sein Stack liegt im internen RAM. Der HTTP-Task mit PSRAM-Stack darf nicht in den Flash schreiben (Cache aus, Assert).
                if (!command.stations || !s_stations.saveStations(command.stations))
                    code = 500;
                else
                {
                    if (command.reorder)
                    {
                        // Erst hier zuordnen, die lokale Senderwahl kann seit dem Eingang der Anfrage gewechselt haben. Der laufende Stream bleibt und gehört ab jetzt zum neuen Platz.
                        s_selectedStation = movedStation(command.order, s_selectedStation);
                    }
                    else if (s_source == Source::Radio && s_selectedStation >= 0 && s_selectedStation < MAX_STATIONS && (command.values[s_selectedStation / 24] & (1u << (s_selectedStation % 24))))
                        stopAudio();
                    ++s_stationsRevision;
                    s_radio.invalidate();
                }
            }
            else if (command.action == A::Station)
            {
                Station station;
                if (s_access.active() || s_timeWindow.sleeping() || s_screen != Screen::Radio || s_radio.modal())
                    code = 409;
                else if (!s_stations.getStation(command.values[0], station) || !station.url[0])
                    code = 400;
                else if (!s_services.networkStatus().connected)
                    code = 503;
                else if (s_source == Source::Radio && s_selectedStation == command.values[0] && g_audioManager.getStatus().state == AudioManager::State::Streaming)
                    code = 200;
                else
                {
                    // Antwort, sobald die Radioansicht die Auswahl übernommen hat. Zeigerfahrt und Senderstart laufen danach wie bei
                    // einer Auswahl am Display. Bis 20.11.2026 wartete die Antwort auf den laufenden Stream (bis 4,6 s gemessen), der einzige HTTP-Task
                    // beantwortete so lange keine Statusabfrage, und die Webnadel lief zum alten Sender zurück.
                    radioAction(s_radio.selectStation(command.values[0], millis()));
                    if (s_radioFailed)
                        code = 503;
                }
            }
            else
            {
                if (command.action == A::Volume)
                    setVolume(command.values[0]);
                else if (command.action == A::Mute)
                    code = setUserMuted(command.values[0]) ? 200 : 503;
                else if (command.action == A::Balance)
                {
                    s_balance = command.values[0];
                    g_audioManager.setBalance(s_balance);
                }
                else if (command.action == A::Tone)
                {
                    for (int i = 0; i < 3; ++i)
                        s_tone[i] = command.values[i];
                    g_audioManager.setTone(s_tone[0], s_tone[1], s_tone[2]);
                }
                s_lastStatus = 0;
            }
            if (!deferred)
            {
                completedId = command.id;
                completedCode = code;
            }
            mbedtls_platform_zeroize(&command, sizeof(command));
        }
        WebPortal::State value;
        value.locked = s_access.active() || s_timeWindow.sleeping();
        value.sdBusy = sdBusy();
        value.radio = s_screen == Screen::Radio && !value.locked;
        value.muted = s_userMuted;
        value.volume = s_volume;
        value.balance = s_balance;
        memcpy(value.tone, s_tone, 3);
        value.station = s_selectedStation;
        value.eyeStyle = s_config.eyeStyle;
        value.stationsRevision = s_stationsRevision;
        value.firmwareBytes = s_firmwareBytes;
        value.appAddress = s_appAddress;
        value.appCapacity = s_appCapacity;
        stringutils::safeCopy(value.title, s_title);
        const auto audio = g_audioManager.getStatus();
        value.playing = s_audioToken && audio.requestId == s_audioToken && audio.state == AudioManager::State::Streaming;
        snprintf(value.bitrate, sizeof(value.bitrate), "%lu kbps", static_cast<unsigned long>(audio.bitRate / 1000));
        s_web.publish(value);
        if (completedId)
            s_web.complete(completedId, completedCode);
    }

    /** @brief Fragt die Adressen 0x08..0x77 über Wire ab und listet die antwortenden kommagetrennt. Eine zu lange Liste endet mit ", ...". @param list Zielpuffer. @param size Größe des Zielpuffers. */
    void scanI2cBus(char *list, size_t size)
    {
        list[0] = 0;
        for (uint8_t address = 0x08; address < 0x78; ++address)
        {
            Wire.beginTransmission(address);
            if (Wire.endTransmission() != 0)
                continue;
            char item[8];
            snprintf(item, sizeof(item), "%s0x%02X", list[0] ? ", " : "", address);
            if (strlen(list) + strlen(item) + 6 > size)
            {
                stringutils::safeAppend(list, size, ", ...");
                break;
            }
            stringutils::safeAppend(list, size, item);
        }
    }

    /** @brief Einmaliger I2C-Startscan. Das CrowPanel hat einen gemeinsamen Bus für Touch, IO-Expander und den I2C-Stecker. Nur vor dem Start von Audio, Diensten und Touchschleife aufrufen, damit kein anderer Zugriff dazwischenkommt. */
    void scanI2cBuses()
    {
        scanI2cBus(s_i2cInternalList, sizeof(s_i2cInternalList));
        s_i2cInternal = Settings::BusScan::Done;
        LOG_INFO_F("I2C Startscan: %s", s_i2cInternalList);
    }

    /** @brief Erstellt zusammengehörige lokale Zeit-/Netzabbilder höchstens einmal pro Sekunde. */
    void updateSystemStatus(uint32_t now)
    {
        if (s_lastStatus && now - s_lastStatus < 1000)
            return;
        s_lastStatus = now;
        const time_t current = time(nullptr);
        tm local{};
        localtime_r(&current, &local);
        const bool valid = local.tm_year >= 100 && local.tm_year < 200;
        auto &home = s_homeStatus;
        ++home.revision;
        home.language = s_config.language;
        home.timeZoneRevision = s_services.timeRevision();
        home.timeValid = valid && local.tm_year >= 124;
        home.year = local.tm_year + 1900;
        home.month = local.tm_mon + 1;
        home.day = local.tm_mday;
        home.weekday = local.tm_wday;
        home.hour = local.tm_hour;
        home.minute = local.tm_min;
        home.second = local.tm_sec;
        home.dstActive = local.tm_isdst > 0;
        home.ntpSynced = s_services.ntpSynced();
        const auto net = s_services.networkStatus();
        const bool connected = net.connected;
        home.wifi = connected ? MainScreen::WifiState::Connected : (s_wifiPending || s_services.wifiBusy() || s_services.wifiConnecting()) ? MainScreen::WifiState::Connecting
                                                                                                                                           : MainScreen::WifiState::Disconnected;
        home.signalValid = connected;
        home.signalPercent = std::max(0, std::min(100, 2 * (static_cast<int>(net.rssi) + 100)));
        home.ipv4Valid = connected && net.ip[0];
        memcpy(home.ipv4, net.ip, 4);
        stringutils::safeCopy(home.ssid, net.ssid);
        s_home.setStatus(home);
        auto &status = s_settingsStatus;
        ++status.revision;
        status.timeValid = valid;
        status.ntpConfirmed = home.ntpSynced;
        status.localTime = {home.year, home.month, home.day, home.hour, home.minute};
        status.wifiConnected = status.signalValid = connected;
        status.rssi = net.rssi;
        memcpy(status.ipv4, home.ipv4, 4);
        stringutils::safeCopy(status.connectedSsid, home.ssid);
        stringutils::safeCopy(status.mac, net.mac);
        memcpy(status.gateway, net.gateway, 4);
        status.channel = net.channel;
        status.second = local.tm_sec;
        status.heapFree = ESP.getFreeHeap();
        status.psramFree = ESP.getFreePsram();
        status.firmwareBytes = s_firmwareBytes;
        status.sound = s_audioSettings;
        status.sound.volume = s_volume;
        const auto audio = g_audioManager.getStatus();
        status.sound.playing = s_audioToken && (audio.requestId != s_audioToken || audio.state == AudioManager::State::Starting || audio.state == AudioManager::State::Streaming);
        status.rmsValid = s_audioToken && audio.requestId == s_audioToken && audio.state == AudioManager::State::Streaming && audio.pcmAgeMs < 1000;
        const float rms = std::max(g_audioManager.getRMSLeft(), g_audioManager.getRMSRight());
        status.rmsDb = status.rmsValid ? 20.0f * log10f(std::max(0.001f, rms)) : -60.0f;
        status.pinKnown = status.pinSet = s_servicesReady;
        status.uptimeSeconds = now / 1000;
        status.flashBytes = s_flashBytes;
        status.psramBytes = s_psramBytes;
        status.i2cInternal = s_i2cInternal;
        stringutils::safeCopy(status.i2cInternalList, s_i2cInternalList);
        float chipTemp = 0.0f;
        status.chipTempValid = s_chipTemp && temperature_sensor_get_celsius(s_chipTemp, &chipTemp) == ESP_OK;
        status.chipTempC10 = status.chipTempValid ? static_cast<int16_t>(lroundf(chipTemp * 10.0f)) : 0;
        status.ftpEnabled = s_ftpReady && s_ftp.enabled();
        stringutils::safeCopy(status.device, "Elecrow CrowPanel 7.0\" · ESP32-S3");
        stringutils::safeCopy(status.firmware, FW_VERSION);
        s_settings.setStatus(status);
        applyWindow(s_timeWindow.evaluate(home.timeValid, local, s_config.timeWindows), now);
    }

    /** @brief Verarbeitet nur Metadaten der aktuellen, quellenübergreifend eindeutigen Wiedergabe. */
    void updatePlayback(uint32_t now)
    {
        const auto audio = g_audioManager.getStatus();
        const bool current = s_audioToken && audio.requestId == s_audioToken;
        if (s_listening && (s_userMuted || !s_audioSettings.speakers || !current || audio.state != AudioManager::State::Streaming || audio.pcmAgeMs > 1000))
            muteAudio();
        if (!s_listening && !s_userMuted && s_audioSettings.speakers && s_codecReady)
            enableAudio();
        AudioManagerEventType type;
        char event[AudioManager::MAX_EVENT_STRING_LENGTH];
        uint32_t token;
        for (int i = 0; i < 12 && g_audioManager.dequeueEvent(type, event, sizeof(event), &token); ++i)
        {
            if (!s_audioToken || token != s_audioToken)
                continue;
            if (type == AudioManagerEventType::STREAM_TITLE || type == AudioManagerEventType::ID3_TITLE)
                stringutils::safeCopyUtf8(s_title, event);
            else if (type == AudioManagerEventType::ID3_ARTIST)
                stringutils::safeCopyUtf8(s_artist, event);
            else if (type == AudioManagerEventType::ID3_ALBUM)
                stringutils::safeCopyUtf8(s_album, event);
        }
        RetroRadio::State radio;
        radio.language = s_config.language;
        radio.stationsRevision = s_stationsRevision;
        radio.acknowledgedRequestId = s_radioAck;
        radio.selectedStation = s_selectedStation;
        radio.streamRequestId = s_radioStream;
        // Jeder Senderwechsel stoppt den Stream zuerst, ein laufender gehört also stets zu s_selectedStation. Anders als audio.stationIndex folgt der Wert einem Umsortieren im Webportal.
        radio.streamStation = current && s_source == Source::Radio ? s_selectedStation : -1;
        radio.streamState = s_source == Source::Radio ? (current ? audio.state : AudioManager::State::Starting) : AudioManager::State::Idle;
        radio.requestFailed = s_radioFailed;
        radio.wifiConnected = WiFi.status() == WL_CONNECTED;
        radio.wifiSignalPercent = s_homeStatus.signalValid ? s_homeStatus.signalPercent : 0;
        radio.muted = s_userMuted;
        radio.timeValid = s_homeStatus.timeValid;
        radio.hour = s_homeStatus.hour;
        radio.minute = s_homeStatus.minute;
        radio.second = s_homeStatus.second;
        radio.volume = s_volume;
        radio.balance = s_balance;
        memcpy(radio.tone, s_tone, sizeof(s_tone));
        if (current && s_source == Source::Radio)
        {
            radio.sampleRate = audio.sampleRate;
            radio.bitRate = audio.bitRate;
            radio.codec = audio.codec;
            stringutils::safeCopy(radio.title, s_title);
        }
        Station station;
        if (s_stations.getStation(s_selectedStation, station))
            stringutils::safeCopy(radio.stationName, station.name);
        s_radio.setState(radio);
        if (s_sdStop && audioStopped())
        {
            s_sdAck = s_sdStop;
            s_sdStop = 0;
        }
        if (s_sdControl && current && (audio.controlRequestId == s_sdControl || s_sdControl == s_sdPlayback))
        {
            s_sdAck = s_sdControl;
            s_sdFailed = audio.commandFailed;
            s_sdControl = 0;
        }
        SdPlaybackStatus sd;
        sd.revision = ++s_sdRevision;
        sd.mediaGeneration = s_sdMedia;
        sd.playbackId = s_sdPlayback;
        sd.trackId = s_sdTrack;
        sd.acknowledgedRequest = s_sdAck;
        sd.muted = s_userMuted;
        sd.commandFailed = s_sdFailed;
        sd.source = s_source == Source::Sd ? SdPlaybackStatus::Source::Sd : s_source == Source::Radio ? SdPlaybackStatus::Source::Radio
                                                                                                      : SdPlaybackStatus::Source::None;
        using State = SdPlaybackStatus::State;
        if (s_source != Source::Sd)
            sd.state = State::Stopped;
        else if (s_sdFailed)
            sd.state = State::Error;
        else if (!current)
            sd.state = State::Starting;
        else
        {
            sd.state = audio.state == AudioManager::State::Error ? State::Error : audio.state == AudioManager::State::Ended                          ? State::Ended
                                                                              : audio.state == AudioManager::State::Paused                           ? State::Paused
                                                                              : audio.state == AudioManager::State::Streaming && audio.fileConfirmed ? State::Playing
                                                                              : audio.state == AudioManager::State::Idle                             ? State::Stopped
                                                                                                                                                     : State::Starting;
            sd.fileConfirmed = audio.fileConfirmed;
            sd.positionSeconds = audio.positionSeconds;
            sd.durationSeconds = audio.durationSeconds;
            sd.bitRate = audio.bitRate;
            sd.durationKnown = audio.durationSeconds > 0;
            sd.seekable = audio.fileConfirmed && sd.durationKnown && audio.state == AudioManager::State::Streaming;
            stringutils::safeCopyUtf8(sd.title, s_title);
            stringutils::safeCopyUtf8(sd.artist, s_artist);
            stringutils::safeCopyUtf8(sd.album, s_album);
        }
        sd.timeValid = s_homeStatus.timeValid;
        sd.hour = s_homeStatus.hour;
        sd.minute = s_homeStatus.minute;
        sd.second = s_homeStatus.second;
        sd.volume = s_volume;
        sd.balance = s_balance;
        memcpy(sd.tone, s_tone, 3);
        s_sd.setServices(s_services.mediaStatus());
        s_sd.setPlayback(sd);
        AudioManager::CoverEvent coverEvent;
        if (g_audioManager.takeCover(coverEvent) && s_source == Source::Sd && coverEvent.requestId == s_audioToken)
            requestSdCover(coverEvent);
        SdCoverFrame cover;
        if (s_services.takeCover(cover))
        {
            s_sd.setCover(cover);
            s_services.releaseCover();
        }
        SdFileResult files;
        if (s_services.takeFiles(files))
        {
            s_sd.acceptFiles(files);
            s_services.releaseFiles();
        }
    }

    /** @brief Verteilt einen Kontakt ausschließlich an das oberste sichtbare Modul. */
    void handleTouch(SettingsUi::TouchPhase phase, int16_t x, int16_t y, uint32_t now)
    {
        const auto accessPhase = static_cast<CrowPanelAccess::TouchPhase>(phase);
        const bool contactHeld = phase == SettingsUi::TouchPhase::Press || phase == SettingsUi::TouchPhase::Move;
        const auto wake = s_timeWindow.observeTouch(contactHeld);
        if (wake != CrowPanelTimeWindow::Event::None)
        {
            applyWindow(wake, now, contactHeld);
            return;
        }
        if (s_timeWindow.sleeping())
            return;
        if (s_access.active())
        {
            if (s_access.touch(accessPhase, x, y, now))
                s_home.invalidate();
            return;
        }
        // Die Eckengeste sperrt nur bei eingeschalteter PIN-Sperre.
        if (s_screen == Screen::Home && s_config.pinLock)
        {
            const bool wasTracking = s_corner.tracking();
            if (s_corner.touch(accessPhase, x, y))
            {
                showScreen(Screen::Home);
                s_access.activate(now, contactHeld);
                return;
            }
            if (wasTracking || s_corner.tracking())
                return;
        }
        if (s_homeNavigationContact)
        {
            if (phase == SettingsUi::TouchPhase::Release || phase == SettingsUi::TouchPhase::Cancel)
                s_homeNavigationContact = false;
            return;
        }
        if (s_screen == Screen::Home)
        {
            const auto action = s_home.touch(static_cast<MainScreen::TouchPhase>(phase), x, y);
            s_homeNavigationContact = action != MainScreen::Action::None;
            if (action == MainScreen::Action::Radio)
                showScreen(Screen::Radio);
            else if (action == MainScreen::Action::SdPlayer)
                showScreen(Screen::Sd);
            else if (action == MainScreen::Action::Settings)
                showScreen(Screen::Settings);
        }
        else if (s_screen == Screen::Radio)
            radioAction(s_radio.touch(static_cast<RetroRadio::TouchPhase>(phase), x, y, now));
        else if (s_screen == Screen::Settings)
            s_settings.touch(phase, x, y);
        else
            s_sd.touch({static_cast<SdTouchPhase>(phase), x, y, now});
    }

#if CROWPANEL_USB_FERNSTEUERUNG
    /** @brief Decodiert begrenzte USB-Hexfelder ohne Ausgabe von Zugangsdaten. */
    bool decodeWifiField(const char *source, char *out, size_t capacity)
    {
        const size_t length = strlen(source);
        if (length % 2 || length / 2 >= capacity)
            return false;
        for (size_t i = 0; i < length; i += 2)
        {
            unsigned byte;
            if (!isxdigit(static_cast<unsigned char>(source[i])) || !isxdigit(static_cast<unsigned char>(source[i + 1])) || sscanf(source + i, "%2x", &byte) != 1 || byte == 0)
                return false;
            out[i / 2] = static_cast<char>(byte);
        }
        out[length / 2] = 0;
        return true;
    }

    /** @brief Nimmt ein WLAN-Profil per USB (Zeile \@wifi:<SSID hex>:<Passwort hex>) entgegen und leitet es nur als temporären Verbindungstest weiter, es wird niemals gespeichert. */
    void receiveWifiProfile(char *line)
    {
        Settings::Request request;
        request.type = Settings::RequestType::TestProfile;
        request.id = UINT32_MAX;
        char *password = strncmp(line, "@wifi:", 6) == 0 ? strchr(line + 6, ':') : nullptr;
        bool valid = false;
        if (password)
        {
            *password++ = 0;
            valid = decodeWifiField(line + 6, request.profile.ssid, sizeof(request.profile.ssid)) && request.profile.ssid[0] && decodeWifiField(password, request.profile.password, sizeof(request.profile.password));
        }
        LOG_PLAIN_F("%s", !valid ? "WIFI_PROFILE INVALID" : deferWifi(request) ? "WIFI_PROFILE ACCEPTED_RAM_ONLY"
                                                                               : "WIFI_PROFILE BUSY_OR_INVALID");
        mbedtls_platform_zeroize(&request, sizeof(request));
    }

    /** @brief Speichert den Themenindex aus einer USB-Zeile \@thema:<N> als vollständige Kopie der Konfiguration mit neuer Revision, der Neustart folgt nach dem bestätigten Speichern. Ein ungültiger Index ergibt nur eine Logzeile. */
    void receiveTheme(const char *line)
    {
        int index = -1, end = 0;
        if (sscanf(line, "@thema:%d%n", &index, &end) != 1 || line[end] || index < 0 || index >= ThemenAnzahl)
        {
            LOG_PLAIN_F("THEMA_USB INVALID");
            return;
        }
        Settings::Request request;
        request.type = Settings::RequestType::Save;
        request.id = ThemeRequestId;
        request.baselineRevision = s_services.configuration(request.configuration);
        request.configuration.theme = static_cast<uint8_t>(index);
        const bool accepted = s_servicesReady && s_services.request(request);
        LOG_PLAIN_F("THEMA_USB Index=%d %s", index, accepted ? "SPEICHERN" : "BUSY");
        // Die Kopie enthält auch die WLAN-Passwörter.
        mbedtls_platform_zeroize(&request, sizeof(request));
    }
#endif

    /** @brief Schreibt die von der Vorgabe abweichenden bearbeitbaren Grundfarben von Modern als „Name=RRGGBB,...“. @param text Puffer, 400 Byte reichen für alle 18. @return text oder „keine“. */
    const char *modernDifferences(const ModernFarben &colors, char *text, size_t size)
    {
        size_t used = 0;
        for (size_t i = ModernErsteBearbeitbare; i < ModernFarbenAnzahl; ++i)
            if (colors.rgb[i] != ModernVorgabe.rgb[i] && used < size)
                used += snprintf(text + used, size - used, "%s%s=%06lX", used ? "," : "", ModernGrundfarben[i].name, static_cast<unsigned long>(colors.rgb[i]));
        return used ? text : "keine";
    }

#if CROWPANEL_USB_FERNSTEUERUNG
    /** @brief Speichert aus einer USB-Zeile \@modernfarbe:<Name>:<RRGGBB> eine bearbeitbare Grundfarbe von Modern zusammen mit den übrigen gespeicherten oder stellt mit \@modernfarbe:vorgabe die Vorgaben wieder her, der Neustart folgt nach dem bestätigten Speichern. Jede andere Zeile ergibt nur eine Logzeile. */
    void receiveModern(const char *line)
    {
        AppServices::ModernRequest request;
        request.id = ModernRequestId;
        const char *argument = line + strlen("@modernfarbe:");
        const char *separator = strchr(argument, ':');
        char name[16]{};
        size_t index = ModernFarbenAnzahl;
        uint32_t value = 0;
        if (separator && static_cast<size_t>(separator - argument) < sizeof(name))
        {
            memcpy(name, argument, separator - argument);
            index = modernFarbeNachName(name);
        }
        if (strcmp(argument, "vorgabe") == 0)
            request.reset = true;
        else if (index < ModernErsteBearbeitbare || index >= ModernFarbenAnzahl || !rgbAusHex(separator + 1, value))
        {
            LOG_PLAIN_F("MODERN_USB INVALID");
            return;
        }
        else
        {
            s_services.modernColors(request.colors);
            request.colors.rgb[index] = value;
        }
        const bool accepted = s_servicesReady && s_services.requestModern(request);
        if (request.reset)
            LOG_PLAIN_F("MODERN_USB VORGABE %s", accepted ? "SPEICHERN" : "BUSY");
        else
            LOG_PLAIN_F("MODERN_USB %s=%06lX %s", name, static_cast<unsigned long>(value), accepted ? "SPEICHERN" : "BUSY");
    }

    /** @brief Verarbeitet die USB-Zeilen \@ftp:an und \@ftp:aus (Schalter nur im RAM), \@ftp:neu (neues Passwort) und \@ftp:passwort:<32 Hex>. Das Passwort verlässt das Gerät dabei nur mit dem vom Host gewählten Einmalschlüssel per XOR verknüpft, nie im Klartext. Jede andere Zeile ergibt FTP_USB INVALID. */
    void receiveFtp(const char *line)
    {
        const char *argument = line + strlen("@ftp:");
        if (!s_ftpReady)
            LOG_PLAIN_F("FTP_USB NICHT_BEREIT");
        else if (!strcmp(argument, "an") || !strcmp(argument, "aus"))
        {
            s_ftp.setEnabled(argument[1] == 'n');
            LOG_PLAIN_F("FTP_USB %s", argument[1] == 'n' ? "AN" : "AUS");
        }
        else if (!strcmp(argument, "neu"))
            LOG_PLAIN_F("FTP_USB NEU %s", s_ftp.renewPassword() ? "ANGEFORDERT" : "FEHLER");
        else if (!strncmp(argument, "passwort:", 9) && strlen(argument + 9) == 2 * FtpProtocol::PasswordCapacity && strspn(argument + 9, "0123456789abcdefABCDEF") == 2 * FtpProtocol::PasswordCapacity)
        {
            uint8_t key[FtpProtocol::PasswordCapacity];
            for (size_t i = 0; i < sizeof(key); ++i)
            {
                unsigned byte = 0;
                sscanf(argument + 9 + 2 * i, "%2x", &byte);
                key[i] = static_cast<uint8_t>(byte);
            }
            char password[FtpProtocol::PasswordCapacity]{}, hex[2 * FtpProtocol::PasswordCapacity + 1]{};
            if (s_ftp.password(password))
            {
                for (size_t i = 0; i < sizeof(key); ++i)
                    snprintf(hex + 2 * i, 3, "%02X", static_cast<uint8_t>(password[i]) ^ key[i]);
                LOG_PLAIN_F("FTP_USB PASSWORT %s", hex);
            }
            else
                LOG_PLAIN_F("FTP_USB PASSWORT FEHLT");
            mbedtls_platform_zeroize(key, sizeof(key));
            mbedtls_platform_zeroize(password, sizeof(password));
            mbedtls_platform_zeroize(hex, sizeof(hex));
        }
        else
            LOG_PLAIN_F("FTP_USB INVALID");
    }

    /** @brief Protokolliert den Stand der Ausgabe (Zeile CODEC). Das CrowPanel hat keinen Codec, der NS4168 hat keine Register, stumm heißt Nullsamples. */
    /** @brief Testton per USB: \@ton:<Hz>:<dBFS> ersetzt die laufende Ausgabe hinter Lautstärke und Klangregelung durch einen Sinus mit festem Spitzenpegel, \@ton:aus blendet ihn aus. Klingt nur, solange Radio oder SD-Player spielen. */
    void receiveTone(const char *line)
    {
        unsigned hz = 0;
        float dbfs = 0.0f;
        int end = 0;
        if (strcmp(line, "@ton:aus") == 0)
            hz = 0;
        else if (sscanf(line, "@ton:%u:%f%n", &hz, &dbfs, &end) != 2 || line[end] || hz < 20 || hz > 20000 || dbfs < -60.0f || dbfs > 0.0f)
        {
            LOG_PLAIN_F("TON INVALID");
            return;
        }
        g_audioManager.setTestTone(static_cast<uint16_t>(hz), dbfs);
        const auto status = g_audioManager.getStatus();
        LOG_PLAIN_F("TON Hz=%u dBFS=%.1f Wiedergabe=%d", hz, hz ? dbfs : -999.0f, static_cast<int>(status.state));
    }

    void logCodecRegisters()
    {
        bool speaker = false;
        const bool read = CrowPanelBoard::readSpeaker(speaker);
        LOG_PLAIN_F("CODEC keiner, NS4168 Ausgabe=%d", read ? speaker : -1);
    }

    /** @brief Verarbeitet eine USB-Zeile: \@touch:<p|m|r>:<x>:<y> speist einen Kontakt über denselben zentralen Eingabeweg wie den GT911 ein, \@thema:<N> speichert das Thema und startet neu, \@modernfarbe:... speichert Grundfarben von Modern und startet neu, \@ftp:... steuert den FTP-Dienst, \@codec protokolliert den Verstärkerstand, \@zeichnen:aus/ein pausiert das Zeichnen ohne Quellenwechsel, jede andere Zeile gilt als WLAN-Profil. */
    void receiveDiagnosticLine(char *line)
    {
        if (strcmp(line, "@zeichnen:aus") == 0 || strcmp(line, "@zeichnen:ein") == 0)
        {
            s_drawingPaused = strcmp(line, "@zeichnen:aus") == 0;
            LOG_PLAIN_F("ZEICHNEN Pause=%u", static_cast<unsigned>(s_drawingPaused));
            return;
        }
        if (strcmp(line, "@codec") == 0)
        {
            logCodecRegisters();
            return;
        }
        if (strncmp(line, "@ftp:", 5) == 0)
        {
            receiveFtp(line);
            return;
        }
        if (strncmp(line, "@thema:", 7) == 0)
        {
            receiveTheme(line);
            return;
        }
        if (strncmp(line, "@modernfarbe:", 13) == 0)
        {
            receiveModern(line);
            return;
        }
        if (strncmp(line, "@ton:", 5) == 0)
        {
            receiveTone(line);
            return;
        }
        if (strncmp(line, "@touch:", 7) != 0)
        {
            receiveWifiProfile(line);
            return;
        }
        char phase = 0;
        int x = -1, y = -1, end = 0;
        if (sscanf(line, "@touch:%c:%d:%d%n", &phase, &x, &y, &end) != 3 || line[end] || x < 0 || x >= CrowPanelRender::PanelWidth || y < 0 || y >= CrowPanelRender::PanelHeight || (phase != 'p' && phase != 'r' && phase != 'm') || (s_screen != Screen::Settings && s_screen != Screen::Home && s_screen != Screen::Radio && s_screen != Screen::Sd && !s_homeNavigationContact))
            return;
        handleTouch(phase == 'p' ? SettingsUi::TouchPhase::Press : phase == 'r' ? SettingsUi::TouchPhase::Release
                                                                                : SettingsUi::TouchPhase::Move,
                    x, y, millis());
    }

    /** @brief Liest die USB-Diagnosebefehle (Einzelbuchstaben und mit \@ beginnende Zeilen). m schaltet den Audioausgang stumm, s stoppt, ? meldet den Status. */
    void handleUsb()
    {
        Logger::AnswerScope answer; // Antworten auf USB-Befehle kommen auch bei ausgeschalteten Logs
        static char line[204]{};
        static size_t used = 0;
        static bool receiving = false, overflow = false;
        static uint32_t receivedAt = 0;
        if (receiving && millis() - receivedAt > 5000)
        {
            LOG_PLAIN_F("WIFI_PROFILE INCOMPLETE");
            mbedtls_platform_zeroize(line, sizeof(line));
            receiving = overflow = false;
            used = 0;
        }
        for (int i = 0; i < 256; ++i)
        {
            // Einziger Befehlskanal ist UART0 über den CH340.
            Stream *source = nullptr;
            if (Serial.available() > 0)
                source = &Serial;
            if (!source)
                break;
            const char command = source->read();
            if (!receiving && command == '@')
            {
                receiving = true;
                receivedAt = millis();
            }
            if (receiving)
            {
                if (command == '\n')
                {
                    if (!overflow)
                    {
                        line[used] = 0;
                        receiveDiagnosticLine(line);
                    }
                    else
                        LOG_PLAIN_F("WIFI_PROFILE INVALID");
                    mbedtls_platform_zeroize(line, sizeof(line));
                    receiving = overflow = false;
                    used = 0;
                }
                else if (used < sizeof(line) - 1)
                    line[used++] = command;
                else
                    overflow = true;
                continue;
            }
            if (command == 'm')
                setUserMuted(true);
            else if (command == 's')
                stopAudio();
            else if ((s_access.active() || s_timeWindow.sleeping()) && strchr("np hakdrv", command))
                continue;
            else if (command == 'n')
                nextStation(1);
            else if (command == 'p')
                nextStation(-1);
            else if (command == 'h')
                showScreen(Screen::Home);
            else if (command == 'a')
                showScreen(Screen::Radio);
            else if (command == 'k')
                showScreen(Screen::Settings);
            else if (command == 'd')
                showScreen(Screen::Sd);
            else if (command == 'r')
            {
                if (s_screen != Screen::Radio)
                    showScreen(Screen::Radio);
                radioAction(s_radio.selectStation(s_selectedStation, millis()));
            }
            else if (command == 'v')
            {
                s_visualizer.nextMode(1);
                LOG_PLAIN_F("VIS Modus=%u", static_cast<unsigned>(s_visualizer.mode()));
            }
            else if (command == 'c')
            {
                s_captureRequested = true;
                s_captureTarget = source;
            }
            else if (command == 'x')
            {
                s_captureRequested = false;
                s_screenshot.cancel();
            }
            else if (command == '?')
            {
                const auto ppa = CrowPanelRender::ppaCounters();
                LOG_PLAIN_F("UI Ansicht=%u Sperre=%u Schlaf=%u PINSperre=%u Rotation=%u Helligkeit=%u PPAZeitgrenze=%lu PPAVerspaetet=%lu Bilder=%lu PPA=%u PPAAuftraege=%lu VollbildUs=%lu Zeichenpause=%u Ms=%lu", static_cast<unsigned>(s_screen), s_access.active(), s_timeWindow.sleeping(), s_config.pinLock, s_display.getRotation(), s_display.getBrightness(), static_cast<unsigned long>(ppa.timeouts), static_cast<unsigned long>(ppa.late), static_cast<unsigned long>(CrowPanelRender::frames()), ppa.ready, static_cast<unsigned long>(ppa.jobs), static_cast<unsigned long>(ppa.fullUs), static_cast<unsigned>(s_drawingPaused), static_cast<unsigned long>(millis()));
                LOG_PLAIN_F("SD Titel=%u Wiedergabe=%lu Datei=%u Bestaetigt=%u Zeit=%lu/%lu Auftrag=%lu", static_cast<unsigned>(s_sd.trackCount()), static_cast<unsigned long>(s_sd.playbackId()), g_audioManager.getStatus().file, g_audioManager.getStatus().fileConfirmed, static_cast<unsigned long>(g_audioManager.getStatus().positionSeconds), static_cast<unsigned long>(g_audioManager.getStatus().durationSeconds), static_cast<unsigned long>(g_audioManager.getStatus().requestId));
                const auto status = g_audioManager.getStatus();
                RadioPreferences preferences;
                s_visualizer.preferences(preferences);
                LOG_PLAIN_F("RADIO Modus=%u Stile=%u%u%u%u%u%u%u%u Balance=%d Klang=%d,%d,%d Auge=%u", preferences.mode, preferences.styles[0], preferences.styles[1], preferences.styles[2], preferences.styles[3], preferences.styles[4], preferences.styles[5], preferences.styles[6], preferences.styles[7], s_balance, s_tone[0], s_tone[1], s_tone[2], s_config.eyeStyle);
                const auto rtc = s_services.rtcStatus();
                LOG_PLAIN_F("RTC Verfuegbar=%u Gueltig=%u Boot=%u Flags=0x%02x RAM-Schreiben=%lu Zeit-Schreiben=%lu Radio-NVS=%lu UTC=%lld System=%lld NTP=%u Wunschpegel=%u Stumm=%u Rueckkehr=%u", rtc.available, rtc.timeValid, rtc.restored, rtc.flags, static_cast<unsigned long>(rtc.ramWrites), static_cast<unsigned long>(rtc.timeWrites), static_cast<unsigned long>(s_services.radioFlashWrites()), static_cast<long long>(rtc.utc), static_cast<long long>(time(nullptr)), s_services.ntpSynced(), s_volume, s_userMuted, s_beforeMute);
                LOG_PLAIN_F("STATUS Radio=%u Sender=%d Auswahl=%d Ausgabe=%u Pegel=%u PCM=%lu PCMAlter=%lu ms Heap=%u PSRAMfrei=%u", static_cast<unsigned>(status.state), status.stationIndex, s_selectedStation, s_listening, status.volume, static_cast<unsigned long>(status.pcmFrames), static_cast<unsigned long>(status.pcmAgeMs), ESP.getFreeHeap(), ESP.getFreePsram());
                LOG_PLAIN_F("AUDIO Rate=%lu Bitrate=%lu Codec=%u Puffer=%lu Luecken=%lu LueckenMs=%lu I2sLeer=%lu", static_cast<unsigned long>(status.sampleRate), static_cast<unsigned long>(status.bitRate), static_cast<unsigned>(status.codec), static_cast<unsigned long>(status.inputBytes), static_cast<unsigned long>(status.pcmGaps), static_cast<unsigned long>(status.pcmGapMs), static_cast<unsigned long>(status.i2sUnderruns));
#if AUDIO_LITE_TIMING
                const auto &timing = status.timing;
                LOG_PLAIN_F("TIMING Writes=%lu Frames=%lu Timeouts=%lu Zero=%lu WriteMaxUs=%lu GapMaxUs=%lu DecodeCalls=%lu DecodeTotalUs=%lu DecodeMaxUs=%lu PrepareCalls=%lu PrepareTotalUs=%lu PrepareMaxUs=%lu",
                            static_cast<unsigned long>(timing.writeCalls), static_cast<unsigned long>(timing.writeFrames),
                            static_cast<unsigned long>(timing.writeTimeouts), static_cast<unsigned long>(timing.writeZero),
                            static_cast<unsigned long>(timing.writeMaxUs), static_cast<unsigned long>(timing.writeGapMaxUs),
                            static_cast<unsigned long>(timing.decodeCalls), static_cast<unsigned long>(timing.decodeTotalUs), static_cast<unsigned long>(timing.decodeMaxUs),
                            static_cast<unsigned long>(timing.prepareCalls), static_cast<unsigned long>(timing.prepareTotalUs), static_cast<unsigned long>(timing.prepareMaxUs));
#endif
                {
                    bool inserted = false, speaker = false;
                    const bool insertedRead = CrowPanelBoard::readHeadphone(inserted), speakerRead = CrowPanelBoard::readSpeaker(speaker);
                    LOG_PLAIN_F("KOPFHOERER Stecker=%u Gelesen=%d Verstaerker=%d", s_headphone.inserted(), insertedRead ? inserted : -1, speakerRead ? speaker : -1);
                }
                {
                    Settings::Configuration config;
                    const uint64_t revision = s_services.configuration(config);
                    LOG_PLAIN_F("KONFIG Revision=%llu Thema=%u Logs=%u Schleichend=%u", static_cast<unsigned long long>(revision), config.theme, config.serialLogs, config.secondSweep);
                    // Die Kopie enthält auch die WLAN-Passwörter.
                    mbedtls_platform_zeroize(&config, sizeof(config));
                    ModernFarben colors;
                    const bool stored = s_services.modernColors(colors);
                    char differences[400];
                    LOG_PLAIN_F("MODERN Eintrag=%u Abweichend=%s", stored, modernDifferences(colors, differences, sizeof(differences)));
                }
                {
                    const auto ftp = s_ftp.status();
                    const auto owner = s_services.fileLease();
                    const char *lease = owner == FileLease::Owner::Web ? "Web" : "frei";
                    if (owner == FileLease::Owner::Ftp)
                        lease = "FTP";
                    char client[16] = "keiner";
                    if (ftp.client)
                        snprintf(client, sizeof(client), "%u.%u.%u.%u", ftp.clientIp[0], ftp.clientIp[1], ftp.clientIp[2], ftp.clientIp[3]);
                    LOG_PLAIN_F("FTP Schalter=%u Lauscht=%u Adresse=%u.%u.%u.%u:%u Benutzer=%s Passwort=%s Client=%s Angemeldet=%u Transfer=%s Bytes=%llu Sitzungen=%lu Abgewiesen=%lu Fehlanmeldungen=%lu Uebertragungen=%lu Sperre=%s Stack=%lu", ftp.enabled, ftp.listening, ftp.ip[0], ftp.ip[1], ftp.ip[2], ftp.ip[3], ftp.port, FtpProtocol::User, ftp.password ? "gesetzt" : "fehlt", client, ftp.loggedIn, FtpService::transferName(ftp.transfer), static_cast<unsigned long long>(ftp.bytes), static_cast<unsigned long>(ftp.sessions), static_cast<unsigned long>(ftp.rejected), static_cast<unsigned long>(ftp.failedLogins), static_cast<unsigned long>(ftp.transfers), lease, static_cast<unsigned long>(ftp.stackFree));
                }
                LOG_PLAIN_F("HEAP DMAfrei=%u DMABlock=%u DMAminimum=%u InternBlock=%u PSRAMBlock=%u PSRAMminimum=%u", static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)), static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)), static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)), static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)), static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)), static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM)));
                // Der ESP-IDF-Wert zählt Bytes, die seit dem Start nie als Stack benutzt wurden.
                const auto stackReserve = [](const char *name) -> unsigned
                {
                    const TaskHandle_t task = xTaskGetHandle(name);
                    return task ? static_cast<unsigned>(uxTaskGetStackHighWaterMark(task)) : 0;
                };
                LOG_PLAIN_F("STACK Audio=%u Dienste=%u Decoder=%u Logger=%u Web=%u",
                            stackReserve("radio-audio"), stackReserve("app-services"),
                            stackReserve("PeriodicTask"), stackReserve("log-usb"), stackReserve("httpd"));
            }
        }
    }
#endif
}

/**
 * @brief Zuteiler für mbedTLS mit Transportreserve im internen RAM. Die Arduino-Bibliotheken legen mbedTLS sonst ganz in den internen RAM
 * (CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC). Große Blöcke ab 8 KB (Record-Puffer, über die die Entschlüsselung läuft) bleiben intern, solange Platz ist, damit die
 * AES-Zugriffe nicht den PSRAM-Bus der RGB-DMA belasten. Dabei bleibt die Transportreserve des Webportals frei.
 * Kleinere Blöcke und große Blöcke, die diese Reserve verbrauchen würden, gehen in den PSRAM.
 */
static void *mbedtlsCalloc(size_t n, size_t size)
{
    constexpr size_t InternalThreshold = 8192;
    if (n && size > SIZE_MAX / n)
        return nullptr;
    const size_t bytes = n * size;
    constexpr uint32_t InternalCaps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    const size_t available = heap_caps_get_free_size(InternalCaps);
    if (bytes >= InternalThreshold && available >= WebPortal::InternalTransportReserve &&
        bytes <= available - WebPortal::InternalTransportReserve)
    {
        if (void *internal = heap_caps_calloc(n, size, InternalCaps))
        {
            // Zwischen Vorprüfung und Allokation können andere Tasks ebenfalls Speicher belegen.
            if (heap_caps_get_free_size(InternalCaps) >= WebPortal::InternalTransportReserve)
                return internal;
            heap_caps_free(internal);
        }
    }
    return heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

/** @brief Initialisiert die bestätigte Hardware und die Module vor Wiedergabe. */
void setup()
{
    mbedtls_platform_set_calloc_free(mbedtlsCalloc, free);
    Serial.setTxBufferSize(4096);
    Serial.begin(SerialBaud);
    delay(500);
    // Bis die gespeicherte Freigabe gelesen ist, bleibt die Ausgabe aus. Das Lesen selbst schreibt keine Logzeile.
    Logger::init(false, false, 512);
    // Im Entwicklungsbuild mit Befehlseingang laufen die seriellen Logs immer, sonst gilt die gespeicherte Freigabe.
    Logger::getInstance().setEnabled(CROWPANEL_USB_FERNSTEUERUNG || CrowPanelConfig::loadSerialLogs());
    heap_caps_malloc_extmem_enable(InternalMallocLimit);
    LOG_PLAIN_F("\nRetroRadio-Crow | Arduino %s | IDF %s", ESP_ARDUINO_VERSION_STR, esp_get_idf_version());
    // Meldungen unmittelbar vor einem Neustart (Brownout, Watchdog-Vorwarnung) erreichen die Logwarteschlange nicht mehr. Der Grund bleibt hier lesbar.
    const esp_reset_reason_t resetReason = esp_reset_reason();
    LOG_PLAIN_F("RESET Grund=%s (%d)", resetReasonName(resetReason), static_cast<int>(resetReason));
    // Ausgabe stumm und Hintergrundlicht aus, bevor irgendetwas anderes startet.
    CrowPanelBoard::silenceBeforeDisplay();
    const bool psramPassed = CrowPanelBoard::testPsram(PsramTestBytes);
    LOG_INFO_F("SPEICHER Flash=%u PSRAM=%u Test=%s", ESP.getFlashChipSize(), ESP.getPsramSize(), psramPassed ? "OK" : "FEHLER");
    if (!psramPassed || !s_display.init())
    {
        LOG_ERROR("Speicher oder Display nicht bestätigt.");
        return;
    }
    s_display.setRotation(DisplayRotation);
    s_display.setBrightness(backlight(Settings::BrightnessDefault));
    if (s_display.width() != CrowPanelLgfx::Width || s_display.height() != CrowPanelLgfx::Height)
    {
        LOG_ERROR("Unerwartete Displayabmessungen.");
        return;
    }
    LOG_INFO_F("RGB-Bildspeicher: %s", CrowPanelRender::begin(s_display) ? "bereit" : "FEHLER");
    // Der Touch richtet Wire mit 100 kHz ein, die Startsuche und eine DS1307 nutzen denselben Bus.
    s_touchReady = s_touch.begin();
    LOG_INFO_F("TOUCH GT911=%s", s_touchReady ? "bereit" : "FEHLT");
    scanI2cBuses();
    // Messbereich 20..100 °C: der Chip wird im Betrieb wärmer als der vom Arduino-Helfer genutzte Bereich 10..50 °C.
    const temperature_sensor_config_t chipTempConfig = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    if (temperature_sensor_install(&chipTempConfig, &s_chipTemp) != ESP_OK)
        s_chipTemp = nullptr;
    else if (temperature_sensor_enable(s_chipTemp) != ESP_OK)
    {
        temperature_sensor_uninstall(s_chipTemp);
        s_chipTemp = nullptr;
    }
    LOG_INFO_F("CHIPTEMPERATUR Sensor=%s", s_chipTemp ? "bereit" : "FEHLER");
    s_stations.loadStations();
    for (int i = 0; i < s_stations.getStationCount(); ++i)
    {
        Station station;
        if (s_stations.getStation(i, station) && station.preset && station.url[0])
        {
            s_selectedStation = i;
            break;
        }
    }
    // Vor jedem begin(), das Farben in Sprites oder Symbolcaches einbrennt. Ein Wechsel wirkt nach dem Neustart.
    ModernFarben modern;
    const bool modernStored = CrowPanelConfig::loadModern(modern);
    setzeModernFarben(modern);
    const uint8_t theme = CrowPanelConfig::loadTheme();
    setzeThema(theme);
    LOG_INFO_F("THEMA Index=%u Aktiv=%s", theme, themaAktiv().name);
    {
        char differences[400];
        LOG_INFO_F("MODERN Eintrag=%u Abweichend=%s", modernStored, modernDifferences(modern, differences, sizeof(differences)));
    }
    s_sdActions = xQueueCreate(4, sizeof(SdPlayerAction));
    LOG_INFO_F("SPEICHER vor den Ansichten: PSRAM frei=%u Block=%u Intern=%u", static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)), static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)), static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
    const char *failedModule = nullptr;
    {
        // Jede Ansicht legt ihre Puffer einmalig im PSRAM an, das Log nennt den Verbrauch je Ansicht.
        const auto psram = []()
        {
            return static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        };
        const auto step = [&](const char *name, bool ok)
        {
            static unsigned before = psram();
            const unsigned after = psram();
            LOG_INFO_F("SPEICHER %s: %u KB PSRAM", name, (before - after) / 1024);
            before = after;
            if (!ok && !failedModule)
                failedModule = name;
            return ok;
        };
        step("SD-Warteschlange", s_sdActions) && step("Visualizer", s_visualizer.begin(s_display)) && step("MainScreen", s_home.begin(s_display)) && step("Einstellungen", s_settings.begin(s_display)) && step("Sperre", s_access.begin(s_display)) &&
            step("Radio", s_radio.begin(s_display, s_stations, s_visualizer)) && step("SD-Player", s_sd.begin(s_display, s_visualizer, {nullptr, queueSdAction, queueFiles}));
    }
    if (failedModule)
    {
        LOG_ERROR_F("Native Module: %s ohne Puffer oder Schriften. PSRAM frei=%u Block=%u", failedModule, static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)), static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
        return;
    }
    // Kein Aufnahmeplatz: Web- und serieller Screenshot lesen die Zeilen direkt aus dem Bildspeicher, solange das Zeichnen ruht (768 KB PSRAM gespart).
    CrowPanelRender::bindFrameLines(s_display.frameLines());
    // Bei laufenden Boottakten die Ausgabe stummschalten. Sie bleibt stumm, während der Decoder später I2S auf die Quellrate umstellt.
    const bool audioStarted = g_audioManager.begin();
    g_audioManager.setStereoOutput(CrowPanelBoard::StereoSpeakers);
    const uint32_t audioStartedAt = millis();
    while (audioStarted && g_audioManager.getStatus().state == AudioManager::State::Unavailable && millis() - audioStartedAt < AudioStartupTimeoutMs)
        delay(1);
    s_audioAvailable = audioStarted && g_audioManager.getStatus().state == AudioManager::State::Idle;
    s_codecReady = s_audioAvailable && CrowPanelBoard::powerDownRadioCodec();
    LOG_INFO_F("Verstärker vor Quellenstart: %s", s_codecReady ? "aus" : "FEHLER");
    s_servicesReady = s_services.begin(s_selectedStation);
    const uint64_t revision = s_services.configuration(s_config);
    s_settings.setConfiguration(s_config, revision);
    s_home.setSecondSweep(s_config.secondSweep);
    applyScreen(s_config.brightness, s_config.rotation, s_config.autoRotate);
    s_audioSettings = s_services.sound();
    setVolume(s_audioSettings.volume);
    applySound(s_audioSettings);
    s_access.setPinValidator([](const char *pin)
    {
        return s_servicesReady && s_services.validatePin(pin);
    });
    Settings::Capabilities caps;
    caps.save = caps.wifiProfiles = caps.wifiScan = caps.wifiTest = caps.time = caps.manualTime = caps.language = s_servicesReady;
    caps.timeWindows = caps.pinChange = caps.appearance = caps.screen = caps.theme = s_servicesReady;
    caps.sound = caps.mute = s_codecReady;
    caps.maxVolume = AudioManager::MaxVolume;
    caps.restart = true;
    s_settings.setCapabilities(caps);
    s_visualizer.setLanguage(s_config.language);
    s_visualizer.setVuSettings(s_config.vuBallistics, s_config.vuSensitivity);
    s_radio.setEyeStyle(s_config.eyeStyle);
    const auto radioPreferences = s_services.radioPreferences();
    s_visualizer.applyPreferences(radioPreferences);
    s_balance = radioPreferences.balance;
    s_userMuted = radioPreferences.muted;
    s_beforeMute = radioPreferences.beforeMute;
    setVolume(radioPreferences.volume);
    memcpy(s_tone, radioPreferences.tone, 3);
    g_audioManager.setTone(s_tone[0], s_tone[1], s_tone[2]);
    g_audioManager.setBalance(s_balance);
    Station restoredStation;
    if (s_stations.getStation(radioPreferences.station, restoredStation) && restoredStation.url[0])
        s_selectedStation = radioPreferences.station;
    s_firmwareBytes = ESP.getSketchSize();
    s_flashBytes = ESP.getFlashChipSize();
    s_psramBytes = ESP.getPsramSize();
    const auto *app = esp_ota_get_running_partition();
    if (app)
    {
        s_appAddress = app->address;
        s_appCapacity = app->size;
    }
    s_ftpReady = s_servicesReady && s_ftp.begin(s_services);
    LOG_INFO_F("FTP Dienst=%u Schalter=aus", s_ftpReady);
    s_webReady = s_servicesReady && s_web.begin(s_services, s_stations, nullptr, s_ftpReady ? &s_ftp : nullptr);
    LOG_INFO_F("WEB Server=%u App=0x%lx Kapazitaet=%lu Bytes", s_webReady, static_cast<unsigned long>(s_appAddress), static_cast<unsigned long>(s_appCapacity));
    s_home.show();
    if (s_config.pinLock)
        s_access.activate(millis());
    LOG_INFO_F("PINSPERRE Schalter=%u", s_config.pinLock);
    s_ready = true;
    LOG_INFO_F("DISPLAY Panel=%s Breite=%d Höhe=%d Touch=%s Rotation=%u", CrowPanelRender::panelName(s_display), s_display.width(), s_display.height(), s_touchReady ? "vorhanden" : "FEHLT", s_display.getRotation());
    LOG_INFO("BEREIT: h=MainScreen, a=Radio, k=Konfiguration, d=SD, r=Sender starten, n/p=Sender, s=Stopp, m=stumm, v=Visualizer, c=Screenshot, x=Abbruch, ?=Status. Senderwahl startet die Wiedergabe.");
}

/** @brief Einziger Besitzer von Touch/Rendern, blockierende Dienste laufen getrennt. */
void loop()
{
    if (!s_ready)
    {
        delay(20);
        return;
    }
    const uint32_t now = millis();
    const auto &touch = updateTouch();
    // Während einer Bildschirmaufnahme oder der USB-Zeichenpause ruhen Zeichnen und Bedienung, die Anzeige steht still.
    // Audio, Dienste und Webaufträge laufen weiter. Ein Kontakt von vor der Sperre wird abgebrochen,
    // jeder Kontakt während der Sperre bis zum Loslassen übergangen.
#if CROWPANEL_USB_FERNSTEUERUNG
    const bool gfxBlocked = s_web.shotBusy() || s_screenshot.active() || s_drawingPaused;
#else
    const bool gfxBlocked = s_web.shotBusy();
#endif
    if (s_touchSuppressed)
    {
        if (!touch.isPressed())
            s_touchSuppressed = false;
    }
    else if (gfxBlocked)
    {
        if (touch.isPressed() || touch.wasReleased())
        {
            if (!touch.wasPressed())
                handleTouch(SettingsUi::TouchPhase::Cancel, touch.x, touch.y, now);
            s_touchSuppressed = touch.isPressed();
        }
    }
    else if (touch.wasPressed())
        handleTouch(SettingsUi::TouchPhase::Press, touch.x, touch.y, now);
    else if (touch.wasReleased())
        handleTouch(SettingsUi::TouchPhase::Release, touch.x, touch.y, now);
    else if (touch.isPressed())
        handleTouch(SettingsUi::TouchPhase::Move, touch.x, touch.y, now);
    if (!touch.isPressed())
        s_timeWindow.observeTouch(false);
    processSettings();
    processSdAction();
#if CROWPANEL_USB_FERNSTEUERUNG
    handleUsb();
#endif
    processWeb();
    processFtp();
    updateSystemStatus(now);
    updateHeadphone(now);
    updatePlayback(now);
    s_display.startWrite();
    if (gfxBlocked)
        g_audioManager.consumeAudioData();
    else if (s_access.active() || s_timeWindow.sleeping())
    {
        if (!s_timeWindow.sleeping())
            s_access.update(now);
        g_audioManager.consumeAudioData();
    }
    else if (s_screen == Screen::Home)
    {
        s_home.update();
        g_audioManager.consumeAudioData();
    }
    else if (s_screen == Screen::Settings)
    {
        s_settings.update(now);
        g_audioManager.consumeAudioData();
    }
    else if (s_screen == Screen::Radio)
    {
        s_radio.update(now, g_audioManager);
        radioAction(s_radio.takeRequest());
    }
    else
        s_sd.update(now, g_audioManager);
    s_display.endWrite();
    rememberRadioState();
#if CROWPANEL_USB_FERNSTEUERUNG
    if (s_captureRequested)
    {
        s_captureRequested = false;
        if (s_web.shotBusy())
            s_captureTarget->println("\nSS1 ERROR BUSY");
        else
        {
            s_display.waitDMA();
            s_screenshot.capture(s_display, *s_captureTarget, s_captureTarget == &Serial);
        }
    }
    const bool sendingShot = s_screenshot.active();
    s_screenshot.poll();
    if (sendingShot)
        Logger::usbOutputWritten();
#endif
    delay(2);
}

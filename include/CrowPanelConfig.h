#ifndef CROWPANEL_CONFIG_H
#define CROWPANEL_CONFIG_H
#include "Settings.h"
#include "RadioPreferences.h"
#include "Thema.h"
#include "FtpProtocol.h"

/** @brief Versionierter Konfigurationsspeicher im NVS-Namensraum rr_crowpanel, Altprofile bleiben unangetastet. */
namespace CrowPanelConfig
{
    /** @brief Lädt ausschließlich den CRC-geprüften eigenen Radiozustand oder RAM-Vorgaben. */
    RadioPreferences loadRadio();
    /** @brief Speichert geprüfte Radiowerte atomar in einem eigenen NVS-Schlüssel, den Sender zusätzlich als eigenen Schlüssel radio_station im selben Commit. */
    bool saveRadio(const RadioPreferences &value);
    /** @brief Liest den im NVS gesicherten Sender, der ohne RTC den Neustart überdauert. @param station Senderindex 0..69. @return true bei vorhandenem, gültigem Eintrag. */
    bool loadStation(uint8_t &station);

    /** @brief Zusätzliche eigene Einstellungen, die bestehende WLAN-Struktur bleibt unverändert. */
    struct Extras
    {
        Settings::Sound sound;
        char pin[5] = "0000";
    };

    /** @brief Prüft Klanggrenzen und eine vierstellige PIN. @param extras Zusätzliche Einstellungen. @return Gültigkeit. */
    bool valid(const Extras &extras);
    /** @brief Erzeugt ausschließlich RAM-Vorgaben, ohne vorhandene NVS-Daten anzufassen. */
    void defaults(Settings::Configuration &config, Extras &extras);
    /** @brief Prüft vollständig terminierte Felder und Wertebereiche vor der Serialisierung. */
    bool valid(const Settings::Configuration &config);
    /** @brief Lädt den eigenen Datensatz oder reine RAM-Defaults, ohne NVS zu ändern. Die Freigabe der seriellen Logs aus logs_v1 hat Vorrang vor Byte 602 von config_v2. Die PIN-Sperre steht nur in pinLockOn und ist ohne diesen Eintrag ein. Der Sekundenzeiger steht nur in secHand_v1 und schleicht ohne diesen Eintrag. @param report false unterdrückt die Logzeile zum Ergebnis. */
    void load(Settings::Configuration &config, uint64_t &revision, Extras *extras = nullptr, bool report = true);
    /** @brief Liest nur den gespeicherten Themenindex, ohne Logzeile. Ohne gültigen Datensatz 0 (Retro). Für den Start vor dem ersten begin() der Ansichten. */
    uint8_t loadTheme();
    /** @brief Liest nur die gespeicherte Freigabe der seriellen Logs, ohne Logzeile. Ohne gültigen Eintrag aus. Für den Start vor der ersten Logzeile. */
    bool loadSerialLogs();
    /** @brief Schreibt nur die Freigabe der seriellen Logs nach rr_crowpanel/logs_v1, einen einzelnen NVS-Eintrag. config_v2 bleibt unberührt. @return false bei NVS-Fehler. */
    bool saveSerialLogs(bool enabled);
    /** @brief Schreibt nur die PIN-Sperre nach rr_crowpanel/pinLockOn, einen einzelnen NVS-Eintrag. config_v2 und die PIN bleiben unberührt. @return false bei NVS-Fehler. */
    bool savePinLock(bool enabled);
    /** @brief Schreibt nur die Gangart des Sekundenzeigers nach rr_crowpanel/secHand_v1, einen einzelnen NVS-Eintrag. config_v2 bleibt unberührt. @param sweep true schleichend, false Sekundenschritte. @return false bei NVS-Fehler. */
    bool saveSecondHand(bool sweep);
    /** @brief Schreibt Konfiguration, PIN und Klang atomar nach rr_crowpanel/config_v2, die Logfreigabe zusätzlich nach logs_v1, die PIN-Sperre nach pinLockOn und den Sekundenzeiger nach secHand_v1. config_v1 bleibt erhalten. */
    bool save(const Settings::Configuration &config, uint64_t revision, const Extras &extras);

    /** @brief Lädt die Grundfarben von Modern aus rr_crowpanel/modern_v1. Fehlt der Eintrag oder ist er ungültig, gelten die Vorgaben. Gelöscht wird nichts. @param farben Ziel. Grund und Fläche tragen immer ihre Vorgabe. @param report false unterdrückt die Warnung bei einem ungültigen Eintrag. @return true, wenn ein gültiger Eintrag geladen wurde. */
    bool loadModern(ModernFarben &farben, bool report = true);
    /** @brief Prüft, dass jede bearbeitbare Grundfarbe höchstens 0xFFFFFF ist. Grund und Fläche zählen nicht. */
    bool validModern(const ModernFarben &farben);
    /** @brief Schreibt die bearbeitbaren Grundfarben nach rr_crowpanel/modern_v1. Grund und Fläche werden nicht gespeichert, config_v2 bleibt unberührt. @return false bei ungültigen Werten oder NVS-Fehler. */
    bool saveModern(const ModernFarben &farben);
    /** @brief Löscht nur den Schlüssel rr_crowpanel/modern_v1, danach gelten die Vorgaben. Ein fehlender Schlüssel gilt als Erfolg. */
    bool resetModern();

    /** @brief Lädt das FTP-Passwort aus rr_crowpanel/ftp_v1. Gelöscht wird nichts. @param out Ziel, ohne gültigen Eintrag leer. @return true, wenn ein gültiger Eintrag geladen wurde. */
    bool loadFtpPassword(char (&out)[FtpProtocol::PasswordCapacity]);
    /** @brief Schreibt ein FTP-Passwort nach rr_crowpanel/ftp_v1, config_v2 und die Geräte-PIN bleiben unberührt. @return false bei ungültigem Passwort (FtpProtocol::validPassword) oder NVS-Fehler. */
    bool saveFtpPassword(const char *password);
}
#endif

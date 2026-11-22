#ifndef CROWPANEL_BOARD_H
#define CROWPANEL_BOARD_H

#include <cstddef>
#include <cstdint>

/** @brief Platinennahe Pins und Startfolge des Elecrow CrowPanel 7.0" ESP32-S3 (DIS08070H, ESP32-S3-WROOM-1-N4R8) laut Herstellerwiki. Ton über einen I2S-Verstärker NS4168 ohne Codec und ohne schaltbare Freigabe, Stummschalten deshalb über Nullsamples vor der I2S-Ausgabe. Ein Lautsprecheranschluss, also Monomischung. */
namespace CrowPanelBoard
{
    constexpr uint8_t I2cSda = 19;              ///< I2C-Bus mit GT911, ab V3.0 PCA9557, optionaler DS1307 am I2C-Stecker (HY2.0)
    constexpr uint8_t I2cScl = 20;
    constexpr uint32_t I2cFrequency = 100000;   ///< eine DS1307 verträgt nicht mehr
    constexpr int8_t I2sMclk = -1;              ///< kein MCLK, der NS4168 braucht keinen
    constexpr uint8_t I2sBclk = 42;             ///< I2S-Bittakt (GPIO)
    constexpr uint8_t I2sLrck = 18;             ///< I2S-Wortauswahl (GPIO)
    constexpr uint8_t I2sData = 17;             ///< I2S-Daten zum NS4168 (GPIO)
    constexpr bool StereoSpeakers = false;      ///< Ein Lautsprecheranschluss (SPK), beide Kanäle bekommen dieselbe Mischung
    constexpr bool SwapChannels = false;        ///< Ohne zweiten Lautsprecher gibt es nichts zu tauschen
    constexpr uint8_t BacklightPin = 2;         ///< PWM des Hintergrundlichts
    constexpr uint8_t LcdVsyncPin = 40;         ///< VSYNC des RGB-Panels, der Eingang am selben Pin liefert die Bildwechselmeldung
    constexpr uint8_t SdSck = 12;               ///< SD-Karte über SPI (FSPI)
    constexpr uint8_t SdMiso = 13;
    constexpr uint8_t SdMosi = 11;
    constexpr uint8_t SdCs = 10;
    constexpr uint32_t SdFrequency = 20000000;  ///< SPI-Takt der SD-Karte in Hz
    constexpr uint8_t Pca9557Address = 0x18;    ///< IO-Expander ab Platinenstand V3.0: IO0 Touch-Reset, IO1 Touch-INT

    /** @brief Erster Schritt nach dem Reset: Tonausgabe stumm und Hintergrundlicht aus, bevor das Display startet. @return true. */
    bool silenceBeforeDisplay();
    /** @brief Prüft einen eigenen PSRAM-Puffer mit zwei Mustern und gibt ihn wieder frei. @param bytes Größe des Prüfpuffers in Bytes, durch vier teilbar. @return true bei vollständig erfolgreicher Prüfung. */
    bool testPsram(size_t bytes);
    /** @brief Ohne Codec nur: Ausgabe stumm. @return true. */
    bool prepareRadioCodec();
    /** @brief Ausgabe stumm, bevor die Quelle ihre Takte ändert oder stoppt. @return true. */
    bool powerDownRadioCodec();
    /** @brief Ausgabe stumm, I2S läuft weiter. @return true. */
    bool muteRadio();
    /** @brief Gibt die Ausgabe frei. Der Aufrufer muss frisches, leises PCM sicherstellen. @param speaker false lässt sie stumm. @return true. */
    bool unmuteRadio(bool speaker);
    /** @brief Schaltet die Ausgabe: Der NS4168 hat keine Freigabeleitung, deshalb ersetzt der Audioworker bei „aus“ jedes Sample vor der I2S-Ausgabe durch 0. @return true. */
    bool setSpeaker(bool on);
    /** @brief Liest den gesetzten Ausgabezustand zurück. @param on true bei freigegebener Ausgabe. @return true. */
    bool readSpeaker(bool &on);
    /** @brief Ausgabezustand ohne Sperre, aus jedem Task, auch aus dem I2S-Pfad. @return true bei freigegebener Ausgabe. */
    bool speakerOn();
    /** @brief Das CrowPanel hat keine Kopfhörerbuchse. @param inserted immer false. @return true. */
    bool readHeadphone(bool &inserted);

    constexpr uint32_t HeadphonePollMs = 50;       ///< Abstand der Abfragen im UI-Task
    constexpr uint8_t HeadphoneStableReads = 3;    ///< So viele gleiche Abfragen in Folge übernehmen einen neuen Steckerzustand

    /** @brief Entprellung der Kopfhörererkennung. Ohne Buchse bleibt der Zustand „kein Stecker“, der Ablauf in main.cpp bleibt dadurch gleich. */
    class HeadphoneDebounce
    {
    public:
        /** @brief Prüft, ob die nächste Abfrage fällig ist. Überlaufsicher. @param now millis(). */
        bool due(uint32_t now) const
        {
            return static_cast<int32_t>(now - m_nextPoll) >= 0;
        }

        /** @brief Übernimmt eine Abfrage und plant die nächste. Ein abweichender Zustand gilt erst nach HeadphoneStableReads gleichen Abfragen in Folge. @param now millis(). @param read Lesen bestätigt. @param inserted Gelesener Steckerzustand. @return true, wenn sich der entprellte Zustand geändert hat. */
        bool update(uint32_t now, bool read, bool inserted)
        {
            m_nextPoll = now + HeadphonePollMs;
            if (!read || inserted == m_inserted)
            {
                m_count = 0;
                return false;
            }
            if (++m_count < HeadphoneStableReads)
                return false;
            m_count = 0;
            m_inserted = inserted;
            return true;
        }

        /** @brief Entprellter Steckerzustand. */
        bool inserted() const
        {
            return m_inserted;
        }

    private:
        uint32_t m_nextPoll = 0; ///< millis() der nächsten fälligen Abfrage
        uint8_t m_count = 0;     ///< Gleiche abweichende Abfragen in Folge
        bool m_inserted = false; ///< Entprellter Zustand, true bei gestecktem Stecker
    };
}

#endif

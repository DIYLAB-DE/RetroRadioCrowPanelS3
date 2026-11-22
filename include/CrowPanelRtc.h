#ifndef CROWPANEL_RTC_H
#define CROWPANEL_RTC_H
#include "RadioPreferences.h"
#include <ctime>
#include <stdint.h>

/** @brief Optionale externe DS1307 an I2C (Wire, 100 kHz). Die Uhr enthält Ortszeit, Radiowerte werden in den NVRAM-Bytes 0 bis 20 gespeichert. Ohne DS1307 ist available false, dann speichert AppServices die Radiowerte im NVS. I/O nur bei Dienststart und im Dienstworker. */
class CrowPanelRtc
{
public:
    /** @brief Befund der RTC für die Diagnose. */
    struct Status
    {
        bool available = false, timeValid = false, restored = false; ///< DS1307 erreichbar, Zeit plausibel, Systemzeit aus der RTC gesetzt
        uint8_t flags = 0;                                          ///< Bit 0: Uhr war angehalten (CH) und wurde gestartet
        uint32_t ramWrites = 0, timeWrites = 0;                     ///< Bestätigte Schreibvorgänge in NVRAM und Uhrzeit
        time_t utc = 0;                                             ///< Zuletzt gelesene RTC-Zeit als Epoche
    };

    static constexpr uint8_t Address = 0x68;     ///< DS1307
    static constexpr uint8_t NvramBytes = 21;    ///< Bytes 0 bis 16, 17 bis 20 eigene Erweiterung für vier weitere Stile

    /** @brief Sucht die DS1307 mit fünf Versuchen und startet eine angehaltene Uhr. @return DS1307 erreichbar. */
    bool begin();
    /** @brief Übernimmt die Ortszeit der RTC als Systemzeit, wenn sie plausibel ist (Jahr ab 2024, Uhr läuft). Setzt eine gesetzte Zeitzone voraus. @return Systemzeit gesetzt. */
    bool restoreSystemTime();
    /** @brief Schreibt die gültige Systemzeit als Ortszeit in die RTC. @return Schreiben bestätigt. */
    bool syncFromSystem();
    /** @brief Übernimmt die Radiowerte aus dem NVRAM, wenn die Kennung ('E', 1) passt. Sonst bleiben die NVS-Werte, Pegel und Rückkehrpegel höchstens quietVolume. @return NVRAM gültig. */
    bool loadRadio(RadioPreferences &value, uint8_t quietVolume);
    /** @brief Schreibt nur veränderte NVRAM-Bytes und die Kennung. Bytes, die diese Firmware nicht nutzt (Mono, Aufgaben), bleiben unverändert. @return Daten unverändert oder Schreiben bestätigt. */
    bool saveRadio(const RadioPreferences &value);

    /** @brief Kopiert den zuletzt im Dienst ermittelten Befund ohne I2C. */
    Status status() const
    {
        return m_status;
    }

private:
    Status m_status;
    bool m_haveNvram = false;          ///< m_nvram spiegelt den Inhalt der DS1307
    uint8_t m_nvram[NvramBytes]{};     ///< zuletzt gelesene oder geschriebene NVRAM-Bytes

    /** @brief Liest count Register ab reg. @return true bei vollständiger Antwort. */
    bool read(uint8_t reg, uint8_t *bytes, uint8_t count);
    /** @brief Schreibt count Register ab reg. @return true bei bestätigter Übertragung. */
    bool write(uint8_t reg, const uint8_t *bytes, uint8_t count);
    /** @brief Liest die Uhr als lokale Kalenderzeit. @param halted Erhält das CH-Bit. @return true bei gelesener Uhr. */
    bool readClock(tm &local, bool &halted);
};
#endif

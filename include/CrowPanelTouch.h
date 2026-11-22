#ifndef CROWPANEL_TOUCH_H
#define CROWPANEL_TOUCH_H

#include <stddef.h>
#include <stdint.h>

/** @brief Touchcontroller GT911 des CrowPanel 7.0" ESP32-S3 an I2C IO19/IO20 (Wire, 100 kHz). Ab Platinenstand V3.0 führt ein PCA9557 Reset (IO0) und INT (IO1) des GT911, ohne ihn startet der GT911 von selbst an 0x5D oder 0x14. Abfrage ohne Interrupt. Nur aus einem Task verwenden. */
class CrowPanelTouch
{
public:
    static constexpr int MaxPoints = 5; ///< Höchstzahl gleichzeitiger Punkte des GT911

    /** @brief Ein Berührungspunkt in Panelkoordinaten. */
    struct Point
    {
        int16_t x; ///< 0 bis 799, links nach rechts
        int16_t y; ///< 0 bis 479, oben nach unten
    };

    /** @brief Startet Wire, setzt den GT911 über einen vorhandenen PCA9557 zurück (Adresse 0x5D), sucht ihn sonst an 0x5D und 0x14 und liest Kennung, Konfigurationsversion und Auflösung ins Log. @return true, wenn die Kennung „911“ lautet und die Konfigurationsversion nicht 0 ist. */
    bool begin();

    /** @brief Fragt neue Punkte ab und quittiert sie. @return Anzahl der Punkte (0 = losgelassen), -2 ohne neue Daten, -1 bei I2C-Fehler. */
    int read(Point *points, int maxPoints);

    /** @brief Ob beim Start ein PCA9557 (Platinenstand V3.0) gefunden wurde. */
    bool expander() const
    {
        return m_expander;
    }

private:
    /** @brief Liest len Byte ab dem 16-Bit-Register reg. */
    bool readRegister(uint16_t reg, uint8_t *data, size_t len);

    /** @brief Schreibt ein Byte in das 16-Bit-Register reg. */
    bool writeRegister(uint16_t reg, uint8_t value);

    /** @brief Resetfolge des GT911 über den PCA9557: Reset und INT Low, Reset High, dann INT als Eingang. Wählt Adresse 0x5D. @return false ohne PCA9557. */
    bool resetThroughExpander();

    uint8_t m_address = 0x5D; ///< 7-Bit-I2C-Adresse des GT911
    bool m_ready = false;     ///< begin() erfolgreich
    bool m_expander = false;  ///< PCA9557 vorhanden
};

#endif

#ifndef CROWPANEL_LGFX_H
#define CROWPANEL_LGFX_H

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

/** @brief Panel_RGB mit Lesezugriff auf die Zeilenzeiger des Bildspeichers für Bildschirmaufnahmen. */
class CrowPanelPanelRgb : public lgfx::Panel_RGB
{
public:
    /** @brief Zeilenzeiger des Bildspeichers in Panellage, RGB565 in LovyanGFX-Bytefolge. @return nullptr vor init(). */
    uint8_t *const *frameLines() const
    {
        return _lines_buffer;
    }
};

/** @brief Display des CrowPanel 7.0" ESP32-S3: RGB-Parallelbus mit 16 Datenleitungen (RGB565) am LCD_CAM des S3, Treiber EK9716BD3 und EK73002ACGB ohne Befehlsschnittstelle, Bildspeicher im PSRAM, Austastlücken am Minimum des Datenblatts EK9716, Pixeltakt 12 MHz. Hintergrundlicht PWM an IO2. Kein Touch hier, der GT911 läuft über CrowPanelTouch und Wire. */
class CrowPanelLgfx : public lgfx::LGFX_Device
{
public:
    static constexpr int Width = 800;  ///< Pixel waagerecht, das Panel liegt nativ quer
    static constexpr int Height = 480; ///< Pixel senkrecht

    /** @brief Stellt Bus, Panel und Hintergrundlicht ein, init() startet sie. */
    CrowPanelLgfx();

    /** @brief Zeilenzeiger des Bildspeichers in Panellage. @return nullptr vor init(). */
    uint8_t *const *frameLines() const
    {
        return m_panel.frameLines();
    }

private:
    lgfx::Bus_RGB m_bus;
    CrowPanelPanelRgb m_panel;
    lgfx::Light_PWM m_light;
};

#endif

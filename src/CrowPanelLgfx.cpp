#include "CrowPanelLgfx.h"

#include "CrowPanelBoard.h"

namespace
{
    constexpr uint32_t PixelClockHz = 12000000; ///< 25,6 Bilder je Sekunde bei 908 x 517 Takten. Weniger Spitzenlast für den gemeinsamen PSRAM-Bus, bei 13 MHz zuckt das Bild unter HTTPS und Zeichnen gelegentlich
    constexpr uint32_t BacklightHz = 44100;     ///< PWM-Frequenz des Hintergrundlichts, über dem Hörbereich
    constexpr int BacklightChannel = 7;          ///< LEDC-Kanal, frei von anderen Nutzern

    // RGB565 an den Datenleitungen D0 bis D15: B0 bis B4, G0 bis G5, R0 bis R4 (Herstellerwiki).
    constexpr int8_t DataPins[16] = {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45};
    constexpr int8_t PinDe = 41;
    constexpr int8_t PinVsync = CrowPanelBoard::LcdVsyncPin;
    constexpr int8_t PinHsync = 39;
    constexpr int8_t PinPclk = 0;
}

CrowPanelLgfx::CrowPanelLgfx()
{
    {
        auto config = m_panel.config();
        config.memory_width = Width;
        config.memory_height = Height;
        config.panel_width = Width;
        config.panel_height = Height;
        config.offset_x = 0;
        config.offset_y = 0;
        config.offset_rotation = 0;
        m_panel.config(config);
    }
    {
        auto detail = m_panel.config_detail();
        detail.use_psram = 2; // Bildspeicher ganz im PSRAM, intern bleibt Platz für WLAN, I2S und SD
        m_panel.config_detail(detail);
    }
    {
        auto config = m_bus.config();
        config.panel = &m_panel;
        for (int i = 0; i < 16; ++i)
            config.pin_data[i] = DataPins[i];
        config.pin_henable = PinDe;
        config.pin_vsync = PinVsync;
        config.pin_hsync = PinHsync;
        config.pin_pclk = PinPclk;
        config.freq_write = PixelClockHz;
        // Kleinste Zeile und kleinstes Bild laut Datenblatt EK9716 (Tabellen 20 und 21): Zeile 908 Takte (vordere Schwarzschulter mindestens 20, Puls und hintere 88), Bild 517 Zeilen (vordere mindestens 5, Puls und hintere 32). Das Herstellerwiki nennt die typischen 928 x 525. Bei 12 MHz ergeben die kleineren Austastlücken rund 25,6 statt 24,6 Bilder je Sekunde. Der Pixeltakt und damit die Spitzenlast auf dem PSRAM bleiben gleich.
        config.hsync_polarity = 0;
        config.hsync_front_porch = 20;
        config.hsync_pulse_width = 48;
        config.hsync_back_porch = 40;
        // Polarität 0: VSYNC ruht low, der Puls ist high. Seine steigende Flanke ist die Bildwechselmeldung, Puls und hintere Schwarzschulter (32 Zeilen, rund 2,2 ms) bleiben bis zur ersten Bildzeile.
        config.vsync_polarity = 0;
        config.vsync_front_porch = 5;
        config.vsync_pulse_width = 19;
        config.vsync_back_porch = 13;
        config.pclk_active_neg = 1;
        config.de_idle_high = 0;
        config.pclk_idle_high = 0;
        m_bus.config(config);
        m_panel.setBus(&m_bus);
    }
    {
        auto config = m_light.config();
        config.pin_bl = CrowPanelBoard::BacklightPin;
        config.invert = false;
        config.freq = BacklightHz;
        config.pwm_channel = BacklightChannel;
        m_light.config(config);
        m_panel.setLight(&m_light);
    }
    setPanel(&m_panel);
}

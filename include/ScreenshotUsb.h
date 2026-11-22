#ifndef SCREENSHOT_USB_H
#define SCREENSHOT_USB_H

#include <LovyanGFX.hpp>
#include "CrowPanelRender.h"

/** @brief Überträgt eine Displayaufnahme als geprüfte RGB565-Lauflängen über UART0. Gelesen wird zeilenweise direkt aus dem Bildspeicher, der Aufrufer zeichnet während active() nicht. */
class ScreenshotUsb
{
public:
    /** @brief Startet eine Aufnahme im UI-Task. Bis zum Ende von active() darf niemand zeichnen. @param display Aktuelles 800x480-Display, lebt bis zum Ende. @param out Kanal der Anforderung, lebt bis zum Ende. @param uart true für UART0: Pakete gehen dann nur, solange in dessen Sendepuffer Platz für ein ganzes Paket ist. Der CDC-Kanal wartet je Paket höchstens CrowPanelUsb::TxTimeoutMs. */
    void capture(lgfx::LGFX_Device &display, Print &out, bool uart);
    /** @brief Überträgt 2 ms lang Pakete, an UART0 höchstens vier. Bricht eine Übertragung nach 120 s oder bei einem Schreibfehler ab. Aus dem UI-Task aufrufen. */
    void poll();

    /** @brief Bricht ausschließlich die laufende Bildübertragung ab. */
    void cancel()
    {
        m_active = false;
    }

    /** @brief Meldet eine laufende Übertragung, während der das Bild nicht verändert werden darf. @return true bis zum Ende oder Abbruch. */
    bool active() const
    {
        return m_active;
    }

private:
    static constexpr uint32_t Width = CrowPanelRender::PanelWidth, Height = CrowPanelRender::PanelHeight, Pixels = Width * Height;
    /** @brief Liefert Pixel position der Aufnahme in Zeilenfolge über einen Zeilenpuffer. */
    uint16_t pixel(uint32_t position);
    lgfx::LGFX_Device *m_display = nullptr;            ///< Display der laufenden Aufnahme
    uint16_t m_row[Width]{};                           ///< Zuletzt gelesene Bildzeile, RGB565 mit dem niederwertigen Byte zuerst
    int m_rowY = -1;                                   ///< Nummer der Zeile in m_row, -1 ohne
    Print *m_out = nullptr;                            ///< Kanal der laufenden Übertragung
    bool m_uart = true;                                ///< Kanal ist UART0 mit Flusskontrolle über den Sendepuffer
    uint32_t m_id = 0, m_position = 0, m_started = 0;  ///< Aufnahmenummer, nächstes Pixel der Übertragung, Startzeit in ms
    bool m_active = false, m_header = false;           ///< Übertragung läuft, Kopfzeile fehlt noch
};

#endif

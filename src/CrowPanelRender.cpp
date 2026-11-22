#include "CrowPanelRender.h"

#include "CrowPanelBoard.h"

#include <driver/gpio.h>
#include <esp_cache.h>
#include <esp_attr.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace
{
    uint32_t s_fullUs = 0;                 ///< Dauer der letzten Vollbildübertragung in µs
    uint8_t *const *s_lines = nullptr;     ///< Zeilenzeiger des Bildspeichers in Panellage
    volatile uint32_t s_frames = 0;        ///< Bildwechselmeldungen seit dem Start
    volatile uint32_t s_lastUs = 0;        ///< Zeitpunkt der letzten Meldung in µs
    volatile uint32_t s_lastMs = 0;        ///< Derselbe Zeitpunkt in ms wie millis()
    volatile uint32_t s_periodUs = 0;      ///< Abstand der letzten beiden Meldungen in µs
    SemaphoreHandle_t s_frameSignal = nullptr; ///< Weckt waitFrame()

    /** @brief Steigende Flanke von VSYNC: Bei Polarität 0 ruht die Leitung low, der Puls ist high. Er beginnt eine Zeile nach dem letzten Bildpixel, bis zur ersten Zeile des nächsten Bildes bleiben Puls und hintere Schwarzschulter, 32 Zeilen. */
    void IRAM_ATTR onVsync(void *)
    {
        const int64_t now = esp_timer_get_time();
        const uint32_t us = static_cast<uint32_t>(now);
        if (s_frames)
            s_periodUs = us - s_lastUs;
        s_lastUs = us;
        s_lastMs = static_cast<uint32_t>(now / 1000);
        s_frames = s_frames + 1;
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(s_frameSignal, &woken);
        if (woken)
            portYIELD_FROM_ISR();
    }
}

bool CrowPanelRender::waitFrame(uint32_t timeoutMs)
{
    if (!s_frameSignal || !s_frames)
        return false;
    xSemaphoreTake(s_frameSignal, 0);
    return xSemaphoreTake(s_frameSignal, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

uint32_t CrowPanelRender::lastFrameMs()
{
    return s_lastMs;
}

uint32_t CrowPanelRender::frames()
{
    return s_frames;
}

uint32_t CrowPanelRender::lastFrameUs()
{
    return s_lastUs;
}

uint32_t CrowPanelRender::framePeriodUs()
{
    return s_periodUs;
}

void CrowPanelRender::bindFrameLines(uint8_t *const *lines)
{
    s_lines = lines;
}

bool CrowPanelRender::copyRow(lgfx::LGFX_Device &display, int y, uint16_t *out)
{
    if (!s_lines || !out || y < 0 || y >= PanelHeight)
        return false;
    // In Rotation 2 liegt Bildzeile y in Panelzeile PanelHeight - 1 - y und läuft rückwärts.
    const bool flipped = display.getRotation() == 2;
    const auto *row = reinterpret_cast<const uint16_t *>(s_lines[flipped ? PanelHeight - 1 - y : y]);
    if (!row)
        return false;
    // LovyanGFX hält RGB565 im Bildspeicher mit vertauschten Bytes.
    for (int x = 0; x < PanelWidth; ++x)
        out[x] = __builtin_bswap16(row[flipped ? PanelWidth - 1 - x : x]);
    return true;
}

bool CrowPanelRender::begin(lgfx::LGFX_Device &display)
{
    const bool landscape = display.getRotation() == 0 || display.getRotation() == 2;
    if (!landscape || display.width() != PanelWidth || display.height() != PanelHeight)
        return false;
    if (!s_frameSignal)
    {
        s_frameSignal = xSemaphoreCreateBinary();
        // Nur den Eingang zuschalten: gpio_set_direction oder gpio_output_enable verbänden den Pin wieder mit dem einfachen GPIO-Ausgang und nähmen dem LCD-Controller VSYNC.
        const auto pin = static_cast<gpio_num_t>(CrowPanelBoard::LcdVsyncPin);
        gpio_input_enable(pin);
        gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
        const esp_err_t service = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        if (!s_frameSignal || (service != ESP_OK && service != ESP_ERR_INVALID_STATE) || gpio_isr_handler_add(pin, onVsync, nullptr) != ESP_OK || gpio_intr_enable(pin) != ESP_OK)
            return false;
    }
    return true;
}

void CrowPanelRender::pushSprite(lgfx::LGFX_Device *display, lgfx::LGFX_Sprite &sprite, int x, int y)
{
    if (!display)
        return;
    sprite.pushSprite(display, x, y);
    // Sofort aus dem Cache zurückschreiben statt erst bei endWrite am Ende des Schleifendurchlaufs, sonst liest der RGB-Bus halb alte Zeilen.
    display->display();
}

void CrowPanelRender::pushRegion(lgfx::LGFX_Device &display, const uint16_t *source, int stride, int x, int y, int width, int height)
{
    if (!source || width <= 0 || height <= 0)
        return;
    const int64_t start = esp_timer_get_time();
    // Zusammenhängende Quellen gehen in einem Auftrag, sonst zeilenweise. Die Bytefolge der Sprites gilt als swap565.
    if (stride == width)
        display.pushImage(x, y, width, height, reinterpret_cast<const lgfx::swap565_t *>(source));
    else
        for (int row = 0; row < height; ++row)
            display.pushImage(x, y + row, width, 1, reinterpret_cast<const lgfx::swap565_t *>(source + row * stride));
    // Sofort aus dem Cache zurückschreiben statt erst bei endWrite am Ende des Schleifendurchlaufs, sonst liest der RGB-Bus halb alte Zeilen.
    display.display();
    if (width == PanelWidth && height == PanelHeight)
        s_fullUs = static_cast<uint32_t>(esp_timer_get_time() - start);
}

uint16_t *CrowPanelRender::framebuffer(lgfx::LGFX_Device &display)
{
    const int rotation = display.getRotation();
    if (!s_lines || (rotation != 0 && rotation != 2))
        return nullptr;
    return reinterpret_cast<uint16_t *>(s_lines[0]);
}

void CrowPanelRender::flushFramebuffer(lgfx::LGFX_Device &display, int x, int y, int width, int height)
{
    if (!s_lines || width <= 0 || height <= 0 || x < 0 || y < 0 || x + width > PanelWidth || y + height > PanelHeight)
        return;
    const bool flipped = display.getRotation() == 2;
    const int left = flipped ? PanelWidth - x - width : x;
    for (int row = 0; row < height; ++row)
    {
        const int panelRow = flipped ? PanelHeight - 1 - (y + row) : y + row;
        esp_cache_msync(s_lines[panelRow] + left * sizeof(uint16_t), width * sizeof(uint16_t), ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }
}

void CrowPanelRender::flushPanelSpan(const uint16_t *pixels, int count)
{
    if (pixels && count > 0)
        esp_cache_msync(const_cast<uint16_t *>(pixels), count * sizeof(uint16_t), ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
}

CrowPanelRender::PpaCounters CrowPanelRender::ppaCounters()
{
    return {0, 0, 0, s_fullUs, false};
}

const char *CrowPanelRender::panelName(lgfx::LGFX_Device &display)
{
    return display.width() == PanelWidth || display.height() == PanelWidth ? "EK9716" : "unbekannt";
}

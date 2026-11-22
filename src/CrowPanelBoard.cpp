#include "CrowPanelBoard.h"

#include <atomic>
#include <driver/gpio.h>
#include <esp_heap_caps.h>

namespace
{
    std::atomic<bool> s_speakerOn{false}; ///< Ausgabe freigegeben, liest der I2S-Pfad im Audioworker
}

bool CrowPanelBoard::silenceBeforeDisplay()
{
    s_speakerOn.store(false);
    // Hintergrundlicht aus, bis das erste Bild steht. Erst den Pegel, dann den Ausgang setzen, damit kein Zwischenpegel entsteht.
    const auto backlight = static_cast<gpio_num_t>(BacklightPin);
    gpio_set_level(backlight, 0);
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << BacklightPin;
    config.mode = GPIO_MODE_OUTPUT;
    gpio_config(&config);
    gpio_set_level(backlight, 0);
    return true;
}

bool CrowPanelBoard::testPsram(size_t bytes)
{
    if (bytes == 0 || bytes % sizeof(uint32_t) != 0)
        return false;
    auto buffer = static_cast<volatile uint32_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buffer)
        return false;
    bool passed = true;
    constexpr uint32_t Patterns[] = {0x55aa55aaU, 0xaa55aa55U};
    for (const uint32_t pattern : Patterns)
    {
        for (size_t index = 0; index < bytes / sizeof(uint32_t); ++index)
            buffer[index] = pattern ^ static_cast<uint32_t>(index);
        for (size_t index = 0; index < bytes / sizeof(uint32_t); ++index)
        {
            if (buffer[index] != (pattern ^ static_cast<uint32_t>(index)))
            {
                passed = false;
                break;
            }
        }
        if (!passed)
            break;
    }
    heap_caps_free(const_cast<uint32_t *>(buffer));
    return passed;
}

bool CrowPanelBoard::prepareRadioCodec()
{
    return setSpeaker(false);
}

bool CrowPanelBoard::powerDownRadioCodec()
{
    return setSpeaker(false);
}

bool CrowPanelBoard::muteRadio()
{
    return setSpeaker(false);
}

bool CrowPanelBoard::unmuteRadio(bool speaker)
{
    return setSpeaker(speaker);
}

bool CrowPanelBoard::setSpeaker(bool on)
{
    s_speakerOn.store(on);
    return true;
}

bool CrowPanelBoard::readSpeaker(bool &on)
{
    on = s_speakerOn.load();
    return true;
}

bool CrowPanelBoard::speakerOn()
{
    return s_speakerOn.load(std::memory_order_relaxed);
}

bool CrowPanelBoard::readHeadphone(bool &inserted)
{
    inserted = false;
    return true;
}

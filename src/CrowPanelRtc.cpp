// DS1307-Zugriff und batteriegestützte Radiowerte. Herkunft und Lizenz: docs/LIZENZEN.md.
#include "CrowPanelRtc.h"
#include "CrowPanelBoard.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstring>
#include <sys/time.h>

namespace
{
    constexpr uint8_t RegSeconds = 0x00;
    constexpr uint8_t RegNvram = 0x08;
    constexpr uint8_t ClockHalt = 0x80;
    constexpr uint8_t Hour12 = 0x40;
    constexpr int MinYear = 2024;           ///< darunter gilt die RTC-Zeit als nie gestellt
    constexpr int Attempts = 5;             ///< Suchversuche beim Start
    constexpr uint32_t AttemptPauseMs = 50;

    // NVRAM-Belegung für Sender, Pegel, Klang und Visualisierung
    constexpr uint8_t NvStation = 0, NvVolume = 1, NvBeforeMute = 2, NvBalance = 3, NvToneLow = 4, NvToneMid = 5, NvToneHigh = 6, NvMuted = 7;
    constexpr uint8_t NvMode = 9, NvVuStyle = 10, NvSpectrumStyle = 11, NvWaveformStyle = 12, NvGonioStyle = 13, NvMarker = 15;
    constexpr uint8_t Marker[] = {'E', 1};
    // Weitere Stilwerte hinter der Kennung
    constexpr uint8_t NvAnalogStyle = 17, NvCenteredStyle = 18, NvWaterfallStyle = 19, NvAuroraStyle = 20;
    constexpr int MaxStation = 69, MaxVolume = 100, MinBalance = -16, MaxBalance = 16, MinGain = -12, MaxGain = 12, MaxMode = 7;

    uint8_t bcdToDec(uint8_t value)
    {
        return static_cast<uint8_t>((value >> 4) * 10 + (value & 0x0F));
    }

    uint8_t decToBcd(uint8_t value)
    {
        return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
    }

    int clampInt(int value, int low, int high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    /** @brief Stil-Index (Visualizermodus) zu NVRAM-Adresse. */
    constexpr uint8_t StyleAddress[8] = {NvAnalogStyle, NvVuStyle, NvSpectrumStyle, NvWaveformStyle, NvGonioStyle, NvCenteredStyle, NvWaterfallStyle, NvAuroraStyle};
}

bool CrowPanelRtc::read(uint8_t reg, uint8_t *bytes, uint8_t count)
{
    Wire.beginTransmission(Address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(Address, count) != count)
        return false;
    for (uint8_t i = 0; i < count; ++i)
        bytes[i] = Wire.read();
    return true;
}

bool CrowPanelRtc::write(uint8_t reg, const uint8_t *bytes, uint8_t count)
{
    Wire.beginTransmission(Address);
    Wire.write(reg);
    Wire.write(bytes, count);
    return Wire.endTransmission() == 0;
}

bool CrowPanelRtc::begin()
{
    m_status = Status{};
    Wire.begin(CrowPanelBoard::I2cSda, CrowPanelBoard::I2cScl, CrowPanelBoard::I2cFrequency);
    bool found = false;
    for (int attempt = 0; attempt < Attempts && !found; ++attempt)
    {
        // Erst nur die Adresse senden. Ohne Antwort meldet die I2C-Schicht sonst beim Lesen Fehler ins Log.
        Wire.beginTransmission(Address);
        found = Wire.endTransmission() == 0;
        if (!found)
            delay(AttemptPauseMs);
    }
    if (!found)
        return false;
    uint8_t seconds = 0;
    if (!read(RegSeconds, &seconds, 1))
        return false;
    if (seconds & ClockHalt)
    {
        // Eine angehaltene Uhr starten, die Sekunden bleiben.
        const uint8_t running = seconds & 0x7F;
        if (!write(RegSeconds, &running, 1))
            return false;
        m_status.flags |= 1;
    }
    m_haveNvram = read(RegNvram, m_nvram, NvramBytes);
    m_status.available = m_haveNvram;
    return m_status.available;
}

bool CrowPanelRtc::readClock(tm &local, bool &halted)
{
    uint8_t r[7] = {};
    if (!read(RegSeconds, r, sizeof(r)))
        return false;
    halted = r[0] & ClockHalt;
    local = tm{};
    local.tm_sec = bcdToDec(r[0] & 0x7F);
    local.tm_min = bcdToDec(r[1] & 0x7F);
    if (r[2] & Hour12)
        local.tm_hour = bcdToDec(r[2] & 0x1F) % 12 + ((r[2] & 0x20) ? 12 : 0);
    else
        local.tm_hour = bcdToDec(r[2] & 0x3F);
    local.tm_mday = bcdToDec(r[4] & 0x3F);
    local.tm_mon = bcdToDec(r[5] & 0x1F) - 1;
    local.tm_year = 2000 + bcdToDec(r[6]) - 1900;
    local.tm_isdst = -1;
    return true;
}

bool CrowPanelRtc::restoreSystemTime()
{
    if (!m_status.available)
        return false;
    tm local{};
    bool halted = true;
    if (!readClock(local, halted))
        return false;
    m_status.timeValid = !halted && local.tm_year + 1900 >= MinYear;
    if (!m_status.timeValid)
        return false;
    const time_t epoch = mktime(&local);
    if (epoch == -1)
        return false;
    m_status.utc = epoch;
    timeval tv{epoch, 0};
    m_status.restored = settimeofday(&tv, nullptr) == 0;
    return m_status.restored;
}

bool CrowPanelRtc::syncFromSystem()
{
    if (!m_status.available)
        return false;
    const time_t now = time(nullptr);
    tm local{};
    localtime_r(&now, &local);
    if (local.tm_year + 1900 < MinYear || local.tm_year + 1900 > 2099)
        return false;
    // 24-Stunden-Format, Wochentag nach ISO 8601 (Montag 1), CH-Bit gelöscht.
    const uint8_t r[7] = {decToBcd(local.tm_sec), decToBcd(local.tm_min), decToBcd(local.tm_hour), static_cast<uint8_t>(local.tm_wday == 0 ? 7 : local.tm_wday), decToBcd(local.tm_mday), decToBcd(local.tm_mon + 1), decToBcd(local.tm_year + 1900 - 2000)};
    if (!write(RegSeconds, r, sizeof(r)))
        return false;
    ++m_status.timeWrites;
    m_status.timeValid = true;
    m_status.utc = now;
    return true;
}

bool CrowPanelRtc::loadRadio(RadioPreferences &value, uint8_t quietVolume)
{
    const bool valid = m_status.available && m_haveNvram && m_nvram[NvMarker] == Marker[0] && m_nvram[NvMarker + 1] == Marker[1];
    if (!valid)
    {
        value.volume = value.volume > quietVolume ? quietVolume : value.volume;
        value.beforeMute = value.beforeMute > quietVolume ? quietVolume : value.beforeMute;
        return false;
    }
    value.station = static_cast<uint8_t>(clampInt(m_nvram[NvStation], 0, MaxStation));
    value.volume = static_cast<uint8_t>(clampInt(m_nvram[NvVolume], 0, MaxVolume));
    value.beforeMute = static_cast<uint8_t>(clampInt(m_nvram[NvBeforeMute], 1, MaxVolume));
    value.balance = static_cast<int8_t>(clampInt(static_cast<int8_t>(m_nvram[NvBalance]), MinBalance, MaxBalance));
    value.tone[0] = static_cast<int8_t>(clampInt(static_cast<int8_t>(m_nvram[NvToneLow]), MinGain, MaxGain));
    value.tone[1] = static_cast<int8_t>(clampInt(static_cast<int8_t>(m_nvram[NvToneMid]), MinGain, MaxGain));
    value.tone[2] = static_cast<int8_t>(clampInt(static_cast<int8_t>(m_nvram[NvToneHigh]), MinGain, MaxGain));
    value.muted = m_nvram[NvMuted] != 0;
    value.mode = static_cast<uint8_t>(clampInt(m_nvram[NvMode], 0, MaxMode));
    for (int i = 0; i < 8; ++i)
        value.styles[i] = m_nvram[StyleAddress[i]] ? 1 : 0;
    return true;
}

bool CrowPanelRtc::saveRadio(const RadioPreferences &value)
{
    if (!m_status.available || !m_haveNvram)
        return false;
    uint8_t next[NvramBytes];
    memcpy(next, m_nvram, sizeof(next));
    next[NvStation] = value.station;
    next[NvVolume] = value.volume;
    next[NvBeforeMute] = value.beforeMute;
    next[NvBalance] = static_cast<uint8_t>(value.balance);
    next[NvToneLow] = static_cast<uint8_t>(value.tone[0]);
    next[NvToneMid] = static_cast<uint8_t>(value.tone[1]);
    next[NvToneHigh] = static_cast<uint8_t>(value.tone[2]);
    next[NvMuted] = value.muted ? 1 : 0;
    next[NvMode] = value.mode;
    for (int i = 0; i < 8; ++i)
        next[StyleAddress[i]] = value.styles[i] ? 1 : 0;
    next[NvMarker] = Marker[0];
    next[NvMarker + 1] = Marker[1];
    if (!memcmp(next, m_nvram, sizeof(next)))
        return true;
    // Jedes geänderte Byte einzeln schreiben. Das NVRAM der DS1307 ist batteriegestütztes SRAM ohne Abnutzung.
    for (uint8_t i = 0; i < NvramBytes; ++i)
    {
        if (next[i] == m_nvram[i])
            continue;
        if (!write(static_cast<uint8_t>(RegNvram + i), &next[i], 1))
            return false;
        m_nvram[i] = next[i];
    }
    ++m_status.ramWrites;
    return true;
}

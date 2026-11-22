#include "CrowPanelTouch.h"

#include <Arduino.h>
#include <Wire.h>

#include "CrowPanelBoard.h"

namespace
{
    constexpr uint8_t ADRESSE_HAUPT = 0x5D;         ///< GT911 bei INT Low während des Resets
    constexpr uint8_t ADRESSE_NEBEN = 0x14;         ///< GT911 bei INT High während des Resets
    constexpr uint16_t REG_PRODUKT = 0x8140;        ///< vier Byte ASCII, „911“
    constexpr uint16_t REG_KONFIGURATION = 0x8047;  ///< Konfigurationsversion, 0 = nicht geladen
    constexpr uint16_t REG_AUFLOESUNG = 0x8048;     ///< X und Y je 16 Bit, niederwertiges Byte zuerst
    constexpr uint16_t REG_STATUS = 0x814E;         ///< Bit 7 neue Daten, Bit 0 bis 3 Anzahl der Punkte
    constexpr uint16_t REG_PUNKT1 = 0x814F;         ///< je Punkt 8 Byte: Kennung, X, Y, Größe, frei
    constexpr size_t PUNKT_BYTES = 8;
    constexpr uint8_t PCA_AUSGANG = 0x01;           ///< Ausgangsregister des PCA9557
    constexpr uint8_t PCA_RICHTUNG = 0x03;          ///< Richtungsregister, 1 = Eingang
    constexpr uint8_t PCA_RESET = 0x01;             ///< IO0 führt den Reset des GT911
    constexpr uint8_t PCA_INT = 0x02;               ///< IO1 führt INT des GT911

/** @brief Prüft, ob an der Adresse ein Baustein antwortet. */
    bool antwortet(uint8_t adresse)
    {
        Wire.beginTransmission(adresse);
        return Wire.endTransmission() == 0;
    }

    /** @brief Schreibt ein Register des PCA9557. */
    bool pcaSchreiben(uint8_t reg, uint8_t wert)
    {
        Wire.beginTransmission(CrowPanelBoard::Pca9557Address);
        Wire.write(reg);
        Wire.write(wert);
        return Wire.endTransmission() == 0;
    }

    /** @brief Liest ein Register des PCA9557. */
    bool pcaLesen(uint8_t reg, uint8_t &wert)
    {
        Wire.beginTransmission(CrowPanelBoard::Pca9557Address);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0 || Wire.requestFrom(CrowPanelBoard::Pca9557Address, static_cast<uint8_t>(1)) != 1)
            return false;
        wert = Wire.read();
        return true;
    }
} // namespace

bool CrowPanelTouch::resetThroughExpander()
{
    if (!antwortet(CrowPanelBoard::Pca9557Address))
        return false;
    uint8_t ausgang = 0, richtung = 0xFF;
    if (!pcaLesen(PCA_AUSGANG, ausgang) || !pcaLesen(PCA_RICHTUNG, richtung))
        return false;
    // Folge laut Herstellerwiki V3.0: beide Leitungen Low, 20 ms, Reset High, 100 ms, dann INT als Eingang. INT Low beim Loslassen des Resets wählt 0x5D.
    ausgang &= static_cast<uint8_t>(~(PCA_RESET | PCA_INT));
    pcaSchreiben(PCA_AUSGANG, ausgang);
    richtung &= static_cast<uint8_t>(~(PCA_RESET | PCA_INT));
    pcaSchreiben(PCA_RICHTUNG, richtung);
    delay(20);
    ausgang |= PCA_RESET;
    pcaSchreiben(PCA_AUSGANG, ausgang);
    delay(100);
    richtung |= PCA_INT;
    return pcaSchreiben(PCA_RICHTUNG, richtung);
}

bool CrowPanelTouch::begin()
{
    if (!Wire.begin(CrowPanelBoard::I2cSda, CrowPanelBoard::I2cScl, CrowPanelBoard::I2cFrequency))
    {
        Serial.println("TOUCH I2C=fehlgeschlagen");
        return false;
    }
    m_expander = resetThroughExpander();
    if (m_expander)
        m_address = ADRESSE_HAUPT;
    else
        m_address = antwortet(ADRESSE_HAUPT) ? ADRESSE_HAUPT : ADRESSE_NEBEN;
    uint8_t produkt[4] = {};
    uint8_t version = 0;
    uint8_t aufloesung[4] = {};
    if (!readRegister(REG_PRODUKT, produkt, sizeof(produkt)) || !readRegister(REG_KONFIGURATION, &version, 1) || !readRegister(REG_AUFLOESUNG, aufloesung, sizeof(aufloesung)))
    {
        Serial.printf("TOUCH Adresse=0x%02X PCA9557=%s antwortet=nein\n", m_address, m_expander ? "ja" : "nein");
        return false;
    }
    char kennung[5] = {static_cast<char>(produkt[0]), static_cast<char>(produkt[1]), static_cast<char>(produkt[2]), static_cast<char>(produkt[3]), 0};
    const unsigned breite = aufloesung[0] | (aufloesung[1] << 8);
    const unsigned hoehe = aufloesung[2] | (aufloesung[3] << 8);
    m_ready = produkt[0] == '9' && produkt[1] == '1' && produkt[2] == '1' && version != 0;
    Serial.printf("TOUCH Adresse=0x%02X PCA9557=%s Produkt=%s Konfiguration=%u Aufloesung=%ux%u bereit=%s\n", m_address, m_expander ? "ja" : "nein", kennung, version, breite, hoehe, m_ready ? "ja" : "nein");
    writeRegister(REG_STATUS, 0);
    return m_ready;
}

int CrowPanelTouch::read(Point *points, int maxPoints)
{
    if (!m_ready)
        return -1;
    uint8_t status = 0;
    if (!readRegister(REG_STATUS, &status, 1))
        return -1;
    if ((status & 0x80) == 0)
        return -2;
    int anzahl = status & 0x0F;
    if (anzahl > MaxPoints)
        anzahl = MaxPoints;
    if (anzahl > maxPoints)
        anzahl = maxPoints;
    uint8_t daten[MaxPoints * PUNKT_BYTES] = {};
    if (anzahl > 0 && !readRegister(REG_PUNKT1, daten, anzahl * PUNKT_BYTES))
        return -1;
    for (int i = 0; i < anzahl; ++i)
    {
        const uint8_t *p = daten + i * PUNKT_BYTES;
        points[i].x = static_cast<int16_t>(p[1] | (p[2] << 8));
        points[i].y = static_cast<int16_t>(p[3] | (p[4] << 8));
    }
    writeRegister(REG_STATUS, 0);
    return anzahl;
}

bool CrowPanelTouch::readRegister(uint16_t reg, uint8_t *data, size_t len)
{
    Wire.beginTransmission(m_address);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    if (Wire.endTransmission(false) != 0)
        return false;
    if (Wire.requestFrom(m_address, len) != len)
        return false;
    for (size_t i = 0; i < len; ++i)
        data[i] = Wire.read();
    return true;
}

bool CrowPanelTouch::writeRegister(uint16_t reg, uint8_t value)
{
    Wire.beginTransmission(m_address);
    Wire.write(static_cast<uint8_t>(reg >> 8));
    Wire.write(static_cast<uint8_t>(reg & 0xFF));
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

#ifndef CROWPANEL_RENDER_H
#define CROWPANEL_RENDER_H

#include <LovyanGFX.hpp>

/** @brief Übertragung opaker RGB565-Flächen in den PSRAM-Bildspeicher des RGB-Panels. Das Panel liegt nativ quer, Rotation 0 und 2 übernimmt LovyanGFX beim Schreiben. Die Bildwechselmeldung kommt aus einem GPIO-Interrupt am Beginn des VSYNC-Pulses, 32 Zeilen (rund 2,2 ms) bevor das Panel die erste Zeile des nächsten Bildes liest. */
namespace CrowPanelRender
{
    constexpr int PanelWidth = 800;  ///< Pixel je Zeile des Bildspeichers
    constexpr int PanelHeight = 480; ///< Zeilen des Bildspeichers

    /** @brief Zähler der Diagnosezeile. Am S3 gibt es keine PPA, alle Werte bleiben 0. */
    struct PpaCounters
    {
        uint32_t timeouts; ///< immer 0
        uint32_t late;     ///< immer 0
        uint32_t jobs;     ///< immer 0
        uint32_t fullUs;   ///< Dauer der letzten Vollbildübertragung in µs, 0 vor der ersten
        bool ready;        ///< immer false, die CPU überträgt
    };

    /** @brief Prüft das Display für die Übertragung und richtet die Bildwechselmeldung an VSYNC ein. @param display Initialisiertes Display in Rotation 0 oder 2. @return false bei anderer Größe. */
    bool begin(lgfx::LGFX_Device &display);
    /** @brief Überträgt einen 16-Bit-Sprite unter Beachtung des Display-Clippings. */
    void pushSprite(lgfx::LGFX_Device *display, lgfx::LGFX_Sprite &sprite, int x, int y);
    /** @brief Überträgt einen RGB565-Ausschnitt (LovyanGFX-Byteordnung) an die Position x/y. Nur aus dem UI-Task aufrufen. @param stride Zeilenbreite der Quelle in Pixeln. */
    void pushRegion(lgfx::LGFX_Device &display, const uint16_t *source, int stride, int x, int y, int width, int height);
    /** @brief Liefert die Zähler für die Diagnosezeile. */
    PpaCounters ppaCounters();
    /** @brief Wartet auf die nächste Bildwechselmeldung (Beginn des VSYNC-Pulses). Eine schon vorliegende Meldung zählt nicht. Nur aus dem UI-Task aufrufen. @param timeoutMs Höchstwartezeit in ms. @return false ohne Bildsignal oder nach der Zeitgrenze. */
    bool waitFrame(uint32_t timeoutMs);
    /** @brief Zeitpunkt der letzten Bildwechselmeldung auf der Basis von millis(), 0 ohne Signal. */
    uint32_t lastFrameMs();
    /** @brief Bildwechselmeldungen seit dem Start, 0 heißt, es kommt kein Bildsignal. */
    uint32_t frames();
    /** @brief Zeitpunkt der letzten Bildwechselmeldung in µs (untere 32 Bit von esp_timer), 0 ohne Signal. */
    uint32_t lastFrameUs();
    /** @brief Abstand der letzten beiden Bildwechselmeldungen in µs, 0 vor der zweiten Meldung. */
    uint32_t framePeriodUs();
    /** @brief Der Bildspeicher hält RGB565 mit vertauschten Bytes, dieselbe Folge wie die Sprites. Wer direkt schreibt, vertauscht Farbwerte, kopiert Sprite-Pixel aber unverändert. */
    constexpr bool FramebufferSwapped = true;
    /** @brief Liefert den Bildspeicher (PanelWidth x PanelHeight, Zeilen lückenlos) zum direkten Schreiben, wenn das Display in Rotation 0 oder 2 steht. Bildpixel (x, y) liegt in Rotation 0 in Zeile y, Spalte x, in Rotation 2 in Zeile PanelHeight - 1 - y, Spalte PanelWidth - 1 - x. Sonst nullptr. Nur UI-Task. Nach dem Schreiben flushFramebuffer aufrufen. */
    uint16_t *framebuffer(lgfx::LGFX_Device &display);
    /** @brief Schreibt direkt in den Bildspeicher geschriebene Pixel eines Rechtecks in Bildkoordinaten sofort aus dem CPU-Cache ins PSRAM, nur die betroffenen Zeilenstücke. Erst danach liest sie der RGB-Bus. LovyanGFX selbst schreibt seine Änderungen erst bei display() zurück, am Ende des Schleifendurchlaufs. Nur UI-Task. */
    void flushFramebuffer(lgfx::LGFX_Device &display, int x, int y, int width, int height);
    /** @brief Schreibt einen zusammenhängenden Pixelbereich des Bildspeichers in Panellage sofort aus dem CPU-Cache ins PSRAM. Für Schreiber, die Zeile für Zeile vor dem Lesestrahl bleiben müssen. Nur UI-Task. @param pixels Erstes Pixel im Bildspeicher. @param count Anzahl der Pixel. */
    void flushPanelSpan(const uint16_t *pixels, int count);
    /** @brief Bindet die Zeilenzeiger des Bildspeichers für copyRow. @param lines Zeilenzeiger in Panellage, PanelHeight Einträge. */
    void bindFrameLines(uint8_t *const *lines);
    /** @brief Kopiert die Bildzeile y in Bildkoordinaten (Rotation 0 oder 2) als RGB565 mit dem niederwertigen Byte zuerst nach out. Liest nur den Speicher ohne LovyanGFX-Aufruf und darf deshalb auch aus dem HTTP-Task kommen, solange der UI-Task nicht zeichnet. @param out PanelWidth Pixel. @return false ohne gebundenen Bildspeicher oder bei ungültiger Zeile. */
    bool copyRow(lgfx::LGFX_Device &display, int y, uint16_t *out);
    /** @brief Name des Panels für das Startlog. */
    const char *panelName(lgfx::LGFX_Device &display);
}

#endif

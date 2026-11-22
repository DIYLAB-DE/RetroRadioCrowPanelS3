#ifndef MAINSCREEN_LAYOUT_H
#define MAINSCREEN_LAYOUT_H

/**
 * @brief Geometrie von MainScreen, Sperrbildschirm und Eckengeste in 800x480.
 *
 * Alle Maße gelten für das native Display. Werte der Uhr sind relativ zur linken oberen Ecke des Uhrsprites, Werte der Tasten relativ zum Tastenfeld.
 */
namespace MainScreenLayout
{
    constexpr int WIDTH = 800; ///< Ansicht deckungsgleich mit dem Display
    constexpr int HEIGHT = 480;

    /** @brief Raster der quadratischen Symbole oben rechts, nur die erste Zeile ist belegt. */
    namespace Buttons
    {
        constexpr int EDGE_MARGIN = 22;                                  ///< Abstand zum oberen und rechten Rand
        constexpr int SIZE = 100;
        constexpr int SPACING = 12;
        constexpr int COLS = 3;
        constexpr int ROWS = 4;                                          ///< Bestimmt über GRID_H auch die Uhrgröße
        constexpr int GRID_W = COLS * SIZE + (COLS - 1) * SPACING;
        constexpr int GRID_H = ROWS * SIZE + (ROWS - 1) * SPACING;
        constexpr int START_X = WIDTH - EDGE_MARGIN - GRID_W;
        constexpr int START_Y = EDGE_MARGIN;
        constexpr int STEP_X = SIZE + SPACING;
        constexpr int RADIUS = 10;                                       ///< Ecken
        constexpr int BORDER = 1;                                        ///< Rahmen
        constexpr int BORDER_PRESSED = 2;                                ///< Rahmen beim Drücken
        constexpr int ICON_INSET = 10;
        constexpr float ICON_SCALE = 1.0f;                               ///< Vektorformen in der Bezugsgröße von IconShapes::draw
        constexpr int ICON_SIZE = 80;
    }

    /** @brief Analoguhr mit Datumszeile in einem gemeinsamen Sprite links. */
    namespace Clock
    {
        constexpr int MARK_INSET = 10;                                                 ///< Abstand der Markierungen vom Uhrrand
        constexpr int DATE_H = 36;                                                     ///< Höhe der Datumszeile
        constexpr int DATE_OFFSET = 10;                                                ///< Abstand Uhr -> Datum
        constexpr int DIAMETER = Buttons::GRID_H + MARK_INSET - DATE_OFFSET - DATE_H;
        constexpr float RADIUS = DIAMETER / 2.0f;
        constexpr int SPRITE_W = DIAMETER + 30;
        constexpr int SPRITE_H = DIAMETER + DATE_OFFSET + DATE_H;
        constexpr int SPRITE_X = 12;
        constexpr int SPRITE_Y = Buttons::START_Y - MARK_INSET;
        constexpr int CENTER_X = SPRITE_W / 2;
        constexpr int CENTER_Y = DIAMETER / 2;                                         ///< Oberkante des Kreises bei 0
        constexpr int DATE_Y = DIAMETER + DATE_OFFSET;                                 ///< Oberkante der Datumszeile
        constexpr float MINOR_MARK_W = 1.0f;                                          ///< Minutenstriche
        constexpr float HOUR_MARK_W = 2.0f;                                            ///< Stundenstriche
        constexpr float MAJOR_MARK_W = 3.0f;                                          ///< Viertelstunden sowie NTP und DST
        constexpr float SECOND_TIP = 0.5f;                                           ///< Spitze des Sekundenzeigers
    }

    /** @brief WLAN-Anzeige und gedrehte NTP/DST-Beschriftungen im Zifferblatt. */
    namespace Indicators
    {
        constexpr int SIGNAL_BAR_COUNT = 5;
        constexpr int SIGNAL_BAR_W = 4;
        constexpr int SIGNAL_BAR_GAP = 4;
        constexpr int SIGNAL_BAR_H[SIGNAL_BAR_COUNT] = {6, 11, 16, 21, 26};
        constexpr float SIGNAL_BAR_RADIAL_POS = 0.36f;              ///< Mitte der Balken relativ zum Radius
        constexpr int SSID_OFFSET_Y = 6;                            ///< Abstand Balken -> SSID
        constexpr int IP_OFFSET_Y = 13;                             ///< Abstand SSID-Oberkante -> IP
        constexpr int SSID_MAX_W = 120;
        constexpr int LABEL_W = 42;                                 ///< NTP/DST-Beschriftung
        constexpr int LABEL_H = 18;
        constexpr float LABEL_RADIAL_POS = 0.68f;
    }

    /** @brief Sperrbildschirm mit Logo, Schriftzug und PIN-Tastenfeld. */
    namespace Lock
    {
        constexpr int LOGO_W = 450;                                           ///< Bildgröße (logo_450)
        constexpr int LOGO_H = 263;                                           ///< Bildgröße
        constexpr int MADEIN_W = 212;                                         ///< Bildgröße (made_in_germany)
        constexpr int MADEIN_H = 40;                                          ///< Bildgröße
        constexpr int LOGO_GAP = 29;
        constexpr int LOGO_X = (WIDTH - LOGO_W) / 2;                          ///< mittig
        constexpr int LOGO_Y = (HEIGHT - LOGO_H - LOGO_GAP - MADEIN_H) / 2;   ///< beide Bilder zusammen senkrecht mittig
        constexpr int MADEIN_X = (WIDTH - MADEIN_W) / 2;
        constexpr int MADEIN_Y = LOGO_Y + LOGO_H + LOGO_GAP;
        constexpr int KEY_W = 65;
        constexpr int KEY_H = 70;
        constexpr int KEY_GAP = 6;
        constexpr int KEY_COLS = 2;
        constexpr int KEY_ROWS = 6;
        constexpr int KEY_RADIUS = 4;
        constexpr int KEY_BORDER = 1;
        constexpr int PULSE_FRAME = 3;                                        ///< Pulsrahmen einschließlich Tastenrand
        constexpr int KEYPAD_W = KEY_COLS * KEY_W + (KEY_COLS - 1) * KEY_GAP;
        constexpr int KEYPAD_H = KEY_ROWS * KEY_H + (KEY_ROWS - 1) * KEY_GAP;
        constexpr int KEYPAD_MARGIN = 15;                                     ///< Abstand zum oberen und rechten Rand. Unten bleiben 13
        constexpr int KEYPAD_X = WIDTH - KEYPAD_MARGIN - KEYPAD_W;
        constexpr int KEYPAD_Y = KEYPAD_MARGIN;
    }

    /** @brief Diagonale Sperrgeste an den beiden linken Ecken des Displays. */
    namespace Corner
    {
        constexpr int START_ZONE = 150; ///< Startzonen an beiden linken Ecken
        constexpr int CORNER = 32;      ///< Zielecken
        constexpr int SWIPE = 50;       ///< Mindestweg je Achse
    }
}

#endif

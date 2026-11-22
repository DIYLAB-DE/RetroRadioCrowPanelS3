#ifndef SD_PLAYER_LAYOUT_H
#define SD_PLAYER_LAYOUT_H
#include "RadioLayout.h"

/**
 * @brief Geometrie des SD-Players und seines Dateidialogs in 800x480.
 *
 * Alle Maße gelten für das native Display. Der untere Bereich mit Drehknopf, Reglern, Visualizer, Uhren und Tasten liegt exakt wie beim Radio (RadioLayout).
 */
namespace SdPlayerLayout
{
    namespace RL = RadioLayout;
    constexpr int WIDTH = RL::WIDTH;   ///< Breite der Szene, deckungsgleich mit dem Display
    constexpr int HEIGHT = RL::HEIGHT; ///< Höhe der Szene

    /** @brief Trennlinien, dunkle Spielerfläche und Lücke zur Playlist. */
    namespace Frame
    {
        constexpr int DIVIDER_H = RL::Scale::DIVIDER_HEIGHT;
        constexpr int TOP_DIVIDER_Y = RL::Scale::TOP_DIVIDER_Y;
        constexpr int BOTTOM_DIVIDER_Y = RL::Scale::BOTTOM_DIVIDER_Y;
        constexpr int PANEL_Y = RL::Scale::Y;
        constexpr int PANEL_H = RL::Scale::HEIGHT;
        constexpr int PANEL_W = 540;
        constexpr int GAP_W = 10;                                ///< Schwarze Lücke vor der Playlist
    }

    /** @brief Kassette mit Fortschritt und Transporttasten darunter. */
    namespace Cassette
    {
        constexpr int X = 10;
        constexpr int Y = 15;
        constexpr int W = 285;
        constexpr int H = 184;
        constexpr int WRAP_SPRITE = 48;     ///< Kantenlänge des Naben-Sprites
        constexpr int LABEL_MARGIN = 6;     ///< Textrand im Etikettstreifen
    }

    /** @brief Fortschrittsbalken mit Zeitanzeige. */
    namespace Progress
    {
        constexpr int X = Cassette::X;
        constexpr int Y = 211;
        constexpr int W = Cassette::W;
        constexpr int H = 24;
        constexpr int TOUCH_Y = 203;     ///< Trefferfläche
        constexpr int TOUCH_H = 40;
        constexpr int BAR_LINE_Y = 22;   ///< Rote Linie am unteren Rand
        constexpr int BAR_LINE_H = 2;
        constexpr int TEXT_Y = 12;       ///< Textmitte
        constexpr int CURRENT_X = 92;   ///< Linke Kante der Laufzeit
        constexpr int SLASH_X = 142;     ///< Mitte des Schrägstrichs
        constexpr int END_X = 152;       ///< Linke Kante der Gesamtzeit
    }

    /** @brief Fünf Transporttasten unter der Kassette. */
    namespace Transport
    {
        constexpr int Y = 250;
        constexpr int SIZE = 45;
        constexpr int GAP = 15;
        constexpr int X0 = Cassette::X;
        constexpr int STEP = SIZE + GAP;
        constexpr int RADIUS = RL::Buttons::RADIUS;
        constexpr int BORDER = RL::Buttons::BORDER;
        constexpr int BORDER_PRESSED = RL::Buttons::BORDER_PRESSED;
    }

    /** @brief Cover und Infofeld darunter, bündig mit dem Ende der Spielerfläche. */
    namespace Cover
    {
        constexpr int W = 230;
        constexpr int H = 220;
        constexpr int X = Frame::PANEL_W - W;
        constexpr int Y = Cassette::Y;
        constexpr int LINE = 1;                   ///< Seitenlinien
        constexpr int INITIALS_XY = 15;           ///< Initialen
        constexpr int TEXT_MARGIN = 15;           ///< Abstand der Textzeile vom Rand
        constexpr int DETAILS_Y = Transport::Y;   ///< Infofeld
        constexpr int DETAILS_H = Transport::SIZE;
    }

    /** @brief Infofeld unter dem Cover mit Kapazität, Album, Titelnummer und Formatmarken. */
    namespace Details
    {
        constexpr int TEXT_X = 8;
        constexpr int LINE1_Y = 6;
        constexpr int LINE2_Y = 26;
        constexpr int TEXT_W = 214;
        constexpr int TEXT_W_PLAYING = 212;
        constexpr int BADGE_Y = 23;
        constexpr int BADGE_H = 18;
        constexpr int BADGE_PAD = 8;
        constexpr int BADGE_GAP = 2;
        constexpr int BADGE_RIGHT = 5;    ///< Abstand zum rechten Rand
        constexpr int TRACK_GAP = 5;      ///< Abstand der Titelnummer zu den Marken
    }

    /** @brief Playlist rechts mit Scrollbalken am Displayrand. */
    namespace Playlist
    {
        constexpr int SCROLLBAR_W = 5;
        constexpr int X = Frame::PANEL_W + Frame::GAP_W;
        constexpr int Y = Frame::PANEL_Y;
        constexpr int W = WIDTH - SCROLLBAR_W - X;
        constexpr int H = Frame::PANEL_H;
        constexpr int ROW_H = 27;
        constexpr int SCROLLBAR_X = WIDTH - SCROLLBAR_W;
        constexpr int SCROLLBAR_MIN_H = 20;
        constexpr int NUMBER_X = 10;
        constexpr int NAME_GAP = 15;                                     ///< Abstand nach "999."
        constexpr int NAME_MARGIN = 10;                                  ///< Rechter Textrand
        constexpr int OVERSCROLL = 50;                                   ///< Gedämpfter Überlauf beim Ziehen
        constexpr int DRAG_THRESHOLD = 10;                               ///< Ab hier gilt der Kontakt als Ziehen
        constexpr int EMPTY_ICON_SCALE = 4;                              ///< Ordnersymbol
        constexpr int EMPTY_ICON_Y = 78;
        constexpr int EMPTY_TITLE_Y = 168;
        constexpr int EMPTY_HINT1_Y = 196;
        constexpr int EMPTY_HINT2_Y = 216;
        constexpr int EMPTY_HINT_MARGIN = 10;
    }

    /** @brief Trefferfläche für Wisch- und Haltegesten auf dem Visualizer, ohne Uhren- und Tastenzeile. */
    namespace Visualizer
    {
        constexpr int TOUCH_H = 71;
    }

    /** @brief Statuszeile wie die Infozeile des Radios. */
    namespace Footer
    {
        constexpr int Y = RL::Footer::Y;
        constexpr int HEIGHT = RL::Footer::HEIGHT;
        constexpr int LINE = RL::Footer::LINE;
    }

    /** @brief Vier Symboltasten rechts unten im Raster der Radiotasten X1 bis X4, alle so breit wie das Haus. */
    namespace Buttons
    {
        constexpr int Y = RL::Buttons::Y;
        constexpr int H = RL::Buttons::H;
        constexpr int W = RL::Buttons::W;
        constexpr int POWER_X = RL::Buttons::X4;  ///< 755
        constexpr int RESET_X = RL::Buttons::X3;  ///< Symboltaste im gemeinsamen Tastenraster
        constexpr int CLEAR_X = RL::Buttons::X2;  ///< Symboltaste zum Leeren der Titelliste
        constexpr int BROWSE_X = RL::Buttons::X1; ///< Symboltaste zum Öffnen des Dateidialogs
    }

    /** @brief Dateidialog mittig. Alle Innenmaße relativ zur linken oberen Ecke. */
    namespace Explorer
    {
        constexpr int W = 550;
        constexpr int H = 350;
        constexpr int X = (WIDTH - W + 1) / 2;
        constexpr int Y = (HEIGHT - H + 1) / 2;
        constexpr int BORDER = 1;
        constexpr int RADIUS = 4;
        constexpr int TEXT_X = 15;                ///< Titel, Pfad, Linien und Status
        constexpr int TITLE_Y = 15;
        constexpr int PATH_Y = 35;
        constexpr int PATH_W = 520;
        constexpr int LINE_W = 520;
        constexpr int LINE_H = 1;
        constexpr int LINE1_Y = 60;
        constexpr int LINE2_Y = 92;
        constexpr int HEADER_Y = 77;             ///< Mitte der Spaltenköpfe
        constexpr int HEADER_NAME_X = 18;
        constexpr int HEADER_TYPE_X = 386;
        constexpr int HEADER_SIZE_X = 526;
        constexpr int LIST_X = 3;
        constexpr int LIST_Y = 94;
        constexpr int LIST_WIDTH = 538;
        constexpr int LIST_HEIGHT = 191;
        constexpr int ROW_H = 31;
        constexpr int ROW_TEXT_Y = 15;
        constexpr int ROW_LEFT = 15;              ///< Linker Rand der Zeile
        constexpr int ICON_GAP = 24;              ///< Textversatz nach Ordnersymbol
        constexpr int ICON_Y = 6;                 ///< Ordnersymbol
        constexpr int ICON_UP_Y = 9;             ///< Symbol für den übergeordneten Ordner
        constexpr float ICON_SCALE = 1.0f;         ///< Faktor für IconShapes::draw, nativ in der Bezugsgröße
        constexpr int TYPE_X = 383;               ///< Mitte der Typspalte in der Liste
        constexpr int SIZE_RIGHT = 523;           ///< Rechte Kante der Größenspalte
        constexpr int NAME_RESERVE = 50;          ///< Abstand des Namens vor der Typspalte
        constexpr int SCROLLBAR_X = 542;
        constexpr int SCROLLBAR_W = 5;
        constexpr int SCROLLBAR_MIN_H = 20;
        constexpr int STATUS1_Y = 292;
        constexpr int STATUS2_Y = 310;
        constexpr int BUTTON_W = 90;
        constexpr int BUTTON_H = 45;
        constexpr int OK_CX = 385;                ///< Mitte "ok"
        constexpr int CANCEL_CX = 490;            ///< Mitte "abbruch"
        constexpr int BUTTON_CY = 313;
        constexpr int DRAG_THRESHOLD = 10;
        constexpr int OVERSCROLL = 80;
    }
}
#endif

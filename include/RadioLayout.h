#ifndef RADIO_LAYOUT_H
#define RADIO_LAYOUT_H
#include "StationManager.h"

/**
 * @brief Geometrie des Radios in 800x480.
 *
 * Alle Maße gelten für das native Display. Visualizer, Uhren und Tasten liegen im rechts unten bündigen Bereich Visual, der Visualizer rechnet dort in Basiskoordinaten (VisualizerGeometry mit Faktor 1).
 */
namespace RadioLayout
{
    constexpr int WIDTH = 800; ///< Breite der Radioszene, deckungsgleich mit dem Display
    constexpr int HEIGHT = 480; ///< Höhe der Radioszene

    /** @brief Senderskala, Trennlinien, Logo und Zeiger. */
    namespace Scale
    {
        constexpr int DIVIDER_HEIGHT = 3;
        constexpr int TOP_DIVIDER_Y = 0;
        constexpr int Y = 5;                  ///< Oberkante des Skalensprites
        constexpr int HEIGHT = 300;
        constexpr int BOTTOM_DIVIDER_Y = 307;
        constexpr int STATION_PITCH_NUM = 10; ///< Senderabstand als Bruch, 10/1 = 10 px
        constexpr int STATION_PITCH_DEN = 1;  ///< Nenner des Senderabstands

        /** @brief X-Position des Senders index, auf ganze Pixel gerundet. 10 + 10 x index. */
        constexpr int stationX(int index)
        {
            return ((index + 1) * STATION_PITCH_NUM * 2 + STATION_PITCH_DEN) / (2 * STATION_PITCH_DEN);
        }

        /** @brief Sender unter der Skalenspalte x. Es gehört zu jedem Sender der Streifen links von seiner Position, einen Senderabstand breit. */
        constexpr int stationAt(int x)
        {
            return x * STATION_PITCH_DEN / STATION_PITCH_NUM;
        }

        constexpr int STATION_X0 = stationX(0); ///< X-Position des ersten Senders
        constexpr int STATION_Y0 = 102;       ///< Zeile des ersten Senders
        constexpr int STATION_STEP_Y = 20;    ///< Zeilenabstand
        constexpr int ROW_WRAP_Y = 300;       ///< Zeilen darüber beginnen oben neu
        constexpr int ROW_RESTART_Y = 2;      ///< Erste Zeile nach dem Umbruch
        constexpr int MARKER_X = -4;          ///< Markerversatz zur Senderposition. Mittig unter dem Zeiger
        constexpr int MARKER_Y = 5;
        constexpr int MARKER_W = 14;
        constexpr int MARKER_H = 6;
        constexpr int NAME_X = 16;            ///< Textversatz zur Senderposition
        constexpr int NAME_Y = 0;
        constexpr int UNDERLINE_H = 1;        ///< Unterstreichung der Favoriten
        constexpr int LOGO_X = 15;            ///< Skalenlogo links oben. Die ersten zehn Sender beginnen erst bei y 102, Sender elf bei x 110
        constexpr int LOGO_Y = 30;
        constexpr int LOGO_W = 90;            ///< Bildgröße logo_90
        constexpr int LOGO_H = 53;
        constexpr int NEEDLE_STRIP = 6;      ///< Wiederhergestellter Streifen je Zeigerposition
        constexpr int NEEDLE_RED_X = 1;       ///< Versatz des roten Zeigers im Streifen
        constexpr int NEEDLE_RED_W = 4;       ///< Breite des roten Zeigers
        constexpr int NEEDLE_MAX_X = WIDTH - NEEDLE_STRIP;
        constexpr float NEEDLE_PX_PER_FRAME = 5.2f; ///< Weg je Displaybild in voller Fahrt. Die Höchstgeschwindigkeit folgt aus der gemessenen Bildrate. Bei rund 25,6 Bildern/s sind es etwa 133 px/s. Die Webnadel nutzt eigene Konstanten in webui/index.html
        constexpr float NEEDLE_SPEED = 140.0f; ///< Höchstgeschwindigkeit in px/s ohne Bildwechselmeldung
        constexpr float NEEDLE_START_S = 0.40f; ///< Anfahren aus dem Stand bis zur vollen Fahrt in s, gleichmäßig beschleunigt
        constexpr float NEEDLE_BRAKE_S = 0.42f; ///< Bremsen aus voller Fahrt bis zum Stand in s, auch vor dem Umkehren. Ein näher liegendes Ziel in Fahrtrichtung darf stärker bremsen (RetroRadio::advanceNeedle)
        constexpr int NEEDLE_SUBSTEPS = 16;    ///< Teilschritte je Pixel: Während der Fahrt mischen die Randspalten der Nadel Rot und Hintergrund nach dem Bruchteil der Lage
        constexpr int NEEDLE_STAGING_W = 64;   ///< Größte Breite des Streifens aus alter und neuer Nadelstelle, die in einem Schritt direkt in die Panelzeilen geht, ohne Panelkopie über einen Zwischenpuffer dieser Breite. Breitere Sprünge gehen aus der Szene
        constexpr int TOUCH_RIGHT = MAX_STATIONS * STATION_PITCH_NUM / STATION_PITCH_DEN; ///< rechte Grenze der Senderwahl
    }

    /** @brief Infozeile mit Codec-Feld, Sender, Titel, Bitrate und Stereo-Symbol. */
    namespace Footer
    {
        constexpr int Y = 316;
        constexpr int HEIGHT = 32;
        constexpr int LINE = 1;              ///< Rahmenlinie oben und unten
        constexpr int RESERVED = 90;        ///< Für Codec-Feld und Stereo-Symbol freigehaltene Breite, je Seite die Hälfte
        constexpr int STEREO_W = 41;
        constexpr int STEREO_H = 25;
        constexpr int STEREO_X = WIDTH - STEREO_W - 1;
        constexpr int STEREO_Y = 319;
        constexpr int STEREO_LEFT_X = 13;    ///< Mittelpunkt linker Ring
        constexpr int STEREO_RIGHT_X = 28;   ///< Mittelpunkt rechter Ring
        constexpr int STEREO_CY = 12;
        constexpr float STEREO_R = 11.0f;
        constexpr float STEREO_THICKNESS = 2.0f;
        constexpr int CODEC_X = 1;           ///< Spiegelbildlich zum Stereo-Symbol
        constexpr int CODEC_Y = STEREO_Y;
        constexpr int CODEC_W = 43;
        constexpr int CODEC_H = STEREO_H;
        constexpr int CODEC_BADGE_H = 18;    ///< Höhe der Codec-Marke
        constexpr int CODEC_BADGE_PAD = 8;  ///< Zusatzbreite zum Text
    }

    /** @brief Magisches Auge oben rechts in der Skala. */
    namespace MagicEye
    {
        constexpr int SIZE = 70;               ///< Breite und Höhe des Sprites
        constexpr int X = 710;                 ///< Linke Kante des magischen Auges
        constexpr int Y = 20;
        constexpr float RING_THICKNESS = 5.0f; ///< Außenring EM11/EM34
        constexpr float EM80_HALF_W = 22.0f;
        constexpr float EM80_HALF_H = 29.0f;
        constexpr float EM84_HALF_W = 12.5f;
        constexpr float EM84_HALF_H = 34.0f;
    }

    /** @brief Lautstärkedrehknopf mit Beschriftung darüber. */
    namespace Knob
    {
        constexpr int X = 0;
        constexpr int Y = 373;
        constexpr int SIZE = 107;
        constexpr int MID = SIZE / 2;           ///< Mittelpunkt im Sprite
        constexpr int LABEL_H = 13;             ///< Beschriftungsstreifen über dem Knopf. Beginnt wie die Reglerbeschriftung bei y 450
        constexpr float RING_R = MID - 1.0f;    ///< Radius mit einem Pixel Abstand zur Spritekante
        constexpr float RING_THICKNESS = 3.0f;
        constexpr int MARKER_OFFSET = 20;       ///< Abstand der Markerbahn vom Rand
        constexpr float MARKER_R = 13.0f;
        constexpr float MARKER_THICKNESS = 3.0f;
        constexpr int VALUE_Y = 15;             ///< Oberkante der Zahl über der Mitte
        constexpr int MUTE_ICON = 28;           ///< Stummsymbol
        constexpr float MUTE_ICON_SCALE = 1.0f; ///< Faktor für IconShapes::draw, nativ in der Bezugsgröße
        constexpr int TOUCH_MUTE_R = 16;        ///< Stummschaltung bis zu diesem Radius
        constexpr int TOUCH_RING_R = 20;        ///< Drehen ab diesem Radius
        constexpr int TOUCH_MARGIN = 20;        ///< Trefferrand um den Knopf
    }

    /** @brief Schieberegler für Balance, Tiefen, Mitten und Höhen in einem Sprite. */
    namespace Tracks
    {
        constexpr int X = 130;
        constexpr int Y = 360;
        constexpr int WIDTH = 190;
        constexpr int HEIGHT = 120;
        constexpr int SLIDER_X0 = 5;       ///< Linke Kante des ersten Reglers
        constexpr int SLIDER_STEP = 50;
        constexpr int TOUCH_W = 31;        ///< Treffer- und Nulllinienbreite
        constexpr int CENTER_X = 15;       ///< Mitte von Bahn, Nulllinie und Knopf im Regler
        constexpr int GROOVE_W = 3;
        constexpr int GROOVE_Y = 2;
        constexpr int GROOVE_H = 116;
        constexpr int PIXEL_MIN = 104;     ///< Knopfmitte bei -12 dB bzw. Balance links
        constexpr int PIXEL_ZERO = 64;
        constexpr int PIXEL_MAX = 24;
        constexpr float KNOB_R = 13.0f;
        constexpr float KNOB_THICKNESS = 3.0f;
        constexpr float KNOB_FILL_R = 10.0f;
        constexpr int HOLD_TOLERANCE = 6;  ///< Weg, den der Finger beim Halten auf dem Knopf wandern darf
    }

    /** @brief Rechter unterer Bereich für Visualizer, Uhren und Tasten: Basisbereich 341..800 x 354..480, rechts unten bündig. */
    namespace Visual
    {
        constexpr int W = 459;
        constexpr int H = 126;
        constexpr int X = WIDTH - W;
        constexpr int Y = HEIGHT - H;
    }

    /** @brief Fünf Bedientasten rechts unten, gleich breit mit gleichen Abständen, rechte Taste am Rand. */
    namespace Buttons
    {
        constexpr int Y = 435;
        constexpr int H = HEIGHT - Y;
        constexpr int W = 45;
        constexpr int GAP = 10;
        constexpr int RADIUS = 4;              ///< Eckenradius
        constexpr int BORDER = 1;              ///< Rahmen
        constexpr int BORDER_PRESSED = 2;      ///< Rahmen gedrückt
        constexpr int X4 = WIDTH - W;          ///< Haus (Hauptmenü und Sleep-Timer)
        constexpr int X3 = X4 - GAP - W;       ///< Reset
        constexpr int X2 = X3 - GAP - W;       ///< Weiter
        constexpr int X1 = X2 - GAP - W;       ///< Zurück
        constexpr int X0 = X1 - GAP - W;       ///< Favoriten
    }

    /** @brief Uhren im unteren Bereich, Lage über die Basiskanten auf die Visualizerfläche abgebildet. */
    namespace Clock
    {
        constexpr int DIGITAL_X = Visual::X + 14;
        constexpr int DIGITAL_Y = Buttons::Y;
        constexpr int DIGITAL_W = 150;
        constexpr int DIGITAL_H = Buttons::H;
        constexpr int ANALOG_X = Visual::X + 380;
        constexpr int ANALOG_Y = Visual::Y;
        constexpr int ANALOG_SIZE = 70;
    }

    /** @brief Dialoge mittig auf dem Display. Die Innenmaße stehen in den Dialogklassen. */
    namespace Dialog
    {
        constexpr int PRESETS_W = 600, PRESETS_H = 440; ///< Favoriten
        constexpr int PRESETS_X = (WIDTH - PRESETS_W) / 2, PRESETS_Y = (HEIGHT - PRESETS_H) / 2;
        constexpr int MESSAGE_W = 300, MESSAGE_H = 200; ///< Reset-Rückfrage
        constexpr int MESSAGE_X = (WIDTH - MESSAGE_W) / 2, MESSAGE_Y = (HEIGHT - MESSAGE_H) / 2;
        constexpr int SLEEP_W = 300, SLEEP_H = 300; ///< Schlaftimer
        constexpr int SLEEP_X = (WIDTH - SLEEP_W) / 2, SLEEP_Y = (HEIGHT - SLEEP_H) / 2;
    }
}
#endif

#ifndef SETTINGS_LAYOUT_H
#define SETTINGS_LAYOUT_H

/**
 * @brief Geometrie der Einstellungen und ihrer Dialoge in 800x480.
 *
 * Alle Maße gelten für das native Display. Alle Seiten folgen denselben Abständen. Werte der Seiten sind relativ zur linken oberen Ecke des Arbeitsbereichs, Werte der Dialoge relativ zu deren linker oberer Ecke.
 */
namespace SettingsLayout
{
    constexpr int WIDTH = 800; ///< Szene deckungsgleich mit dem Display
    constexpr int HEIGHT = 480;

    /** @brief Gemeinsame Rahmen, Radien und Linien von Zeilen, Feldern und Tasten. */
    namespace Style
    {
        constexpr int BORDER = 1;           ///< Rahmen von Feldern, Kacheln und Tasten
        constexpr int BORDER_PRESSED = 2;   ///< Rahmen gedrückter Tasten
        constexpr int LINE = 1;             ///< Trennlinien
        constexpr int FIELD_R = 8;         ///< Zeilen, Felder und Kacheln
        constexpr int FIELD_INNER_R = 7;
        constexpr int BUTTON_R = 6;         ///< Tasten
        constexpr int BUTTON_INNER_R = 5;
        constexpr int ACCENT_INSET = 8;    ///< Akzentstreifen oben und unten eingerückt
    }

    /** @brief Grobaufteilung: Menü links, Scrollbalken, Arbeitsbereich rechts, Fußzeile unten. */
    namespace Frame
    {
        constexpr int FOOTER_H = 56;
        constexpr int VIEWPORT_H = HEIGHT - FOOTER_H;
        constexpr int FOOTER_Y = VIEWPORT_H;
        constexpr int MENU_X = 2;
        constexpr int MENU_W = 170;
        constexpr int SECTION_ROW_H = 28;                     ///< Sektionskopf. Zwei Köpfe und zehn Einträge füllen 416 der 424 Pixel über der Fußzeile
        constexpr int ITEM_ROW_H = 36;                        ///< Höhe eines Menüeintrags, die Beschriftung bleibt einzeilig
        constexpr int ITEM_ACCENT_W = 4;                      ///< Kategoriestreifen
        constexpr int ITEM_TEXT_X = ITEM_ACCENT_W + 6;
        constexpr int SCROLLBAR_X = MENU_X + MENU_W + 4;
        constexpr int SCROLLBAR_W = 6;
        constexpr int DIVIDER_X = SCROLLBAR_X + SCROLLBAR_W;
        constexpr int WORK_X = DIVIDER_X + 10;
        constexpr int WORK_Y = 5;
        constexpr int WORK_W = WIDTH - WORK_X - 10;
        constexpr int WORK_H = VIEWPORT_H - WORK_Y;
        constexpr int WORK_R = 12;
    }

    /** @brief Titelbox oben im Arbeitsbereich. */
    namespace Title
    {
        constexpr int BOX_X = 10;
        constexpr int BOX_Y = 10;
        constexpr int BOX_W = Frame::WORK_W - 2 * BOX_X;
        constexpr int BOX_H = 69;
        constexpr int BOX_R = 10;
        constexpr int TEXT_X = 28;                       ///< Titel und Untertitel
        constexpr int TEXT_Y = 18;
        constexpr int SUB_Y = 50;
    }

    /** @brief Inhaltsbereich unter der Titelbox. */
    namespace Content
    {
        constexpr int X = Title::BOX_X;
        constexpr int Y = Title::BOX_Y + Title::BOX_H + 12;
        constexpr int W = Frame::WORK_W - 2 * X;
        constexpr int H = Frame::WORK_H - Y - 10;
    }

    /** @brief WLAN: drei Profilzeilen, Tastenreihe und Netzwerknamenfeld. */
    namespace Wifi
    {
        constexpr int ROW_H = 46;
        constexpr int ROW_GAP = 8;
        constexpr int ROW_ACCENT_INSET = 6;
        constexpr int DOT_X = 22;           ///< Statuspunkt
        constexpr int DOT_R = 6;
        constexpr int TEXT_X = 40;          ///< Name und Status
        constexpr int NAME_Y = 16;          ///< Mitte des Namens
        constexpr int STATUS_DY = 14;       ///< Mitte des Status über der Zeilenunterkante
        constexpr int ACTION_GAP_Y = 6;     ///< Tastenreihe unter den Profilen
        constexpr int ACTION_H = 44;
        constexpr int ACTION_GAP = 10;
        constexpr int HOST_H = 64;          ///< Netzwerknamenfeld
        constexpr int HOST_BOTTOM_GAP = 10;
        constexpr int HOST_PAD_X = 14;
        constexpr int HOST_LINE_GAP = 4;
        constexpr int EDIT_W = 130;         ///< ändern-Knopf
        constexpr int EDIT_H = 44;
    }

    /** @brief Umschalter in den Profilzeilen. */
    namespace Toggle
    {
        constexpr int W = 55;
        constexpr int H = 29;
        constexpr int MARGIN = 14;  ///< Abstand zum rechten Zeilenrand
        constexpr int INSET = 2;    ///< Gehäuserand
        constexpr int KNOB_R = 10;
        constexpr int KNOB_GAP = 1; ///< Abstand des Knopfes zum Innenrand
    }

    /** @brief Sprachzeilen. */
    namespace Language
    {
        constexpr int ROW_H = 54;
        constexpr int ROW_GAP = 12;
        constexpr int ROW_R = 10;
        constexpr int TEXT_X = 24;
        constexpr int ACTIVE_RIGHT = 20;
        constexpr int HEADING_H = 32;    ///< Rubriküberschrift „Sprache“ bzw. „Serielle Logs“ auf der Seite Allgemein
        constexpr int SECTION_GAP = 24;  ///< Abstand zwischen den Rubriken, die Trennlinie liegt in der Mitte
    }

    /** @brief Themenseite: Zeilen wie auf der Sprachseite, so hoch, dass links eine Minivorschau im Seitenverhältnis des Displays Platz hat. */
    namespace Theme
    {
        constexpr int ROW_H = 92;                                   ///< Drei Zeilen und zwei Abstände füllen 300 der 318 Inhaltspixel
        constexpr int ROW_GAP = Language::ROW_GAP;
        constexpr int PREVIEW_X = Language::TEXT_X;                 ///< Einzug wie der Sprachname
        constexpr int PREVIEW_H = ROW_H - 2 * Style::ACCENT_INSET;  ///< oben und unten bündig mit dem Akzentstreifen
        constexpr int PREVIEW_W = 127;                              ///< 800:480 wie das Display
        constexpr int PREVIEW_R = Style::BUTTON_R;
        constexpr int TEXT_X = PREVIEW_X + PREVIEW_W + Language::TEXT_X; ///< Name rechts der Vorschau
        constexpr int CLOCK_R = 30;                                 ///< Zifferblatt links in der Vorschau
        constexpr int CARD_INSET = 8;                               ///< Abstand der Fläche rechts in der Vorschau zum Rand
    }

    /** @brief Sicherheitsseite: Schalter der PIN-Sperre oben, darunter PIN-Hinweis, PIN-Punkte und Ziffernfeld. */
    namespace Security
    {
        constexpr int HINT_Y = Toggle::H + 40; ///< Mitte des Hinweises unter dem Schalter der PIN-Sperre
        constexpr int DOTS_Y = HINT_Y + 45;    ///< Mitte der PIN-Punkte unter dem Hinweis
        constexpr int DOT_R = 13;
        constexpr int DOT_STEP = 36;           ///< Abstand zweier Punktmitten, die Punkte stehen einzeln
        constexpr int PAD_GAP = 4;   ///< Ziffernfeld
        constexpr int PAD_KEY_H = 27;
        constexpr int PAD_R = 6;
        constexpr int PAD_TEXT_OY = 2;
    }

    /** @brief Zeit: drei Auswahlzeilen, fünf Schrittgeber links, der Knopf zum Setzen rechts daneben, unten die Zeile des Sekundenzeigers. */
    namespace Time
    {
        constexpr int FIELD_H = 50;
        constexpr int FIELD_GAP = 6;
        constexpr int FIELD_PAD_X = 14;
        constexpr int LABEL_Y = 6;
        constexpr int VALUE_Y = 26;
        constexpr int FIELD_COUNT = 3;     ///< Zeitzone, NTP-Server, Intervall
        constexpr int COL_W = 70;
        constexpr int COL_GAP = 6;
        constexpr int STEP_H = 26;         ///< Plus und Minus
        constexpr int VALUE_H = 32;
        constexpr int VALUE_GAP = 2;       ///< Zwischen Knopf und Wert
        constexpr int TOTAL_H = 2 * STEP_H + VALUE_H + 2 * VALUE_GAP;
        constexpr int VALUE_OFFSET_Y = 3;  ///< Ziffern optisch mittig
        constexpr int SECOND_Y = Content::H - FIELD_H; ///< Zeile „Sekundenzeiger“ am unteren Rand des Inhalts
        constexpr int STEPPER_Y = (FIELD_COUNT * (FIELD_H + FIELD_GAP) - FIELD_GAP + SECOND_Y - TOTAL_H) / 2; ///< Mittig zwischen Auswahlzeilen und Sekundenzeile, Schrittgeber links und „zeit setzen“ rechts daneben
        constexpr int SET_W = 160;
        constexpr int SET_H = 45;
    }

    /** @brief Zeitfenster: sieben Wochentage mal 24 Stunden. */
    namespace Window
    {
        constexpr int DAY_LABEL_W = 28;
        constexpr int CELL_H = 27;
        constexpr int CHECK = 15;        ///< Kantenlänge der Checkbox
        constexpr int CHECK_LINE = 1;    ///< Rahmen der Kästchen, bleibt als Ausnahme 1 Pixel, 2 Pixel wirkten zusammen mit dem Haken zu kräftig
        constexpr int HEADER_H = 26;
        constexpr int TOP = 10;          ///< Abstand zum Inhalt
        constexpr int DAY_TEXT_X = 8;
        constexpr int FRAME_R = 6;
        constexpr int INNER_R = 5;
        constexpr int FRAME_INSET = 1;
        constexpr int FRAME_PAD_Y = 2;
        constexpr float TICK_R = 2.0f;   ///< Radius des Hakenstrichs für drawWideLine, die Strichbreite beträgt ungefähr den doppelten Wert
        constexpr int HINT_GAP_Y = 16;
        constexpr int HINT_LINE_H = 19;
        constexpr int HINT_X = 4;
    }

    /** @brief Klang: sechs Zeilen mit Minus, Wert und Plus. */
    namespace Sound
    {
        constexpr int ROW_H = 44;
        constexpr int ROW_GAP = 5;
        constexpr int FIRST_Y = 4;
        constexpr int STEP_W = 58;
        constexpr int VALUE_W = 150;
        constexpr int STEP_GAP = 10;
        constexpr int LABEL_X = 4;
        constexpr int VALUE_OFFSET_Y = 3;
    }

    /** @brief Anzeige: Röhrenkacheln und zwei Wahlzeilen des VU-Meters. */
    namespace Appearance
    {
        constexpr int HINT_H = 23;        ///< Reservierte Höhe einer Hinweiszeile
        constexpr int SEC_LINE_DY = 24;   ///< Linie unter der Überschrift
        constexpr int SEC1_Y = 0;
        constexpr int TILE_Y = 29;
        constexpr int TILE_GAP = 14;
        constexpr int TILE_W = (Content::W - 3 * TILE_GAP) / 4;
        constexpr int TILE_H = 94;
        constexpr int EYE_BOX = 70;      ///< Röhrenvorschau
        constexpr int EYE_Y = 4;
        constexpr float RING = 5.0f;
        constexpr float EM80_HALF_W = 22.0f;
        constexpr float EM80_HALF_H = 29.0f;
        constexpr float EM84_HALF_W = 12.5f;
        constexpr float EM84_HALF_H = 34.0f;
        constexpr int NAME_DY = 13;       ///< Mitte des Namens über der Kachelunterkante
        constexpr int HINT1_Y = 128;
        constexpr int SEC2_Y = 159;
        constexpr int ROW1_Y = 189;
        constexpr int ROW2_Y = 256;
        constexpr int ROW_H = 36;
        constexpr int HINT_DY = 39;
        constexpr int LABEL_W = 150;
        constexpr int SEG_GAP = 6;
        constexpr int LEVEL_W = 220;      ///< Pegelanzeige
        constexpr int LEVEL_GAP = 8;     ///< Zwischen Beschriftung und Pegel
        constexpr int TEXT_X = 2;         ///< Überschriften und Hinweise
    }

    /** @brief Bildschirmseite: Abschnitte im Raster der Anzeige-Seite, Helligkeit als Zeile wie auf der Klangseite. */
    namespace Screen
    {
        constexpr int ROW1_Y = Appearance::TILE_Y;              ///< Helligkeitszeile, so hoch wie eine Klangzeile
        constexpr int HINT1_Y = ROW1_Y + Sound::ROW_H + 8;      ///< Hinweis unter der Helligkeit
        constexpr int SEC2_Y = Appearance::SEC2_Y;              ///< Abschnitt Ausrichtung
        constexpr int ROW2_Y = Appearance::ROW1_Y;              ///< Wahlzeile normal oder 180 Grad gedreht
        constexpr int HINT2_Y = ROW2_Y + Appearance::HINT_DY;   ///< Hinweis darunter, die Fläche darunter bleibt frei
    }

    /** @brief Info: zwei Spalten mit Kopf, Zeilen und I2C-Zeile. */
    namespace Info
    {
        constexpr int COL_GAP = 14;
        constexpr int COL_W = (Content::W - COL_GAP) / 2;
        constexpr int HDR_H = 26;
        constexpr int HDR_PAD_X = 10;
        constexpr int ROWS_GAP = 4;        ///< Erste Zeile unter dem Kopf
        constexpr int ROW_H = 28;
        constexpr int ROW_PAD_X = 8;
        constexpr int I2C_LABEL_GAP = 10;
        constexpr int I2C_BOTTOM_GAP = 10;
        constexpr int I2C_BADGE_Y = 1;
        constexpr int SIGNAL_W = 80;
        constexpr int SIGNAL_H = 8;
        constexpr int SIGNAL_R = 2;
        constexpr int SIGNAL_GAP = 6;      ///< Text vor dem Balken
    }

    /** @brief Fußzeile mit Speichern, Textknöpfen, Klangfeld, Uhr und Haus-Taste. */
    namespace Footer
    {
        constexpr int CY = Frame::FOOTER_Y + Frame::FOOTER_H / 2;
        constexpr int BUTTON_Y = Frame::FOOTER_Y + 6;
        constexpr int BUTTON_H = 44;
        constexpr int SAVE_W = 150;
        constexpr int ACTION_MIN_W = 120;              ///< Textknöpfe
        constexpr int ACTION_PAD = 48;                 ///< Zusatzbreite zum Text
        constexpr int ACTION_GAP = 12;
        constexpr int EXIT = 44;                       ///< Haus-Taste
        constexpr int EXIT_CX = WIDTH - EXIT / 2 - 10;
        constexpr int CLOCK_W = 104;                   ///< Sprite für "88:88" in Font7 mit Textgröße 0,75 wie die Uhren von Radio und SD-Player
        constexpr int CLOCK_H = 45;
        constexpr int CLOCK_X = EXIT_CX - EXIT / 2 - 14 - CLOCK_W; ///< Linke Kante der Uhr, 14 Pixel Abstand vor der Haus-Taste
        constexpr int CLOCK_Y = Frame::FOOTER_Y + (Frame::FOOTER_H - CLOCK_H) / 2 + 1;
        constexpr int CLOCK_GAP = 16;                  ///< Mindestabstand von Textknöpfen und Klangfeld zur Uhr
    }

    /** @brief Klangfeld in der Fußzeile: Hörprobe, leiser, Lautstärke, lauter. */
    namespace SoundFooter
    {
        constexpr int BUTTON = 44;
        constexpr int GAP = 8;
        constexpr int DOT = 3;
        constexpr int DOT_100 = 2;        ///< Schmalere Punkte für 100
        constexpr int DOT_SPACE = 1;
        constexpr int CHAR_SPACE = 5;
        constexpr int CHAR_W = 5 * DOT + 4 * DOT_SPACE;
        constexpr int DIGITS_W = 2 * CHAR_W + CHAR_SPACE;
        constexpr int DISP_W = DIGITS_W;
        constexpr int DISP_H = 7 * DOT + 6 * DOT_SPACE;
        constexpr int FRAME_PAD = 8;
        constexpr int FRAME_W = DISP_W + 2 * FRAME_PAD;
        constexpr int BLOCK_W = BUTTON * 3 + FRAME_W + GAP * 3;
        constexpr int BLOCK_RIGHT = Footer::CLOCK_X - Footer::CLOCK_GAP;
        constexpr int PLAY_CX = BLOCK_RIGHT - BLOCK_W + BUTTON / 2;
        constexpr int DOWN_CX = PLAY_CX + BUTTON + GAP;
        constexpr int FRAME_X = DOWN_CX + BUTTON / 2 + GAP;
        constexpr int DISP_X = FRAME_X + FRAME_PAD;
        constexpr int UP_CX = FRAME_X + FRAME_W + GAP + BUTTON / 2;
    }

    /** @brief Auswahlliste für Zeitzone und Intervall, mittig. */
    namespace Picker
    {
        constexpr int W = 460;
        constexpr int H = 400;
        constexpr int X = (WIDTH - W) / 2;
        constexpr int Y = (HEIGHT - H) / 2;
        constexpr int BORDER = 2;
        constexpr int R = 12;
        constexpr int INNER_R = 10;
        constexpr int TITLE_Y = 34;       ///< Mitte des Titels
        constexpr int LIST_X = 16;
        constexpr int LIST_Y = 60;
        constexpr int LIST_WIDTH = 428;
        constexpr int LIST_HEIGHT = 268;
        constexpr int ROW_STEP = 38;
        constexpr int ROW_H = 36;
        constexpr int ROW_R = 5;
        constexpr int TEXT_X = 14;
        constexpr int END_GAP = 2;
        constexpr int SELECT_Y = 126;     ///< Abstand des gewählten Eintrags zur Listenoberkante beim Öffnen
        constexpr int DRAG = 5;           ///< Ziehschwelle
        constexpr int CANCEL_X = 150;
        constexpr int CANCEL_Y = 340;
        constexpr int CANCEL_W = 160;
        constexpr int CANCEL_H = 44;
    }

    /** @brief Meldungen, die Innenmaße stehen nativ in SettingsMessageBox. */
    namespace Message
    {
        constexpr int X = 200;
        constexpr int Y = 140;
        constexpr int W = 400;
        constexpr int H = 200;
        constexpr int TALL_H = 220;  ///< Meldungen mit Neustartfrage
        constexpr int WIDE_W = 420;  ///< Werkseinstellungen
        constexpr int WIDE_Y = 130;
    }

    /** @brief Fortschrittsdialog des WLAN-Tests, mittig. */
    namespace Progress
    {
        constexpr int W = 400;
        constexpr int H = 200;
        constexpr int BORDER = 1;
        constexpr int R = 4;
        constexpr int MARGIN = 20;
        constexpr int TITLE_Y = 18;
        constexpr int LINE_Y = 48;
        constexpr int LINE_H = 2;
        constexpr int TEXT_Y = 65;
        constexpr int TEXT_LEADING = 2;
        constexpr int BAR_DY = 65;      ///< Balkenoberkante über der Unterkante
        constexpr int BAR_H = 40;
        constexpr int SEGMENT = 1;
        constexpr int SEGMENT_GAP = 2;
    }

    /** @brief WLAN-Auswahl mit fünf sichtbaren Zeilen, mittig. */
    namespace Networks
    {
        constexpr int W = 500;
        constexpr int H = 300;
        constexpr int X = (WIDTH - W) / 2;
        constexpr int Y = (HEIGHT - H) / 2;
        constexpr int BORDER = 1;
        constexpr int R = 4;
        constexpr int TITLE_X = 10;
        constexpr int TITLE_Y = 25;         ///< Titelmitte
        constexpr int CLOSE = 30;           ///< Schließen-Taste
        constexpr int CLOSE_RIGHT = 40;     ///< Linke Kante vor dem rechten Rand
        constexpr int CLOSE_Y = 10;
        constexpr int LIST_X = 10;
        constexpr int LIST_Y = 50;
        constexpr int LIST_WIDTH = W - 2 * LIST_X;
        constexpr int LIST_HEIGHT = 185;
        constexpr int ROW_H = 36;
        constexpr int ROW_TEXT_X = 10;
        constexpr int SIGNAL_RIGHT = 10;
        constexpr int LOCK_RIGHT = 68;     ///< Schlossmitte vor dem rechten Listenrand
        constexpr int LOCK_Y = 16;
        constexpr int SEP_X = 5;            ///< Trennlinie eingerückt
        constexpr int BUTTON_W = 80;
        constexpr int BUTTON_H = 45;
        constexpr int OK_RIGHT = 90;       ///< Linke Kante vor dem rechten Rand
        constexpr int SCAN_RIGHT = 180;
        constexpr int BUTTON_DY = 55;       ///< Oberkante über der Unterkante
        constexpr int SCROLL_RIGHT = 18;
        constexpr int SCROLL_W = 8;
        constexpr int SCROLL_MIN_H = 20;
        constexpr int SCROLL_R = 3;
        constexpr int DRAG = 5;             ///< Ziehschwelle
    }

    /** @brief Bildschirmtastatur, mittig. */
    namespace Keyboard
    {
        constexpr int W = 770;
        constexpr int KEY_H = 45;
        constexpr int KEY_GAP = 4;
        constexpr int R = 4;              ///< Rahmen, Eingabefeld und Tasten
        constexpr int BORDER = 1;
        constexpr int PADDING = 10;
        constexpr int LABEL_H = 40;
        constexpr int INPUT_H = 36;
        constexpr int INPUT_MARGIN = 8;
        constexpr int TITLE_Y = 25;       ///< Titelmitte
        constexpr int CLOSE = 30;         ///< Schließen-Taste
        constexpr int CLOSE_RIGHT = 40;
        constexpr int CLOSE_Y = 10;
        constexpr int TEXT_X = 8;        ///< Texteinzug im Eingabefeld
        constexpr int HIDDEN_DY = 4;      ///< Sternchen tiefer gesetzt
        constexpr int CURSOR_INSET = 6;
        constexpr int CURSOR_W = 1;
        constexpr int LABEL_OFFSET = 15;  ///< Einzug links oder rechts ausgerichteter Tastentexte
    }
}
#endif

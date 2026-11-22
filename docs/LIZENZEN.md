# Lizenzen und Herkunft

Der Quellcode von RetroRadioCrowPanelS3 steht unter der **GNU GPL Version 3**, siehe [vollständigen Lizenztext](../LICENSE). Große Teile des Anwendungscodes stammen aus [RetroRadio](https://github.com/DIYLAB-DE/RetroRadio). Enthaltene Fremdanteile behalten ihre Urhebervermerke und ihre jeweiligen Lizenzen.

## Herkunft des Anwendungscodes

Die Herkunft ist hier zentral beschrieben. Die Quellkommentare erläutern die aktuelle Implementierung und verweisen bei Bedarf auf diese Übersicht. Vorhandene Copyright- und Lizenzvermerke bleiben erhalten.

| Bereich | Grundlage |
| --- | --- |
| Radio, Hauptmenü, Einstellungen und Zeichenfunktionen | DIYLAB-DE/RetroRadio, insbesondere Stand `ea4229a34fb39faaff23619092cf09cafb8c51cf`, über die CrowPanel-Portierung übernommen und für 800 × 480 Pixel angepasst. Dazu gehören Button-/Canvas-Zeichenwege, Touch-Dialoge, Tastatur und Symbolkonturen. |
| SD-Player und Dateiauswahl | Bedienrollen, Wickelmodell, Darstellung und Systemdateifilter aus demselben Quellstand. Dateizugriffe, zentrale Dienste und Audioanbindung wurden für dieses Gerät angepasst. |
| Sprache und Texte | Radio-, Einstellungs-, Kalender-, PIN- und Dateidialogtexte aus RetroRadio, ergänzt um eigene Status- und Fehlermeldungen. |
| DS1307 und Radiowerte | DS1307-Routinen und NVRAM-Grundbelegung aus RetroRadio, dort unter anderem Stand `23bf539`, mit Erweiterungen für die hier vorhandenen Visualizer-Stile. |
| Kassetten, Coverplatzhalter und Audioanzeigen | Bildmotive, Geometrie und Zeichenansätze aus RetroRadio, angepasst an die nativen Abmessungen und den gemeinsamen Bildspeicher. |

Die GPL-Lizenz des Quellprojekts liegt unter [referenz/RetroRadio-LICENSE](../referenz/RetroRadio-LICENSE) bei. Die vollständigen LovyanGFX-, Bodmer- und Adafruit-Hinweise für die Button-Grundlage sind in [RetroRadio-Button-LICENSE.txt](../referenz/RetroRadio-Button-LICENSE.txt) enthalten. Die zugehörigen Copyright-Vermerke stehen zusätzlich in den betroffenen Dateien.

## Quellcode und Bibliotheken

| Bestandteil | Herkunft und Lizenz | Beilage |
| --- | --- | --- |
| RetroRadioCrowPanelS3 | DIYLAB-DE, GPL-3.0, Portierung von RetroRadioCrowPanel auf das CrowPanel 7.0" ESP32-S3 | [GPLv3](../LICENSE) |
| Übernommene Teile aus RetroRadio | Radio, SD-Player, Hauptmenü, Einstellungen, Visualisierungen, Weboberfläche, Schriften und Bilder aus DIYLAB-DE/RetroRadio, GPL-3.0 | [Lizenz des Quellprojekts](../referenz/RetroRadio-LICENSE) |
| audio-lite v0.2.0 | Gemeinsame Bibliothek von DIYLAB-DE, veränderte Fassung von Wolle/schreibfaul1s ESP32-audioI2S 3.4.7, GPL-3.0 laut Herkunftsprojekt, ergänzende Bedingungen einzelner Bestandteile bleiben bestehen | [Herkunft](https://github.com/DIYLAB-DE/audio-lite/blob/v0.2.0/docs/PROVENANCE.md), [Änderungen](https://github.com/DIYLAB-DE/audio-lite/blob/v0.2.0/docs/UPSTREAM.md), [Lizenz](https://github.com/DIYLAB-DE/audio-lite/blob/v0.2.0/LICENSE), [Komponentenlizenzen](https://github.com/DIYLAB-DE/audio-lite/blob/v0.2.0/docs/LICENSES.md) |
| MP3-Decoder in audio-lite | Auf Helix von RealNetworks basierend, siehe [Helix-Herkunft](#helix-herkunft) | [Helix-Hinweis](lizenzen/Helix-LICENSE.txt), [RPSL](lizenzen/Helix-RPSL.txt), [RCSL](lizenzen/Helix-RCSL.txt) |
| AAC-Decoder in audio-lite | libfaad2, M. Bakker/Nero AG, GPL-2.0-or-later laut Dateiköpfen | [GPLv2](lizenzen/GPL-2.txt), [Urhebervermerke im Decoder](https://github.com/DIYLAB-DE/audio-lite/tree/v0.2.0/src/aac_decoder) |
| Vorbis-Decoder in audio-lite | Xiph.Org Foundation, Dateikopf nennt Copyright 1994-2007 und BSD-Bedingungen | [Xiph-Lizenztext](lizenzen/Xiph-Vorbis-COPYING.txt), ursprüngliche Vermerke im Decoder |
| KISS FFT | Mark Borgerding, Copyright 2003-2010, BSD-3-Clause | [COPYING](../lib/kissfft/COPYING), [BSD-Bedingungen](../lib/kissfft/LICENSES/BSD-3-Clause) |
| Button-Klasse `UiButton` | Erweiterte Fassung von LGFX_Button aus LovyanGFX, über RetroRadio übernommen, Herkunftsvermerke in den Dateiköpfen | [Lizenzbeilage](../referenz/RetroRadio-Button-LICENSE.txt) |
| LovyanGFX 1.2.36 | lovyan03, FreeBSD-Lizenz, für RGB-Bus, Sprites, VLW-Schriften und Bitmapschriften wie DejaVu und Font7 | Wird von PlatformIO mit den Lizenztexten der Bibliothek geladen |
| Arduino-ESP32 3.3.7 | Espressif und Mitwirkende, LGPL-2.1-or-later, weitere Bestandteile mit eigenen Vermerken | [LGPL-2.1](lizenzen/LGPL-2.1.txt), [Arduino-ESP32](https://github.com/espressif/arduino-esp32) |
| ESP-IDF | Espressif und Mitwirkende, überwiegend Apache-2.0, darunter auch der HTTP-Server und cJSON mit eigenen Vermerken | [Apache-2.0](lizenzen/Apache-2.0.txt), [ESP-IDF](https://github.com/espressif/esp-idf) |

Die PlatformIO-Abhängigkeiten werden anhand der Projektkonfiguration in fester Version bezogen. Ihre Lizenztexte liegen den von PlatformIO geladenen Paketen bei. Aus dem Herstellerwiki des CrowPanels stammen nur Fakten (Pins, Timing, Resetfolge des Touchcontrollers), kein Code.

### Helix-Herkunft

audio-lite übernimmt den MP3-Decoder aus [ESP32-audioI2S 3.4.7, Commit 59bf36b](https://github.com/schreibfaul1/ESP32-audioI2S/tree/59bf36b214580ad2d1157748cd0268c9376d6640). Der [Dateikopf dieses Stands](https://github.com/schreibfaul1/ESP32-audioI2S/blob/59bf36b214580ad2d1157748cd0268c9376d6640/src/mp3_decoder/mp3_decoder.h) nennt Helix als Grundlage. Die Änderungen sind in der [gemeinsamen audio-lite v0.2.0](https://github.com/DIYLAB-DE/audio-lite/blob/v0.2.0/docs/UPSTREAM.md) beschrieben.

Die öffentlich belegbaren Stationen der Herkunft sind:

| Stand | Beleg |
| --- | --- |
| 15. Januar 2019 | Der [Import 2d96710](https://github.com/schreibfaul1/ESP32-audioI2S/commit/2d96710ebfd1e80899bee8aaae1edb6e9d45b98e) enthält den MP3-Decoder, die [damalige README](https://github.com/schreibfaul1/ESP32-audioI2S/blob/2d96710ebfd1e80899bee8aaae1edb6e9d45b98e/README.md) nennt den enthaltenen Helix-MP3-Decoder. |
| 5. September 2019 | Wolle/schreibfaul1 fügt mit [Commit 2e43c40 die GPLv3](https://github.com/schreibfaul1/ESP32-audioI2S/blob/2e43c4089fd571d776e95d2a9f625eb4d7a80e16/LICENSE) hinzu und bestätigt die ergänzte Lizenz in [Issue 5](https://github.com/schreibfaul1/ESP32-audioI2S/issues/5#issuecomment-528328654). |
| Übernahme für RetroRadio | Grundlage ist ESP32-audioI2S 3.4.7. Herkunft, Änderungen und die GPLv3 des Herkunftsprojekts bleiben in audio-lite dokumentiert. Dieser Übernahmeweg beschreibt die Herkunft. CrowPanelS3 bezieht heute die feste Version v0.2.0 aus DIYLAB-DE/audio-lite. |

Die separat beigelegten ursprünglichen Helix-Hinweise stammen aus der erhaltenen Fassung unter [ESP8266Audio/libhelix-mp3](https://github.com/earlephilhower/ESP8266Audio/tree/master/src/libhelix-mp3) und nennen Copyright 1995-2004 RealNetworks sowie RPSL/RCSL. Damit sind die Herkunft aus ESP32-audioI2S und dessen GPLv3-Angabe nachvollziehbar belegt. Die genaue Zuordnung der ursprünglichen Helix-Lizenzbedingungen zum übernommenen Decoder und eine zusätzliche GPL-Lizenzoption für dessen RealNetworks-Anteile sind damit noch nicht abschließend geklärt.

## Schriften und Symbole

| Schrift | Verwendung | Lizenz |
| --- | --- | --- |
| Comfortaa Bold | Beschriftungen am Gerät und in der Weboberfläche | SIL OFL 1.1, [Lizenztext](fonts/OFL-Comfortaa.txt) |
| League Gothic | Tastenbeschriftungen in Dialogen, Radio, SD-Player und Einstellungen | SIL OFL 1.1, [Lizenztext](fonts/OFL-LeagueGothic.txt) |
| Saira Condensed Bold | Ziffern der analogen VU-Skalen | SIL OFL 1.1, [Lizenztext](fonts/OFL-Saira.txt) |
| Orbitron Light | Initialen auf dem Cover-Platzhalter des SD-Players | SIL OFL 1.1, [Lizenztext](fonts/OFL-Orbitron.txt) |

Die VLW-Schriften unter `include/fonts/` und `src/fonts/` wurden aus diesen Schriften erzeugt. Die Quelldateien (TTF) sind nicht versioniert. Die Webschrift der Weboberfläche ist lokal im HTML eingebettet, dort ist ihr Lizenztext beigefügt.

Die Symbole für Radio und Einstellungen im Hauptmenü stammen von **Phosphor Icons**, Copyright 2023, unter MIT. Die Haus-Taste verwendet eine angepasste Kontur von [House Bold](https://github.com/phosphor-icons/core/blob/main/assets/bold/house-bold.svg). Ebenso angepasst sind die Tasten Zurücksetzen in Radio und SD-Player sowie Dateiauswahl und Liste leeren im SD-Player nach [Arrow Counter Clockwise Bold](https://github.com/phosphor-icons/core/blob/main/assets/bold/arrow-counter-clockwise-bold.svg), [Folder Open Bold](https://github.com/phosphor-icons/core/blob/main/assets/bold/folder-open-bold.svg) und [Trash Bold](https://github.com/phosphor-icons/core/blob/main/assets/bold/trash-bold.svg). Der vollständige Text liegt als [Phosphor-Lizenz](lizenzen/Phosphor-Lizenz.txt) bei und steht außerdem in `include/IconShapeData.h`. Das Symbol des SD-Players ist eigenes Material des Projekts.

## Darstellungen und Bilder

Die vier magischen Augen EM11, EM34, EM80 und EM84 werden durch eigenen Zeichencode dargestellt. Die EM34-Optik orientiert sich am **EM34-Emulator von N. Waldt**, dessen Veröffentlichung steht unter MIT. Die Vorlagenfotos beziehungsweise Röhrentabellen sind nicht Bestandteil des Repositorys.

Das mittig wachsende Spektrum orientiert sich optisch an [spectrum24 von Volos Projects](https://github.com/VolosR/spectrum24). Es wurden keine Quelltextpassagen, Treiber oder Schriften aus diesem Projekt übernommen.

Die Bedienbilder sind Bildschirmaufnahmen des Geräts und seines Webportals. Persönliche Angaben wurden unkenntlich gemacht. Die Fotos der DS1307-Platine mit entfernter D1 und CR2032 stammen aus der bestehenden CrowPanel-Benutzerdokumentation und werden mit Erlaubnis wiederverwendet. Das Gerätefoto wurde für dieses Projekt bereitgestellt. Angezeigte Musiktitel und Cover dienen nur der Bedienungsdemonstration, Audiodateien werden nicht mitgeliefert.

Alle acht Kassettenmotive **Auralite, Auralux, Aurion, Ember, Glasstone, Lumotape, Modutape und Sunbeam** stammen aus RetroRadio. Sie wurden mit ChatGPT erzeugt und für die Darstellung aufbereitet. Die Vorlagen sind nicht versioniert. Die beiden analogen VU-Skalen, das Radiologo, der Cover-Platzhalter und die Projektgrafiken gehören zu den Projektbildern.

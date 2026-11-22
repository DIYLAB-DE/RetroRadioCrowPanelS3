# Stand und Änderungen

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Stand und Änderungen

## v26.11.22 – 22. November 2026

### Rotes Häuschen in Radio und SD-Player

Ein **kurzer Tipp** auf das rote Häuschen beendet jetzt direkt die Wiedergabe und führt zum Hauptmenü. **Mindestens 0,8 Sekunden gedrückt halten** öffnet den Sleep-Timer. Damit ist der direkte Rückweg vom Aufruf des Timers getrennt. Das rote Häuschen innerhalb des Timerdialogs beendet die Wiedergabe ebenfalls sofort und führt zum Hauptmenü. **abbruch** schließt dagegen nur den Dialog.

### Zentrale Audio-Bibliothek

Umstellung auf die gemeinsame **audio-lite v0.2.0** als feste Abhängigkeit `https://github.com/DIYLAB-DE/audio-lite.git#v0.2.0`. Die bisher mitgelieferte Kopie unter `lib/audio-lite` wurde entfernt.

Die SD-Lesegröße bleibt bei **8192 Bytes**. Früher Ausgabehook, separater Visualizer, Mono-Ausgabe und bisherige Audioeinstellungen bleiben erhalten. **Für diese Migration musste kein funktionaler Anwendungscode angepasst werden**, die Abhängigkeitsumstellung genügte.

### Dokumentation und Geräteinformation

Der Produktstand umfasst das CrowPanel 7.0″ ESP32-S3 mit 800 × 480 Pixeln, Radio, SD-Player, Uhr, Einstellungen, Webportal und FTP. Die Dokumentation zeigt das tatsächlich verwendete Leihgerät. Angaben zur eingeschränkten Weiterentwicklung stehen bereits auf der [Startseite](../README.md).

Die Erste Inbetriebnahme beschreibt Windows, Linux und macOS und liegt auch als [PDF](RetroRadioCrowPanelS3-Ersteinrichtung.pdf) vor. Dazu kommen Aufnahmen von Display und Weboberfläche sowie die Anleitung für die externe DS1307 an der I2C-Buchse.

Vor der Vorbereitung dieses Produktstands wurden Kommentare und Formatierung geprüft. Veraltete Hardwarebeschreibungen wurden berichtigt. Die Geräteinformation bezeichnet den USB-Seriell-Wandler als **CH340 an UART0**.

Die Firmware trägt die Versionsanzeige **v26.11.22** vom **22.11.2026**. Der genaue Quellstand ist über die Git-Historie des Produktrepositorys nachvollziehbar.

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Stand und Änderungen

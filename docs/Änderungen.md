# Änderungen

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Änderungen

Hier steht, was sich von Version zu Version ändert, die neueste Version steht oben. Neue Versionen erscheinen als [Release auf GitHub](https://github.com/DIYLAB-DE/RetroRadioCrowPanelS3/releases) mit eigener Nummer. Die Nummer zeigt das CrowPanel unter Einstellungen -> Info & Neustart und die Weboberfläche im Abschnitt System. Sie ändert sich nur mit einem neuen Release, auch wenn Du die Firmware selbst baust.

## v26.11.24 vom 24.11.2026

Die zweite öffentliche Version, [v26.11.24 auf GitHub](https://github.com/DIYLAB-DE/RetroRadioCrowPanelS3/releases/tag/v26.11.24).

### Geändert

- Geändert: Das Menü links in den Einstellungen nutzt die ganze Bildschirmhöhe. Die Einträge und ihre Tippflächen sind größer, die Trennlinie unter dem letzten Eintrag ist weg.
- Geändert: Die gesicherte Lautstärke gilt nach einem Neustart auch ohne die optionale Uhr DS1307. Bisher begann ein Gerät ohne Uhr jeden Start mit höchstens Lautstärke 10.
- Geändert: **Zurücksetzen** am Gerät und **Werkseinstellungen** in der Weboberfläche löschen jetzt auch die gesicherten Radiowerte (Lautstärke, Klangregler, Senderwahl und Visualisierung), die eigenen Farben des Themas Individuell und das FTP-Passwort. Die Senderliste bleibt erhalten ([Neustart und Zurücksetzen](Bedienung.md#neustart-und-zurücksetzen)).
- Geändert: Die Nadel der Radioskala fährt in der Weboberfläche so schnell wie am Gerät.

### Behoben

- Behoben: Die Weboberfläche lud trotz guter WLAN-Verbindung langsam und reagierte träge, eine Bildschirmaufnahme ließ besonders lange auf sich warten. Seiten, Bedienung und Bildschirmaufnahme sind jetzt deutlich schneller.
- Behoben: Nach manchen Starts antwortete das Radio im Netz verzögert, nach anderen sofort. Die Stromsparfunktion des WLAN blieb je nach Start eingeschaltet. Jetzt ist sie nach jedem Start aus.
- Behoben: Das Hochladen über FTP wurde immer mit der Meldung abgewiesen, es sei zu wenig Speicher frei. Uploads funktionieren jetzt, und auch nach einer langen Übertragung bleibt die Verbindung bestehen. Bei laufendem Radio bleiben Uploads wie bisher gesperrt ([Dateimanager und FTP](Weboberfläche.md#dateimanager-und-ftp)).
- Behoben: Nach **Speichern & neu starten**, **Neu starten** oder dem Speichern eines Themas zeigte der Browser eine Fehlerseite, weil die Weboberfläche zu früh neu lud. Sie wartet jetzt, bis das Radio wieder antwortet. Die Meldung „Zugriff verweigert“ direkt nach dem Start tritt nicht mehr auf.
- Behoben: Unter Einstellungen -> Info & Neustart lag das I2C-Feld über der Zeile FTP. Jetzt haben alle Zeilen Platz. Bei eingeschaltetem FTP zeigt die Zeile die Adresse, zum Beispiel `ftp://crow@192.168.178.123`.

### Bekannt und offen

- Die Lautstärke ohne Uhr und das erweiterte Zurücksetzen sind am Gerät noch nicht geprüft.
- Nach dem Abschalten der Stromsparfunktion des WLAN steht eine Hörprobe noch aus.

## v26.11.22 vom 22.11.2026

Die erste öffentliche Version von RetroRadioCrowPanelS3.

### Umfang

- **Gerät:** Elecrow CrowPanel 7.0″ ESP32-S3 mit 800 × 480 Pixeln, mit Radio, SD-Player, Uhr, Einstellungen, Weboberfläche und FTP.
- **Rotes Häuschen:** In Radio und SD-Player beendet ein kurzer Tipp auf das rote Häuschen die Wiedergabe und führt zum Hauptmenü. Mindestens 0,8 Sekunden gedrückt halten öffnet den Sleep-Timer. Im Dialog des Timers beendet das rote Häuschen die Wiedergabe ebenfalls, **abbruch** schließt nur den Dialog ([Haus-Taste und Sleep-Timer](Radio-und-Musik.md#haus-taste-und-sleep-timer)).
- **Audio-Bibliothek:** Der Ton läuft über die gemeinsame Bibliothek audio-lite v0.2.0, eine feste Abhängigkeit des Projekts. Eine eigene Kopie unter `lib/audio-lite` gibt es nicht mehr.
- **Geräteinformation:** Die Seite Info & Neustart nennt den USB-Seriell-Wandler als CH340 an UART0.
- **Dokumentation:** Die [Erstinstallation](RetroRadioCrowPanelS3-Ersteinrichtung.md) beschreibt Windows, Linux und macOS und liegt auch als [PDF](RetroRadioCrowPanelS3-Ersteinrichtung.pdf) vor. Dazu kommen Aufnahmen von Display und Weboberfläche und die Anleitung für die Uhr DS1307 an der I2C-Buchse.

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Änderungen

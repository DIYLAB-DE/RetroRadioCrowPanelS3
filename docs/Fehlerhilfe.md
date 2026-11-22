# Fehlerhilfe

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Fehlerhilfe

Das Leihgerät steht nicht mehr für weitere Entwicklung zur Verfügung. Eine neue Meldung kann deshalb unter Umständen nur anhand von Quellcode, Fotos und Protokollen untersucht werden. Beschreibe Modell, Platinenstand, Quellstand und die Schritte bis zum Fehler. Veröffentliche keine WLAN-Passwörter, PINs, FTP-Passwörter oder ungeschwärzten Netzwerkdaten.

| Beobachtung | Prüfen |
| --- | --- |
| PIN beim ersten Start unbekannt | **0000 – vier Nullen**, danach **ok**. Nach einer eigenen PIN-Änderung gilt Deine gespeicherte PIN, auch nach einem Update. |
| Windows: Pfad oder Dateiname zu lang | Kurzen Projektpfad wie `C:\RRS3` wählen. [Pfadlängen und Windows-Einstellungen](RetroRadioCrowPanelS3-Ersteinrichtung.md#windows-pfad-oder-dateiname-zu-lang). |
| macOS: Python meldet fehlendes `lzma` | [xz nachinstallieren und die PlatformIO-Python-Umgebung prüfen](RetroRadioCrowPanelS3-Ersteinrichtung.md#macos-python-meldet-fehlende-lzma-unterstützung). |
| macOS: USB-Gerät vorhanden, serieller Port fehlt | [WCH-Treiber-App installieren und Treibererweiterung freigeben](RetroRadioCrowPanelS3-Ersteinrichtung.md#macos-ch340-treiber-installieren-und-freigeben). |
| Kein serieller Anschluss | USB-Datenkabel, CH340-Treiber, Stromversorgung und tatsächlichen Port prüfen. |
| Upload scheitert | Serielle Monitore schließen, Anschluss ausdrücklich angeben und die erste Fehlermeldung lesen. Die Build-Umgebung `crowpanel7` mit dem enthaltenen Boardprofil `crowpanel7-s3` verwenden. |
| Bild bleibt dunkel | Versorgung, Helligkeit und Zeitfenster prüfen. Falls nötig kurz berühren. Ein anderes Displaymodell benötigt andere Einstellungen. |
| Bild steht auf dem Kopf | Unter **Einstellungen -> Bildschirm** die feste Drehung um 180° wählen. |
| WLAN verbindet nicht | 2,4-GHz-Netz, Passwort, aktiviertes Profil und gespeicherte Einstellungen prüfen. Ein Test allein speichert das Profil nicht. |
| Keine oder falsche Uhrzeit | WLAN, Zeitserver und Zeitzone prüfen. Ohne externe Uhr braucht das Gerät nach einem stromlosen Start einen Zeitabgleich. |
| Ein HTTPS-Sender startet nicht | Gültige Uhrzeit, erreichbare Streamadresse und einen anderen Sender prüfen. Manche Sender ändern ihre Adressen oder Formate. |
| Kein Ton | Lautstärke, Stummschaltung, **Klang -> Lautsprecher**, Streamstatus und Lautsprecheranschluss prüfen. |
| SD-Karte fehlt | FAT32-Format, Sitz der Karte und Neustart prüfen. exFAT wird nicht unterstützt. Das Gerät formatiert die Karte nicht. |
| SD-Player nimmt keinen Start an | Web-Dateiverwaltung und FTP-Verbindung schließen, dann erneut versuchen. Gleichzeitige Zugriffe auf die Karte sind begrenzt. |
| Titel fehlt in der Auswahl | Endung und Dateipfad prüfen. Ungültige Namen und Pfade, Pfade über 255 Byte und versteckte Verwaltungsdateien werden ausgelassen. Lange Anzeigenamen werden gekürzt. |
| Cover fehlt | `folder.jpg` im Titelordner prüfen. Pro Bild gilt höchstens 1 MB, eingebettete PNG-Bilder zusätzlich höchstens 500 × 500 Pixel. |
| Webportal nicht erreichbar | IP-Adresse am Gerät ablesen und im selben Heimnetz über `http://` öffnen. Gastnetze können Verbindungen zwischen Geräten sperren. |
| Web-Radiosteuerung fehlt | Am Display das Radio öffnen. Der entsprechende Webbereich erscheint nur bei aktivem Radiomodul. |
| FTP-Verbindung scheitert | FTP einschalten, angezeigte Adresse, Port 21 und aktuelle Zugangsdaten verwenden. Im Client nur eine gleichzeitige Verbindung und passiven Modus erlauben. |
| Dateien lassen sich nicht hochladen | Wiedergabe beenden. Die Dateiübertragung ist zugunsten der Audioausgabe eingeschränkt. |
| Versionsnummer bleibt nach eigenem Build gleich | Die Version ist fest im Projekt hinterlegt und ändert sich nur mit einem ausdrücklich neuen Versionsstand. |

## Aussagekräftige Rückmeldung

Nenne die verwendete Build-Umgebung, den Git-Commit oder das Datum der bezogenen Quelle und die genaue Fehlermeldung. Bei einem Streamproblem hilft die öffentlich erreichbare Streamadresse. Bei einem Dateiproblem reichen zunächst Dateiformat, Abtastrate, Dateigröße und die Bedienfolge. Lade keine Musikdateien oder private Sicherungen ungefragt hoch.

Ein erfolgreicher Build sagt aus, dass der Quellcode mit der vorgesehenen Toolchain übersetzt und gelinkt wurde. Er ersetzt keine Prüfung jeder Kombination aus Sender, Datei, Karte und Hardware. Unterschiedliche Karten, Netzbedingungen und Datenformate können sich auf das Verhalten auswirken.

[RetroRadioCrowPanelS3](../README.md) > [Dokumentation](../README.md#dokumentation) > Fehlerhilfe

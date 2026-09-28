# TFTD Workshop 2.12.7

## 1. Einstieg

Workshop 2.12.7 bearbeitet und zeigt TFTD-/OXCE-Inhalte. Es öffnet Karten, erstellt Baugruppen und erzeugt prozedurale Karten. Es ist kein Malprogramm: PNGs in einem Bildeditor erstellen oder bearbeiten und hier prüfen.

Drei getrennte Begriffe:
• MAP: Teilepositionen mit vier Schichten je Feld.
• MCD: Identität und Eigenschaften, Bilder, Bewegung, Türen, Zerstörung, Höhen und Belegung.
• PNG: grafische Darstellung. Ein neues Bild ändert nicht automatisch Kollision, ZE, Schüsse oder Zerstörungszustände.

Schichten: Boden, Westwand, Nordwand und Objekt. Z ist die vertikale Ebene. Mehrere Teile können auf verschiedenen Schichten dasselbe Feld belegen. Ein Felsenbild kann ein Viertel einer Baugruppe aus vier Feldern sein: zuerst das Ganze prüfen.

Unter Ressourcen die Pfade für TFTD ORIGINAL, OXCE STANDARD und OXCE MODS einrichten. TFTD verweist auf Spieldaten, OXCE auf den Installationsordner, MODS auf user/mods. Neu indexieren liest Dateien und Profile erneut. Die Bereiche trennen Originale, OXCE-Ergänzungen und Mods. Persönliche Pfade sind lokale Einstellungen, keine vorgeschriebene veröffentlichte Konfiguration.

Eine vorhandene SEABED-MAP zum Studium bekannter Baugruppen öffnen. Originale sind geschützt; Arbeitskopie im eigenen Mod erstellen. Zuerst ein verstandenes Einzelteil, dann die ganze Baugruppe prüfen. Stand: 28. September 2026. Prototypen sind keine im Spiel bestätigten Funktionen.

## 2. Workshop-Funktionen

DATEI
Neue MAP legt X, Y und Z getrennt fest (1 bis 255). MAP öffnen lädt logische Geometrie und Datensätze. Speichern schreibt aktuelles Dokument: Szenen als JMW; Mod-MAP mit Sicherung. In Mod speichern / Export verlangt Name, Biom, Gruppe. Beenden behandelt ungespeicherte Arbeit.

RESSOURCEN
TFTD/OXCE/MODS-Pfade bestimmen Quellen. Manueller HD-Mod wählt PNG-Anbieter ohne Änderung der Logik. Zusätzliche MAP-Ordner sind für Kompatibilität, nicht Profilvermischung anderer Mods. Neu indexieren liest Quellen erneut. Palette leeren entfernt aktive Liste, keine Dateien. MCD hinzufügen/LBM-Palette laden sind erweitert: PCK/TAB müssen zum MCD passen.

BROWSER
Karten öffnet/filtert MAP. Bibliothek wählt Teile und zeigt Datensatz/MCD. MAP-Datensätze zeigt aktive Sätze und Indizes. Suche filtert; Trennleisten skalieren Bereiche. LEGACY INAKTIV/OXCE UNERREICHBAR bedeutet Referenzstatus, keinen Löschauftrag.

BEARBEITUNG
Auswahl [V]: Teil auf aktivem Z; Umschalt+Klick ergänzt/entfernt. Baugruppe erkennt nur vollständige Bibel-/Hangargruppen. Platzieren [B]: Teil wählen und klicken; Pinsel erlaubt fortlaufendes Malen. Radieren [E] löscht ausdrücklich auf gewähltem Z. Pipette [I] nimmt Teil auf. Verschieben [M]/Duplizieren verlangt Ziel; Mausrad ändert Z, Klick bestätigt, Rechtsklick/Esc bricht ab. Löschen entfernt Auswahl. Strg+Z stellt gesamten Vorgang wieder her; Strg+Y wiederholt.

Optionen erlaubt ausdrücklich Ersetzen belegter Plätze, Baugruppenauswahl, Abdunklung, Deckkraft und Einpassen. FLOOR-/BigWall-Hinweise fordern Prüfung, keine automatische Reparatur. Technische Details zeigt MCD, Bild, Schicht, Eigenschaften und Herkunft.

ANSICHTEN
F1/F2/F3/F4 wählt Boden/Westwand/Nordwand/Objekt. Mausrad ändert Z; Strg+Mausrad zoomt; Mitteltaste verschiebt. BildAuf/BildAb zoomt. Nur aktiv / aktiv+darunter / Gesamtansicht werden gemerkt. Platzieren bleibt auf aktivem Z. Raster [G] blendet auch Schiffsreservierungen aus. Zentrieren/Einpassen findet Karte wieder.

PLAN
Teile zeigt Grafik; 2D-Plan logische Draufsicht; ISO perspektivische Anmerkungen. [P] wechselt. Boden/Zone und Objekt/Dekor sind getrennt. Feld, Umriss, gefüllt, füllen bestimmt Aktion; Quadrat/Kreis/Raute und Größe die Fläche. Gerade/diagonale Wände, Dreiecksböden und Z-Verbindungen beschreiben Plan. Sperre schützt Feld. Rechteckauswahl, Strg+C, Strg+V und Klick fügt ein; Esc bricht ab. Kopie erhält logische Daten und vier Schichten. Anmerkung ergänzt keine Motorregel. Diagonalen benötigen kompatibles BigWall-MCD.

BAUGRUPPEN / HANGAR
Hangar verwaltet eigene Bibliothek. Neue Aufnahme: sichtbare Teile aller Schichten/Ebenen anklicken; erneut entfernt; benennen/speichern. Verwenden nimmt Baugruppe auf; umbenennen/löschen verwaltet Einträge. Ganze MAP kopieren erhält relative Positionen. Hangarordner wählen wechselt Bibliothek.

KOMPOSITION
Originalgetreues OpenXcom folgt offiziellen Vorgaben; Vorlage ansehen erklärt Befehle/Gruppen. Frei kombiniert Familie mit Größen als Vielfache von 10. Variante bestimmt Startwert; andere Variante ändert ihn. Schiff-/USO-Optionen: Fahrzeuge, Positionen, Abstand. USO einsetzen füllt Platz. JMW speichern/öffnen erhält Szene; schließen kehrt zum darunterliegenden Dokument zurück. Nicht unterstützte Vorlagen werden abgelehnt, nie still ersetzt.

DARSTELLUNG
F6 wechselt Legacy, PNG Remastered, manuellen Anbieter, REAL HD, Debug. Universelle Vorlagen ist Grafikanbieter. GEO_TERRAIN öffnet unabhängigen Inspektor für 102 Teile/Szenen: GEO öffnen/speichern, Teil ansehen, Referenzbaugruppen, neutrale Geometrie/SAND-Material, sichtbaren Bereich/Ebene. Externes Labor ist vom Generator getrennt.

RMP-ROUTEN
Ein-/ausblenden [R] ändert keine Daten. Bearbeitung setzt/wählt/löscht Knoten. Verbindung verbindet zwei; N/O/S/W-Anschlüsse verbinden Blöcke. Einfache Route nutzt neutrale Werte; alle/1x1/Flug begrenzt Einheiten. Erweitert: Rang, Patrouillenpräferenz, Spawn-Priorität, Ziel. Analyse schlägt im Speicher vor; entfernen löscht Vorschläge; anwenden ergänzt geprüfte Knoten. RMP speichern schreibt Mod mit Sicherung. Vorschläge beweisen keine Missions-KI-Navigation.

PROZEDURAL folgt im nächsten Kapitel. SPRACHE wechselt Oberfläche und merkt Auswahl. ANLEITUNG öffnet Hilfe. HILFE/Über zeigt Version und Grenzen.

## 3. Generator und Baugruppen

Prozedural verbessert einen einzigen Generator. Größe: Breite/Länge 20 bis 120 Felder. Vielfalt: locker, vielfältig, dicht. Relief: flacher Boden oder SAND + GEO mit Terrassen/mehreren Ebenen. Abstand: 2 bis 5 Felder. Optionale X-COM-/Alienschiffe behalten Richtung. Startwert (0 bis 4294967295) reproduziert Karte bei gleichen Ressourcen/Version. Zufallskarte ändert Startwert und erzeugt. Fehler/Abbruch erhält vorheriges Dokument.

Originalbaugruppen werden mit neuem Relief kombiniert. Dekor steht auf horizontalen Bereichen mit freiem geometrischem Zugang. Das beweist nicht, dass alle Einheiten die Karte im Spiel durchqueren können.

Felsenkatalog: eigenständige ROCKS-MCD 0/1; 2×2-Blöcke mit Reihen [9,8]/[7,10], [5,4]/[3,6] oder [9,8]/[7,6], in Original-MAP bestätigt. ROCKS 10 ist nicht eigenständig. Blöcke reservieren ganze Fläche. Über Boden nur Einzelsteine. Neue Baugruppen mit ausdrücklichen Teilen/Positionen definieren, nicht aus Bildschirmnähe ableiten.

GEO-Stand: Erzeugung, Teileansicht und JMW4-Speicherung mit automatischen Prüfungen. Direkte GEO-Bearbeitung, Planansicht und MAP/OXCE-Export nicht angebunden. Export gesperrt; .JMW speichern. Ältere Programme lehnen JMW4 ab; alte Projekte bleiben lesbar. Dekor auf Zwischenhöhen bleibt an GEO gebunden. Form derzeit durch andere Startwerte verändern.

## 4. HD-PNG mit 512 × 640 erstellen

Ein Legacy-Geländesprite nutzt eine Hülle von 32 × 40 Pixeln. Standard-HD-Leinwand 512 × 640 entspricht ×16 in beiden Achsen. Das ist die Leinwand, kein bis zum Rand zu streckendes Objekt. Isometrische Projektion, Ursprung, relative Position und Transparenz erhalten. Teile nicht einzeln beschneiden oder am Fuß neu zentrieren. Manche dokumentierte Vorlagen reichen darüber hinaus: nicht auf 640 Höhe abschneiden.

Vor Zeichnen Datensatz, MCD-Index, Frame[0], Schicht, Richtung, Animation, zerstörten Zustand und Baugruppe erfassen. Geprüftes Original-SAND: MCD 13 → Frame[0] 15 → 015.png; MCD 15 → Frame[0] 17 → 017.png. MCD- und PNG-Nummern sind nicht austauschbar. Technischer Inspektor zeigt Zuordnung; mehrere MCD können ein Bild teilen.

Ablauf:
1. Originalsprite oder bestätigte Vorlage mit Baugruppenaufnahme verwenden. Original erhalten.
2. Für getreue Pixelgrafik 32×40 mit nächstem Nachbarn auf 512×640 skalieren. Das vergrößert Pixel, schafft keine Details. HD-Neuzeichnung oder unterstützte Verbesserung schafft Details, muss Hülle/Übergänge erhalten.
3. Geometriereferenz, Alphamaske und Material/Details auf getrennten Ebenen. Bestätigte Vorlagen ohne Änderung von Silhouette/Alpha texturieren. Neue Geometrie getrennt prüfen.
4. Transparentes RGBA-PNG ohne deckenden Hintergrund exportieren. Ganze Leinwand und dreistelligen Namen erhalten. Gemalte Schatten, Ränder, halbtransparente Pixel auf hellem/dunklem Grund prüfen.
5. Im Testordner ablegen, allein, mit Nachbarn, wiederholt, auf mehreren Z-Ebenen und zerstört prüfen. Schönes Einzelbild kann sichtbare Baugruppenfugen erzeugen.

Animation: Workshop nutzt das erste MCD-Bild für Gelände-PNG, nicht alle Spielzyklen. Weitere Bilder/Richtungen im Spiel mit aktivem Anbieter prüfen.

Texturen reparieren keine falschen Hänge/Übergänge. Beabsichtigte Leerräume nicht füllen oder funktionalen Floor nur für die Grafik in Object verwandeln.

## 5. Ordner und erster PNG-Test

Für eigenen Test MyHDWorkshop außerhalb geschützter Mods erstellen:

MyHDWorkshop/
  Resources/TFTD_HD/Terrain/SAND/015.png

SAND ist der Datensatz, 015 das Bild. Ressourcen > Manueller HD-Mod: Hauptordner wählen. F6: Universelle Vorlagen/manuellen Anbieter wählen. Karte mit SAND MCD 13 öffnen. PNG ersetzt dessen erstes Bild; fehlt es, wird Legacy verwendet. REAL-HD-Sandmaterial nutzt anderen Pfad: Kapitel 6.

Ordner funktioniert als Workshop-Grafikanbieter auch ohne aktivierten Spielmod. Für echten OXCE-Mod metadata.yml mit eindeutiger id, name, version, author und master: xcom2 ergänzen; nach Motorregeln aktivieren. Beispiel:

id: my_hd_workshop
name: My HD Workshop
version: 0.1.0
author: Ihr Name
master: xcom2

Metadaten benennen Mod; sie binden nicht allein einen Grafikanbieter in jeder OXCE-Version an. Angepassten REAL-HD-Motor und Ressourcenpriorität im Spiel prüfen. Technische IDs/Namen stabil halten, Anzeigen getrennt übersetzen.

Workshop PNG Remastered prüft zuerst den Quellmod des Teils, dann TFTD PNG remastered unter Resources/TFTD_HD/Terrain/<dataset>/<frame>.png. Nicht alle installierten Mods werden beliebig durchsucht. Manueller Anbieter gibt ausdrücklichen Testpfad unabhängig von logischen Profilen.

Eigene Struktur:
Resources/ = fertige Laufzeitdateien
Sources/ = bearbeitbare Ebenen/Projekte
References/ = Aufnahmen und Datensatz/MCD/Bildnotizen
Documentation/ = Baugruppen/Prüfung

Im gemeinsamen Mod dessen Regeln beachten: fertige PNG in Terrain/<dataset>, Quellen/Nachweise in Datasets/<dataset>, stabile Dokumentation. GEO nutzt Terrain/00_geo_terrain und Datasets/GEO_TERRAIN. Keine neuen Präfixe je Revision, keine bestätigten Vorlagen zum Test ersetzen.

Nach PNG-Bearbeitung Workshop neu starten, um Bildcache sicher zu leeren. Externe Änderungen werden nicht live überwacht. Workshop-Vorschau bestätigt keine Spielpriorität oder Kollision.

## 6. REAL HD: Geometrie und Materialien

REAL HD ist mehr als Spritevergrößerung. Darstellung erzeugt Flächen aus Karte, Profilen und Geometrie und legt Materialien darauf. OXCE behält Simulation: Kollision, Bewegung, Schüsse, Sicht, Türen und Zerstörung. Textur bestimmt diese Regeln nicht.

Aktive SAND-Ressourcen:
user/mods/TFTD_REAL_HD_TEXTURES/Resources/TFTD_HD/RealHD/Datasets/SAND/Materials/
  TOP_BASE.png
  VERTICAL_BASE.png
  TOP_NORMAL_DX.png
  TOP_ROUGHNESS.png
  TOP_AO.png

TOP_BASE: Farbe oben, kein isometrisches Teil-PNG. VERTICAL_BASE: vertikale Flächen. NORMAL_DX: Licht-Mikrodetails nach DirectX-Konvention; keine Silhouetten-/Kollisionsänderung. ROUGHNESS: Rauheit (üblicherweise dunkel=glatt, hell=rau; aktuellen Shader prüfen). AO: lokale Abschattung, kein Ersatz für gesamte Beleuchtung. Einschlagsmasken, BlastSets und Staub sind getrennte Effekte.

Materialien wiederholen sich auf Flächen, keine 512×640-Spriteleinwände. Nahtlose Vierkanten-Texturen bei einheitlicher Größe verwenden. Ausgangsmaße/-konventionen erhalten. Normalmap ist Daten, kein umzufärbendes Foto. Starke gemalte gerichtete Schatten vermeiden, wenn Motor Licht berechnet.

Erster Versuch: SAND-Materialdateien ins Arbeitsverzeichnis kopieren. Nur TOP_BASE bearbeiten, übrige Karten/Namen erhalten. Workshop liest beide Basen aus ausdrücklich benanntem TFTD_REAL_HD_TEXTURES; manueller PNG-Anbieter leitet Materialien nicht um. Für Vorschau aktuelle Materie wiederherstellbar sichern, nur gewünschte Datei dort ersetzen, neu starten. Bei Bedarf zurückstellen. Anleitung ersetzt selbst nichts. Beliebiger REAL-HD-Materialordner ist noch nicht auswählbar.

F6 > REAL HD zeigt SAND/DEBRIS-Geometrie und Basen, nicht ganzen Motor: nicht dieselben Shader, Beleuchtung, Wasser, Kaustik oder dauerhaften Effekte wie P2ZJ. Normalmaps/Rauheit/AO im aktuellen Spielmotor prüfen; Version/Mods/Werte notieren.

Für weitere Familien reicht ähnlicher Ordner nicht: Darstellungsproduzent und Auflösung müssen existieren. SEA/Wasser hat eigene Verträge/Werte; Einheiten, HUD und Effekte andere Anbieter. SAND-Änderung erzeugt keine Rumpf-, Einheiten- oder Wassermaterialien.

Gemischte Darstellung (2.12.7): REAL HD Textur oder REAL HD Debug wählen, dann PNG zusammen mit REAL HD → PNG Remastered oder Universelle Vorlagen. SAND/DEBRIS und GEO_TERRAIN behalten REAL HD Geometrie; andere Teile nutzen den ausgewählten PNG Anbieter und bei fehlender PNG die Legacy Darstellung. Die PNG Auswahl wird gespeichert. GEO nutzt SAND TOP_BASE und VERTICAL_BASE des normalen oder Debug Mods. Geometrie und logische Daten bleiben unverändert.

## 7. Prüfen und Fehler beheben

PNG unsichtbar: F6, manuellen Ordner, Datensatz, Frame[0], dreistelligen Namen 000.png, echte .png-Endung, Lesbarkeit und nicht leeres Alpha prüfen. Legacy-Fallback beweist kein geladenes PNG. Nach externer Bearbeitung zum Leeren des Cache neu starten.

Teil groß/verschoben/abgeschnitten: Leinwand, Anker, Maße prüfen; 512 Breite, 640 Höhe. Auf Beschnitt, Streckung und dokumentierte Erweiterungen achten.

Nähte: echte MAP-/MCD-Nachbarn zusammensetzen, Kanten/Richtung/Wiederholung prüfen. Profilfehler nicht mit Pflanzen/Schatten verbergen.

Unvollständiger Felsen: ganze Katalogeinheit prüfen, Fragmente nicht eigenständig. Mehrfeldblock in Höhe braucht gleiche horizontale Unterlage auf ganzer Fläche.

Magenta-REAL-HD-Raster: TFTD_REAL_HD_TEXTURES, TOP_BASE/VERTICAL_BASE, MODS-Pfad, PNG-Lesbarkeit prüfen. Sprites umbenennen ersetzt keine fehlende Normalmap.

MAP-Index nicht aufgelöst: richtiges Quellprofil laden. MAP nutzt 0=leer, 1..255=Indizes der Datensatzliste; zufälliges Ergänzen erzeugt Geisterteile. Normalisierungshinweis verstehen, bevor bestätigen.

Falsche Kollision/Wege trotz richtigem Bild: MCD, Patches, Schicht, Höhe, BigWall, Zerstörung prüfen, OXCE testen. Anmerkungen erzeugen keine Bewegungsregeln.

Vier Prüfstufen: erzeugte Datei → Softwareprüfung → Workshop-Baugruppe → OXCE-Mission und Nutzerfeedback. Jeweils Version, Ressourcen, Startwert/Karte und Aufnahmen behalten. Licht/Wasser, Entdeckung/Verdeckung, Animation, Türen, Zerstörung, Z-Ebenen prüfen. Exporterfolg oder Einzelbild bestätigt kein Spielverhalten.

## 8. Teilen und beitragen

Künftiges Repository kann Quellen, Bauwerkzeuge, Tests, Übersetzungen und Anleitung enthalten. Lieferung bleibt lokal; kein Repository erstellt/veröffentlicht. TFTD-/OXCE-Daten und Projekt-PNG nicht im Codepaket; Nutzer richtet Pfade unter Ressourcen ein.

Übersetzungen: locales/fr.json, en.json, es.json, de.json. Jede Kennung verweist auf Quelltext in sources.json. Technische Ordner, MCD-/MAP-IDs, Formate, Tastenkürzel nicht übersetzen. %ls, %d, %u und weitere Platzhalter in gleicher Reihenfolge erhalten. Generator prüft und erstellt workshop_i18n_data.h vor Kompilieren. Neue Sprache braucht vollständigen Katalog und Menüeintrag. Oberfläche übersetzt; eigene Namen, Dateinamen und Vorlagen-IDs unverändert. Muttersprachliche Prüfung kann Stil verbessern, ohne Parameter zu ändern.

Anleitung: docs/chapters.json enthält vier Sprachen. Werkzeuge erzeugen Offline-Seiten und Daten des eingebauten Lesers. Version/Grenzen bei Funktionsänderungen aktualisieren. Reproduzierbare Beispiele statt pauschaler Kompatibilitätsbehauptung.

Fehler melden mit Workshop-/Motorversion, Modus, Datensatz/MCD/Bild, Karte/Startwert, erwartetem/beobachtetem Ergebnis und Baugruppenaufnahme. Texturbeitrag: Maße/Alpha, Geometriequelle, Enddateien und tatsächliche Workshop-/Spielprüfungen. Gültige PNG bestätigen keine neuen Vorlagen.

Schrittweise lernen: einzelner Boden → vier Nachbarn → Mehrfeldbaugruppe → Karte → Mission. Eine Änderung je Versuch zeigt ihre Wirkung.

Kontakt Benjamin: colmoutarde57700@gmail.com

Thanks to GPT-6 Sol

Spenden zur Unterstützung des Projekts sind freiwillig.

PayPal Konto: col.moutarde@hotmail.fr
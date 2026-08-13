# NotePropertiesBar

Die `NotePropertiesBar` ist eine kompakte Eigenschaftenleiste für die im Piano-Roll ausgewählten MIDI-Noten. Sie befindet sich zwischen der Piano-Roll-Kopfzeile und der Timeline und ist 30 Pixel hoch.

## Zweck

Die Leiste ermöglicht die gemeinsame Anzeige und Bearbeitung folgender Eigenschaften:

| Feld | Bedeutung | Anzeige/absolute Eingabe | Relative Eingabe |
|---|---|---|---|
| `START` | Globale Startposition | `Takt.Schlag.Tick`, z. B. `6.2.240` | `+1/16`, `-120 ticks` |
| `END` | Globale Endposition | `Takt.Schlag.Tick` | `+1/16`, `-120 ticks` |
| `DURATION` | Notenlänge | Notenwert wie `1/4` oder `960 ticks` | `+1/16`, `-120 ticks` |
| `PITCH` | MIDI-Tonhöhe | MIDI-Nummer oder Notenname wie `C3`, `G#4`, `Bb2` | `+1 st`, `-12 st` |
| `VELOCITY` | Anschlagstärke | Ganzzahl | `+5`, `-10` |

Links wird zusätzlich die Anzahl der ausgewählten Noten angezeigt.

## Mehrfachauswahl

Die Auswahl wird als Liste von Paaren aus `te::MidiClip*` und `te::MidiNote*` bereitgestellt. Dadurch kann die Leiste Noten aus mehreren MIDI-Clips gleichzeitig bearbeiten und clipinterne Positionen korrekt in globale Projektpositionen umrechnen.

Haben alle ausgewählten Noten für ein Feld denselben Wert, wird dieser Wert angezeigt. Bei unterschiedlichen Werten erscheint ein Gedankenstrich (`—`). Eine Eingabe in ein gemischtes Feld setzt den neuen Wert für alle ausgewählten Noten.

Ohne ausgewählte Noten sind alle Felder deaktiviert und zeigen `—`.

## Bedienung

### Texteingabe

Die Felder sind standardmäßig schreibgeschützt und für Scrubbing vorbereitet.

- Doppelklick: Texteingabe öffnen.
- `Enter` oder `F2` bei fokussiertem Feld: Texteingabe öffnen.
- `Enter`: Wert übernehmen.
- `Escape`: Änderung verwerfen.
- `Tab` / `Shift+Tab`: Wert übernehmen und zum nächsten/vorherigen Feld wechseln.
- Fokusverlust: gültigen Wert übernehmen; ungültige Eingabe auf den vorherigen Anzeigewert zurücksetzen.

Ungültige Eingaben werden rot markiert, solange die Bearbeitung aktiv bleibt.

### Scrubbing

Schreibgeschützte Felder können ohne Texteingabe verändert werden:

- Mausrad: einen Schritt erhöhen oder verringern.
- Vertikales Ziehen: ein Schritt je vier Pixel; die Mausbewegung wird während des Ziehens nicht durch den Bildschirmrand begrenzt.

Schrittweiten:

- Start, Ende und Dauer: `1/16` pro Schritt.
- Tonhöhe: ein Halbton pro Schritt.
- Velocity: eine Einheit pro Schritt.

Ein zusammenhängender Drag wird als eine Undo-Operation behandelt. Mausrad- und Texteingaben beginnen jeweils eine passende Undo-Transaktion.

## Eingabeformate und Validierung

### Positionen

Absolute Start- und Endpositionen verwenden `Takt.Schlag.Tick`. Takt und Schlag sind einsbasiert, Ticks nullbasiert. Bei `te::Edit::ticksPerQuarterNote == 960` gilt beispielsweise:

- `6` entspricht `6.1.000`.
- `6.2` entspricht `6.2.000`.
- `6.2.240` ist ein vollständiger Wert.
- Tickwerte müssen zwischen `0` und `959` liegen.

Startpositionen dürfen nicht vor Projektbeginn liegen. Eine absolute Endposition muss nach dem Start jeder ausgewählten Note liegen.

### Dauer

Notenwerte werden als Bruch eines ganzen Taktnotenwerts interpretiert:

- `1/1` = vier Viertelnoten
- `1/2` = zwei Viertelnoten
- `1/4` = eine Viertelnote
- `1/8` = eine halbe Viertelnote

Alternativ sind positive Tickwerte mit dem Suffix `ticks` möglich. Dauern müssen nach der Änderung größer als null sein.

Für bekannte binäre Notenwerte von `1/1` bis `1/128` wird der Bruch angezeigt. Andere Längen werden gerundet als Tickwert dargestellt.

### Tonhöhe

Absolute Tonhöhen akzeptieren MIDI-Nummern von `0` bis `127` sowie Notennamen mit optionalem `#` oder `b` und einer Oktave. Relative Änderungen benötigen das Suffix `st`. Ergebnisse werden auf den MIDI-Bereich `0..127` begrenzt.

### Velocity

Velocity wird auf `1..127` begrenzt. Ein führendes `+` oder `-` kennzeichnet eine relative Änderung; eine vorzeichenlose Zahl setzt einen absoluten Wert.

## Positionsumrechnung

MIDI-Noten speichern ihre Startposition relativ zum Clipinhalt. Die Leiste zeigt dagegen globale Projektpositionen.

Globale Startposition in Beats:

```text
clip.start + note.start - clip.offset
```

Rückrechnung auf die clipinterne Startposition:

```text
globalStart - clip.start + clip.offset
```

Für die Umrechnung zwischen Beats und `Takt.Schlag.Tick` verwendet die Komponente die Tempo-Sequenz des Edits und die gemeinsamen Funktionen aus `PositionDisplayHelpers`.

## Auswahl- und Modell-Synchronisation

`PianoRollEditor` installiert einen `SelectionProvider`, der ausschließlich noch vorhandene Noten aus den gecachten MIDI-Clips zurückliefert. Dadurch gelangen nach dem Löschen keine veralteten Notenzeiger in die Leiste.

Aktualisierungen erfolgen bei:

- Änderungen an `te::SelectedMidiEvents`, die vom `MidiViewport` weitergereicht werden,
- Änderungen am globalen `SelectionManager`,
- Änderungen an NOTE-Properties,
- dem Entfernen einer Note,
- Track-Wechsel oder Schließen des aktuellen Tracks,
- Theme-Änderungen.

Eine laufende Texteingabe wird nur verworfen, wenn sich die tatsächliche Notenauswahl geändert hat. Reine Tool- oder verzögerte Änderungsnachrichten überschreiben den eingegebenen Text nicht.

## Layout und Theme

Die Feldbreiten orientieren sich an stabilen Referenzwerten. Ist weniger Platz verfügbar, werden alle Eigenschaftsfelder proportional verkleinert. Dadurch bleiben auch `PITCH` und `VELOCITY` erreichbar, statt eine Breite von null zu erhalten.

Farben stammen aus `ApplicationViewState`. Labels verwenden eine reduzierte Deckkraft, deaktivierte Felder werden abgeschwächt und ungültige Werte rot dargestellt.

## Öffentliche Schnittstelle

```cpp
explicit NotePropertiesBar(EditViewState&);
void setSelectionProvider(SelectionProvider);
void refreshFromSelection(bool discardActiveEdit = false);
void clearSelection();
void updateColours();
```

- `setSelectionProvider`: verbindet die Komponente mit der aktuellen MIDI-Auswahl.
- `refreshFromSelection`: liest Auswahl und Werte neu ein.
- `clearSelection`: deaktiviert alle Felder und entfernt gespeicherte Auswahlreferenzen.
- `updateColours`: übernimmt das aktuelle Theme.

## Dateien

- Deklaration: `App/include/NotePropertiesBar.h`
- Implementierung: `App/src/NotePropertiesBar.cpp`
- Einbindung: `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp`

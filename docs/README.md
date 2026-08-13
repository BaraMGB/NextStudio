# NextStudio-Dokumentation

Dieser Ordner enthält technische Dokumentation zu Architektur, Komponenten und größeren Änderungen.

## Inhalte

- [NotePropertiesBar](note-properties-bar.md) – Bedienung, Datenmodell, Validierung und Integration der MIDI-Noteneigenschaftenleiste.
- [Änderungen: NotePropertiesBar und Positionsanzeige](changes/note-properties-bar-and-position-display.md) – vollständige Übersicht der zusammengehörigen Codeänderungen.

## Relevante Quellbereiche

- Piano-Roll: `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp`
- MIDI-Auswahl: `App/include/MidiViewport.h`, `App/src/MidiViewport.cpp`
- Noteneigenschaften: `App/include/NotePropertiesBar.h`, `App/src/NotePropertiesBar.cpp`
- Positionsformatierung: `App/include/PositionDisplayHelpers.h`, `App/src/PositionDisplayHelpers.cpp`
- Tests: `App/tests/PositionDisplayTests.cpp`

# Gemeinsamer eingebetteter Projekt-Dateibrowser

## Zusammenfassung

Projekt Load und Save As erzeugen keine modalen Top-Level-Dateidialoge. Der Projects-Tab zeigt dauerhaft einen gefilterten Verzeichnisbrowser. Home und Projects verwenden dieselbe `DirectoryBrowserComponent`; projektspezifische Lifecycle-Entscheidungen bleiben in `ProjectsBrowserComponent`.

## Motivation

Die frühere Lösung besaß drei überlappende Darstellungen:

- den navigierbaren Home-Dateibrowser;
- eine flache rekursive Projektliste;
- einen separaten eingebetteten Load-/Save-As-Browser.

Der Load-Button wechselte nur zwischen zwei Projektlisten. Zusätzlich waren Sample Preview und Projektanfragen direkt in Browserklassen gekoppelt. Die neue Struktur trennt Verzeichnisnavigation, Dateiauswahl und Projektworkflow.

## Architektur

### DirectoryBrowserComponent

`App/include/DirectoryBrowser.h` und `App/src/DirectoryBrowser.cpp` enthalten die gemeinsame Navigation:

- asynchroner Directory-Scanner;
- Pfadfeld und Aufwärtsnavigation;
- Vor-/Zurück-Verlauf;
- Sortierung;
- Suche;
- Datei-Filtercallback;
- Auswahl-, Aktivierungs- und Verzeichniscallbacks;
- konfigurierbare Drag-Source-Beschreibung.

Die Komponente hat keine Abhängigkeit von Engine, Edit, Sample Preview oder Project Workflow.

### Home

`FileBrowserComponent` ist eine dünne Home-Konfiguration des gemeinsamen Browsers. `SidebarComponent` verbindet die Dateiauswahl mit `SamplePreviewComponent`. Damit verbleiben Audioformatprüfung, Preview Edit und BPM-Synchronisation in der editgebundenen Preview-Komponente.

Doppelklick auf eine persistente Projektdatei ruft direkt `MainComponent::requestProjectOperation()` auf. Der frühere `ProjectRequestState`-/`ChangeBroadcaster`-Adapter entfällt.

### Projects

`ProjectsBrowserComponent` komponiert den gemeinsamen Browser und ergänzt:

- New;
- Save;
- Save As;
- Dirty-State-Entscheidung;
- Overwrite-Bestätigung;
- Saving/Committing;
- Inline-Fehler;
- Pending New/Load/Quit;
- Interaction Lock.

Verzeichnisse bleiben sichtbar. Dateien werden über `ProjectLifecycle::isPersistentProjectFile()` gefiltert, einschließlich abweichender Groß-/Kleinschreibung der Endung.

## Bedienung

### Laden

Es gibt keinen Load-Button und keinen Load-Modus. Der Projects-Tab ist bereits ein vollständiger Verzeichnisbrowser. Ein Doppelklick auf eine Projektdatei erzeugt eine typisierte Load-Operation.

Das Browsen selbst bleibt nicht-modal. Erst wenn eine konkrete Operation nach Clean-, Save- oder Discard-Entscheidung ausgeführt wird, beginnt der Lock.

### Save

Hat das aktuelle Edit einen persistenten Pfad, wird dieser exakt erhalten. Insbesondere wird `Song.TRACKTIONEDIT` nicht in `Song.tracktionedit` umgeschrieben.

Ein temporäres Projekt wechselt zu Save As.

### Save As

Der Projects-Tab behält den Verzeichnisbrowser sichtbar und ergänzt:

- Projektname;
- normalisierten vollständigen Zielpfad;
- Save und Cancel;
- Overwrite-Bestätigung;
- Inline-Fehler.

Der aktuelle Browserpfad bestimmt den Zielordner. Neue Ziele erhalten genau eine kanonische `.tracktionedit`-Endung.

## Workflow

`ProjectWorkflow::Controller` besitzt die Zustände:

- `normal`;
- `saveProjectAs`;
- `confirmOverwrite`;
- `saving`;
- `committing`;
- `operationError`;
- `confirmUnsavedChanges`.

Load bleibt als `OperationType::load` erhalten, ist aber kein UI-Zustand mehr.

Pending Operations unterscheiden:

- `createNew`;
- `load`;
- `quit`.

Sie überleben einen erforderlichen Save-As-Vorgang und werden ausschließlich nach erfolgreichem Schreiben fortgesetzt. Ein Schreibfehler verwirft Operation und Fortsetzungszustand; ein anschließender Save-As-Retry speichert nur und führt kein altes New, Load oder Quit mehr aus.

## Dirty-State-Sicherheit

Beim Commit wird die Interaktion gesperrt. Das stoppt Transport und MIDI, gibt den Playback Context frei, deaktiviert Editor, Lower Range, Pluginfenster und Kommandos und trennt das Computer-MIDI-Keyboard.

Ein vor dem Lock als clean erkanntes Edit wird nach dem Lock erneut geprüft. `MainComponent::executeProjectOperation()` erfasst zusätzlich Edit-Identität und `lastSignificantChange` und vergleicht beides unmittelbar vor der asynchronen Ausführung.

## Fehlerbehandlung

- Ungültige oder verschwundene Projektdateien lassen das aktuelle Edit aktiv.
- Fehler erscheinen inline im Projects-Tab.
- Save-Fehler verwerfen Pending Operation und Fortsetzungszustand.
- Save As sichert und restauriert bei Fehlern Editname, Editpfad und die exakten Clip-/SoundFont-Pfadwerte.
- Cancel löscht Pending Operation und Lock.

## Persistenz und Crash Recovery

Der zuletzt angezeigte Projektbrowserpfad wird in `m_projectLoadDir` weiterverwendet, damit bestehende Einstellungen kompatibel bleiben. Der frühere separate Save-Ordnerzustand entfällt; der sichtbare Browserpfad ist die einzige Quelle für Load und Save As. Ein Wechsel des Content Root setzt ihn auf das neue Projects-Verzeichnis.

Eine vorhandene Crash-Recovery-Datei wird vor einem gegebenenfalls erforderlichen Setup-Wizard angeboten. Sie wird nicht hinter dem Wizard geladen oder bis zu dessen Abschluss zurückgestellt, damit ein Beenden während des Setups keine noch nicht angebotene Recovery-Datei löscht.

## Relevante Dateien

- `App/include/DirectoryBrowser.h`
- `App/src/DirectoryBrowser.cpp`
- `App/include/FileBrowser.h`
- `App/src/FileBrowser.cpp`
- `App/include/ProjectsBrowser.h`
- `App/src/ProjectsBrowser.cpp`
- `App/include/ProjectWorkflow.h`
- `App/src/ProjectWorkflow.cpp`
- `App/src/SidebarComponent.cpp`
- `App/src/MainComponent.cpp`

## Validierung

- `ProjectLifecycleTests` prüfen Filter- und Endungsregeln.
- `ProjectWorkflowTests` prüfen Commit, Discard, Save-As-Fortsetzung, Abbruch nach Schreibfehlern, asynchrone Execution Guards, Cancel und Fehlerzustände.
- `ProjectLifecycleTests` prüfen zusätzlich die exakte Wiederherstellung temporär veränderter Pfad-Properties.
- Der vollständige Build und alle CTests müssen erfolgreich sein.
- Debug-Shell-Screenshots prüfen normale Projects-Ansicht und Save-As-Darstellung visuell.

## Ergebnis

Der Projects-Tab enthält nur noch die fachlich notwendige Workflow-Schicht. Navigation und Dateidarstellung werden mit Home geteilt. Dadurch entfallen Load-Button, separater Load-Modus, rekursive Normalprojektliste und doppelte Scanner-/Navigationslogik.

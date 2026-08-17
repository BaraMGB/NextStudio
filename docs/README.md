# NextStudio Documentation

This directory contains the user and developer documentation for NextStudio.

## User documentation

- [Getting started](user/getting-started.md) — first launch, content folders, projects, the main window, and basic workflow.
- [Piano roll](user/piano-roll.md) — selecting MIDI material, editing notes, tools, navigation, velocity, and shortcuts.
- [Peak Limiter](user/peak-limiter.md) — controls, metering, suggested settings, and limitations.

## Component documentation

- [NotePropertiesBar](components/note-properties-bar.md) — behavior, input formats, validation, selection handling, undo, and Piano Roll integration.

## Architecture

- [Architecture overview](architecture/overview.md) — process lifetime, ownership, major UI areas, model boundaries, and external libraries.
- [State and event model](architecture/state-and-events.md) — application state, edit-local view state, Tracktion model state, selection, listeners, and asynchronous refreshes.
- [Playback graph reallocation inhibition](architecture/playback-graph-reallocation.md) — Tracktion's `ReallocationInhibitor`, delayed graph rebuilds, lifetime rules, limitations, and NextStudio's bulk clip-edit usage.
- [Project lifecycle](architecture/project-lifecycle.md) — new/load/save/save-as, unsaved changes, autosave, recovery, validation, and teardown order.

## Development

- [Building](development/building.md) — prerequisites, submodules, build types, scripts, output locations, and packaging.
- [Testing](development/testing.md) — test targets, commands, current coverage, and adding tests.
- [Source layout](development/source-layout.md) — repository map, naming conventions, source registration, resources, and common extension points.

## Change documentation

Change documents explain a coherent implementation diff rather than acting as release notes. User-facing release history remains in [`CHANGELOG.md`](../CHANGELOG.md).

- [Note properties bar and position display](changes/note-properties-bar-and-position-display.md)

## Documentation conventions

- Documentation is written in English.
- Paths and commands are relative to the repository root unless stated otherwise.
- Public build instructions use the portable defaults of the scripts. `BUILD_JOBS=<count>` may be used to tune parallelism for a specific machine.
- User documentation describes observable behavior. Component and architecture documents may refer directly to C++ classes and source files.
- Implementation-specific claims should include the relevant source paths so they can be verified when the code changes.

## Primary source areas

| Area | Main files |
|---|---|
| Application lifetime | `App/src/Main.cpp`, `App/include/MainComponent.h`, `App/src/MainComponent.cpp` |
| Persistent application settings | `App/include/ApplicationViewState.h` |
| Edit-local UI state | `App/include/EditViewState.h`, `App/src/EditViewState.cpp` |
| Arrangement editor | `App/include/EditComponent.h`, `App/src/EditComponent.cpp` |
| Lower editor area | `App/include/LowerRangeComponent.h`, `App/src/LowerRangeComponent.cpp` |
| Piano Roll | `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp`, `App/include/MidiViewport.h`, `App/src/MidiViewport.cpp` |
| MIDI note properties | `App/include/NotePropertiesBar.h`, `App/src/NotePropertiesBar.cpp` |
| Position formatting | `App/include/PositionDisplayHelpers.h`, `App/src/PositionDisplayHelpers.cpp` |
| Project lifecycle helpers | `App/include/ProjectLifecycle.h`, `App/src/ProjectLifecycle.cpp` |
| Tests | `App/tests/` |

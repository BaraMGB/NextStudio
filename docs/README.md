# NextStudio Documentation

This directory contains the user and developer documentation for NextStudio.

## User documentation

- [Getting started](user/getting-started.md) — first launch, content folders, projects, the main window, and basic workflow.
- [Piano roll](user/piano-roll.md) — selecting MIDI material, editing notes, tools, navigation, velocity, and shortcuts.
- [Peak Limiter](user/peak-limiter.md) — controls, metering, suggested settings, and limitations.

## UI documentation

- [Header Bar](ui/header-bar.md) — transport controls, time display, global switches, automation controls, pre-roll counter.
- [Side Browser](ui/side-browser.md) — projects, instruments, effects, samples, home, settings, render, sample preview.
- [Song Editor](ui/song-editor.md) — track list, arrangement area, toolbar, clip properties bar, footer bar, navigation.
- [Mixer](ui/mixer.md) — channel strips, master channel, level meters, navigation.
- [Track Chain](ui/track-chain.md) — modifier stack, plugin chain sections, channel strip, MIDI learn, automation lanes.
- [Keyboard Shortcuts](ui/shortcuts.md) — transport, global editing, song editor, track list, MIDI editor, virtual MIDI keyboard.

## Plugin documentation

- [Arpeggiator](plugins/arpeggiator.md) — MIDI arpeggiator with mode, rate, octave, and gate controls.
- [SoundFont Player](plugins/soundfont-player.md) — `.sf2` sample player using TinySoundFont engine.
- [Simple Synth](plugins/simple-synth.md) — two-oscillator subtractive synthesizer with unison, filter, and mono/portamento.
- [Drum Sampler](plugins/drum-sampler.md) — 16-pad drum sampler with drag & drop and sound editor.
- [Volume & Pan](plugins/volume-pan.md) — utility volume and pan control.
- [Spectrum Analyzer](plugins/spectrum-analyzer.md) — real-time FFT frequency analyzer (pass-through).
- [EQ](plugins/eq.md) — 4-band parametric equalizer with interactive frequency response graph.
- [Compressor](plugins/compressor.md) — dynamic compressor with sidechain support and transfer curve.
- [Filter](plugins/filter.md) — state-variable filter with lowpass/highpass and selectable slope.
- [Delay](plugins/delay.md) — versatile delay with sync, multiple modes, and feedback filters.
- [Reverb](plugins/reverb.md) — algorithmic reverb based on JUCE reverb engine.
- [Chorus](plugins/chorus.md) — stereo chorus with modulated delay line.
- [Phaser](plugins/phaser.md) — stereo phaser with adjustable feedback and sweep graph.
- [Saturation](plugins/saturation.md) — saturation/distortion with multiple modes, oversampling, and tone filter.

## Component documentation

- [Piano Roll Editor](components/piano-roll-editor.md) — component hierarchy, data model, coordinate conversion, rendering, hit testing, tool architecture, and note-editing operations.
- [NotePropertiesBar](components/note-properties-bar.md) — behavior, input formats, validation, selection handling, undo, and Piano Roll integration.
- [ClipPropertiesBar](components/clip-properties-bar.md) — arrangement clip fields, preview/commit flow, snapping, insertion length, and selection integration.
- [PluginChainView](components/plugin-chain-view.md) — component structure, panel persistence, rack layout, ordering, scrolling, drag-and-drop, and refresh model.

## Architecture

- [Architecture overview](architecture/overview.md) — process lifetime, ownership, major UI areas, model boundaries, and external libraries.
- [State and event model](architecture/state-and-events.md) — application state, edit-local view state, Tracktion model state, selection, listeners, and asynchronous refreshes.
- [Playback graph reallocation inhibition](architecture/playback-graph-reallocation.md) — Tracktion's `ReallocationInhibitor`, delayed graph rebuilds, lifetime rules, limitations, and NextStudio's bulk clip-edit usage.
- [Central clip overwrite command](architecture/clip-overwrite-command.md) — incoming-wins planning, selective victim trimming, atomic commit, undo, and entry points.
- [Project lifecycle](architecture/project-lifecycle.md) — new/load/save/save-as, unsaved changes, autosave, recovery, validation, and teardown order.

## Development

- [Building](development/building.md) — prerequisites, submodules, build types, scripts, output locations, and packaging.
- [Testing](development/testing.md) — test targets, commands, current coverage, and adding tests.
- [Source layout](development/source-layout.md) — repository map, naming conventions, source registration, resources, and common extension points.
- [Wine/Bottles compatibility](development/wine-bottles.md) — JUCE 8 renderer issues under Wine, runtime fallbacks, Bottles test workflow, and current limitations.
- [Agent debug system](agent-debug.md) — debug-shell protocol, pi tools, isolated sessions, deterministic editing commands, artifacts, and tests.
- [Logging](logging.md) — central logger API, categories, levels, output policy, and migration rules.

## Change documentation

Change documents explain a coherent implementation diff rather than acting as release notes. User-facing release history remains in [`CHANGELOG.md`](../CHANGELOG.md).

- [Note properties bar and position display](changes/note-properties-bar-and-position-display.md)
- [Piano Roll MIDI key lighting](changes/piano-roll-midi-key-lighting.md) — routed live-MIDI event flow, active-key state, PrimeColour rendering, batching behavior, and lifecycle.
- [Computer MIDI keyboard controller](changes/computer-midi-keyboard-controller.md) — command removal, dedicated JUCE keyboard-state handling, plugin-window integration, tests, and the Linux/JUCE latency bug analysis.

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
| Arrangement clip properties | `App/include/ClipPropertiesBar.h`, `App/src/ClipPropertiesBar.cpp` |
| Lower editor area | `App/include/LowerRangeComponent.h`, `App/src/LowerRangeComponent.cpp` |
| Piano Roll | `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp`, `App/include/MidiViewport.h`, `App/src/MidiViewport.cpp` |
| MIDI note properties | `App/include/NotePropertiesBar.h`, `App/src/NotePropertiesBar.cpp` |
| Position formatting | `App/include/PositionDisplayHelpers.h`, `App/src/PositionDisplayHelpers.cpp` |
| Project lifecycle helpers | `App/include/ProjectLifecycle.h`, `App/src/ProjectLifecycle.cpp` |
| Agent debug system | `App/include/Debug*.h`, `App/src/Debug*.cpp`, `.pi/extensions/nextstudio-debug.ts`, `tools/debug-shell-client.js` |
| Tests | `App/tests/` |

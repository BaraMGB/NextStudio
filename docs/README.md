# NextStudio documentation

## Start with your task

| Task | Entry |
|---|---|
| Use the application | [Getting started](user/getting-started.md), [Piano Roll](user/piano-roll.md) |
| Look up a screen/control/plugin | UI and plugin references below |
| Understand implementation | Architecture and component references below |
| Build, test or debug | [Building](development/building.md), [Testing](development/testing.md), [Agent debug](agent-debug.md) |
| Make a change | [Contributing](development/contributing.md), [Documentation maintenance schema](development/documentation-policy.md) |
| Consult history or planned work | [Historical records](archive/README.md), [Active plans](plans/README.md), [Open-work queue](../Todo.md) |

Current references describe the repository revision containing them. Historical records describe past designs/results; they are not prerequisites for understanding current behavior. User-visible release history belongs in [CHANGELOG](../CHANGELOG.md).

## User workflows

- [Getting started](user/getting-started.md) — first launch, projects, basic track workflow and settings.
- [Piano Roll](user/piano-roll.md) — note editing, tools, velocity and navigation.
- [Peak Limiter](user/peak-limiter.md) — controls, metering, suggested settings and limitations.

## UI reference

- [Header Bar](ui/header-bar.md) — transport, time display, switches and automation controls.
- [Side Browser](ui/side-browser.md) — projects, instruments, effects, samples, settings and render.
- [Song Editor](ui/song-editor.md) — tracks, arrangement, tools, clip properties and navigation.
- [Mixer](ui/mixer.md) — channel strips, master and meters.
- [Track Chain](ui/track-chain.md) — modifiers, plugin chain, channel strip and MIDI learn.
- [Keyboard Shortcuts](ui/shortcuts.md) — transport/editing, editor commands and virtual keyboard.

## Plugin reference

- [Arpeggiator](plugins/arpeggiator.md)
- [SoundFont Player](plugins/soundfont-player.md)
- [Simple Synth](plugins/simple-synth.md)
- [Drum Sampler](plugins/drum-sampler.md)
- [Volume & Pan](plugins/volume-pan.md)
- [Spectrum Analyzer](plugins/spectrum-analyzer.md)
- [EQ](plugins/eq.md)
- [Compressor](plugins/compressor.md)
- [Filter](plugins/filter.md)
- [Delay](plugins/delay.md)
- [Pitch Shifter](plugins/pitch-shifter.md)
- [Reverb](plugins/reverb.md)
- [Chorus](plugins/chorus.md)
- [Phaser](plugins/phaser.md)
- [Saturation](plugins/saturation.md)

## Component contracts

- [Timeline view transform](components/timeline-view-transform.md) — normalized zoom, fits, float geometry and band rendering.
- [Timeline snapping](components/timeline-snapping.md) — discrete/mouse APIs, profiles, inverse anchors, constraints, replay and feedback.
- [Lasso and TimeRange selection](components/selection-gestures.md) — independent gestures, musical anchors, source hit testing and selection lifecycle.
- [Song Editor implementation](components/song-editor.md) — arrangement integration and stationary cursor ownership.
- [Piano Roll Editor](components/piano-roll-editor.md) — ownership, model, tools, MIDI cursors, operations and refresh.
- [Property-field input](components/property-field-input.md) — shared numeric formats, focus/Tab/wheel/scrub conventions and parser differences.
- [NotePropertiesBar](components/note-properties-bar.md) — exact note fields, validation, selection and undo.
- [ClipPropertiesBar](components/clip-properties-bar.md) — clip fields, preview/commit and insertion controls.
- [Metronome settings](components/metronome-settings.md) — managed WAV samples, persistence and tests.
- [Theme settings](components/theme-settings.md) — colors, presets, persistence and tests.
- [PluginChainView](components/plugin-chain-view.md) — composition, rack layout, ordering, bypass and refresh.
- [Built-in effect editors](components/built-in-effect-editors.md) — responsive layouts, Pitch Map gesture/undo, EQ reset and Reverb header.
- [Lower-range layout](components/lower-range.md) — Piano Roll resize/collapse, pitch anchoring and arrangement activation.
- [Computer MIDI keyboard](components/computer-midi-keyboard.md) — held keys, aliases, virtual state, focus and lifetime.
- [Directory browser](components/directory-browser.md) — shared asynchronous Home/Projects navigation and domain integration.

## Architecture

- [Overview](architecture/overview.md) — process lifetime, ownership, model boundaries and libraries.
- [State and events](architecture/state-and-events.md) — application/edit/model state and asynchronous refresh.
- [Playback graph reallocation inhibition](architecture/playback-graph-reallocation.md) — bulk clip edits, guard lifetime and limitations.
- [MIDI input routing and exclusive focus](architecture/midi-input-routing.md) — automatic/manual routes, ownership and migration.
- [Central clip overwrite command](architecture/clip-overwrite-command.md) — incoming-wins planning, atomic commit and undo.
- [Project lifecycle](architecture/project-lifecycle.md) — load/save, unsaved changes, autosave and recovery.
- [Project workflow controller](architecture/project-workflow.md) — continuations, interaction locks and engine suspension.
- [Embedded startup wizard](architecture/startup-wizard.md) — placement, locking, completion and recovery order.

## Development procedures

- [Contributing](development/contributing.md) — mandatory approval, implementation, validation and delivery workflow.
- [Documentation maintenance schema](development/documentation-policy.md) — placement, outlines, update matrix, lifecycle and review checklist.
- [Building](development/building.md) — prerequisites, scripts, outputs and packaging.
- [Testing](development/testing.md) — console/native regressions, documentation checks and coverage boundaries.
- [Source layout](development/source-layout.md) — repository map, registration and extension points.
- [Wine/Bottles compatibility](development/wine-bottles.md) — Windows runtime fallbacks, testing and limitations.
- [Agent debug](agent-debug.md) — isolated shell sessions, commands and artifacts.
- [Logging](logging.md) — API, categories, output policy and ground rules.

Repository documentation is written in English. Commands are relative to the repository root unless stated otherwise. Public build examples retain portable script defaults; project-local agent settings are documented in the contribution workflow. Source-area details belong in the source-layout and subsystem references, not duplicated in this index.

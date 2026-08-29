# Changelog

All notable changes to NextStudio are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Arrangement knife preview** — Hovering a clip with the Knife tool now shows a vertical split preview that follows the arrangement snap setting. The preview and actual split bypass snapping while `Shift` is held.
- **Timeline loop interaction hints** — The loop lane now shows contextual hints and cursors for drawing, moving, and resizing the start or end of a loop range.

### Changed

- **Visible timeline loop lane** — The lower loop-editing lane is identified by a 30%-opaque black overlay. Loop ranges use 50% opacity while looping is enabled and 20% opacity while it is disabled.

### Fixed

- **Stable loop draw cursor** — The loop lane reuses its pencil cursor instead of resetting and recreating it on every pointer movement, preventing cursor flicker.

## [v0.04-alpha] - 2026-08-21

### Added

- **Arrangement clip properties bar** — The Song Editor now provides exact Start, End, and Duration fields for selected clips, including mixed-value display, text entry, wheel stepping, drag scrubbing with a non-destructive preview, and multi-clip edits that preserve selection-relative offsets.
- **Independent arrangement snapping and insertion length** — Arrangement snapping can be Off, Adaptive, or fixed from 1/1 through 1/128. Newly created MIDI clips use a separately configurable Adaptive or fixed insertion length.
- **Piano Roll snap and insert length selectors** — The piano roll now has its own snap selector and insert length selector, giving independent control over grid snapping and note insertion duration.
- **Provisional MIDI note paste** — Pasted MIDI notes are now inserted provisionally, allowing preview before committing. Note properties (velocity, length) are preserved during paste.
- **Live MIDI feedback on piano roll keyboard** — The piano roll keyboard now lights up in real time during playback, showing which notes are currently sounding.
- **Resize selected piano roll notes together** — Selecting multiple notes and resizing now resizes all selected notes as a group.
- **Centralized clip overwrite edits** — Destructive clip overwrite operations are now centralized through a single code path, ensuring consistent behavior across clip automation, range edits, and clip duplication.

### Fixed

- **Piano Roll velocity synchronization** — Velocity changes previewed or committed through the note-properties bar now immediately update the velocity lane.
- **Double-click note selection** — A note inserted with the pointer tool by double-click is now selected and replaces the previous MIDI-note selection.
- **Piano Roll pointer pitch display** — The footer now derives pitch from note-grid-local coordinates, updates after vertical view changes, clears when the pointer leaves the grid, and limits values to MIDI range 0–127. Updates are emitted only when the pointer crosses a note boundary.
- **Playhead click and playback return behavior** — Clicking the playhead no longer triggers unexpected playback jumps. Playback returns to the correct position after reaching the loop end.
- **Piano roll note cursor display** — The note cursor in the piano roll now displays correctly after vertical scroll and view changes.
- **MIDI note overlap handling** — Overlapping MIDI notes are now handled safely, preserving note properties when notes overlap or are moved on top of each other.
- **Plugin preservation during clip duplication** — Duplicating clips no longer drops or resets plugins on the target track.
- **Clip automation and range edit safety** — Clip automation data and range edits are now handled safely during overwrite operations, preventing data corruption.
- **New MIDI clip snap point** — Newly created MIDI clips now snap to the preceding grid point, preventing clips from starting slightly off-grid.
- **Provisional note property edits** — Note property edits (velocity, length) are now provisional and snap-aware, ensuring smooth drag editing without accidental value jumps.

## [v0.03-alpha] - 2026-08-13

### Added

- **Direct parameter value entry** — Click any value label (volume, pan, gain, mixer controls) and type in a numeric value directly. This provides precise control without having to nudge knobs back and forth. Works across the mixer, track headers, and plugin chain controls. (#29)
- **Ctrl+S save shortcut** — Pressing Ctrl+S now saves the current project. A confirmation flash indicates a successful save. (#27)
- **Save As action** — A new Save As action allows saving the current project to a new file without overwriting the original. Useful for creating variations or snapshots of a project.
- **Editable position display** — BPM and time signature are now editable via double-click directly in the position display. The BPM field accepts numeric input, and the time signature field allows setting numerator/denominator. The display layout has been refined with better interaction handling, improved visual alignment, and stabilized column widths that no longer shift when values change. (#19)

### Fixed

- **MIDI note duplication (Ctrl+D)** — Selecting MIDI notes in the piano roll and pressing Ctrl+D now correctly duplicates the selected notes. Previously, the shortcut had no effect. This is a standard DAW shortcut that was not wired up. (#23)
- **Drum sampler gain/pan default values** — When navigating from the master track back to a drum sampler track, gain and pan no longer reset to incorrect default values of -48 and 0 respectively. The controls now correctly retain and display the actual stored values of the drum sampler. (#24)
- **Drum sampler gain/pan text sync** — The text labels for gain and pan on drum sampler tracks now stay in sync with knob positions when switching between samples. Previously, the knobs updated correctly but the text values remained stale, showing the previous sample's values. (#25)
- **Project load/cancel lifecycle** — Cancelling the "Load Project" file dialog no longer discards the current project and starts a new one. The entire project lifecycle — new, load, save, save as — has been hardened to prevent unintended state transitions and data loss. This includes proper cleanup of state when a load operation is cancelled mid-way.
- **Plugin window persistence during chain rebuilds** — Plugin editor windows now stay open when the plugin chain view is rebuilt (e.g., when adding, removing, or reordering plugins). Previously, chain rebuilds would destroy and recreate all plugin windows, causing flickering, state loss in plugin UIs, and potential crashes. (#plugin-stability)
- **Native plugin editor lifecycle** — When a plugin editor window is closed by the user, the native editor is now hidden rather than destroyed. This preserves the plugin's internal UI state (scroll positions, tab selections, etc.) when the window is reopened later.
- **Modal input blocking during plugin window creation** — Fixed an issue where creating a plugin window could globally block keyboard/mouse input to other windows. Plugin window creation no longer grabs global modal focus.
- **Linux plugin window stacking** — On Linux, plugin editor windows now follow normal window stacking order. Previously, they could appear behind the main window or other plugin windows in certain desktop environments.
- **Position display undo transactions** — Editing BPM and time signature in the position display now creates proper undo transactions. Previously, these edits were not correctly tracked by the undo system, making it impossible to undo/redo tempo and time signature changes. (#19)

### Changed

- **Source tree restructured** — The project source tree has been flattened into an `include/` + `src/` layout, similar to how LMMS organizes its codebase. This separates public headers from implementation files and makes the project structure cleaner and more navigable for contributors. Build scripts and CMake configuration have been updated accordingly.
- **Position display layout improvements** — The position display has been refined with better visual alignment, improved click targets for editing, and more consistent spacing between BPM, time signature, and position fields.

### Removed

- No features or components were removed in this release.

### Developer / Build

- **GCC 15 build fix** — Suppressed `-Wtemplate-body` warnings for tracktion nanorange.hpp header. GCC 15 introduced stricter template-body checks that flagged false positives in the tracktion engine's nanorange implementation. The warning is now disabled for that specific translation unit.
- **Project lifecycle tests** — Added automated tests for the project lifecycle (new, load, save, save as, cancel) to the test suite. These tests verify that project state is correctly managed across all lifecycle operations and that no data loss occurs during transitions.
- **Test script** — Added `test.sh` for running the test suite locally.

## [v0.02-alpha] - 2026-06-23

### Added

- **Mono & Portamento (Glide) for Simple Synth** — Switch Simple Synth to monophonic mode with optional pitch glide. Three glide modes: Off, Always, and Legato. Glide time adjustable up to 2 seconds.
- **Automation Read/Write** — Record and play back parameter automation for any automatable control. Global toggles in the Header Bar.
- **MIDI Learn** — Assign any MIDI CC to sliders, knobs, and toggles via right-click → MIDI Learn. Works across all automatable parameters in the Track Chain.
- **Built-in SoundFont Player** — Load `.sf2` files and play them via MIDI. 16-channel support, up to 256 voices, preset selection.
- **Peak Limiter** — Transparent brickwall limiter with fixed look-ahead. Mono and stereo. Controls for ceiling, release, and mix.
- **Selectable Timestretch Algorithms** — Choose between different time-stretch algorithms in Settings for different speed/quality trade-offs.
- **Track Presets (Audio & MIDI)** — Track preset system now distinguishes between audio and MIDI tracks.
- **Pre-roll Counter** — Count-in before recording starts. Configurable number of beats.
- **Plugin Setup in First-Run Wizard** — New users get guided plugin setup on first launch.
- **Light & Dark Themes** — Built-in light and dark themes with selection in the startup wizard.

### Fixed

- Simple Synth renders MIDI sample-accurately
- Muted tracks excluded from render exports
- Arpeggiator notes stop cleanly on transport stop
- Plugin chain insert and ordering stabilized
- Multi-track header dragging improved
- Track selection actions aligned with editor state
- View state kept out of undo history
- Autosave recovery stays current during playback
- Lowpass plugin rack crash fixed
- Audio clip fade handle editing improved
- Header undo/redo buttons with reset on startup
- Folder mute buttons kept in sync
- Shift+Space stop shortcut added
- Alpha warning in welcome wizard
- Configurable editor scrollbar thickness
- Configurable build jobs
- Automation lane height persistence fixed
- Time range dragging restricted to pointer tool
- `build.sh` shows proper usage message when called without arguments

### Removed

- **Four Osc Synth** — Superseded by Simple Synth, which now covers the same feature set with a cleaner interface.
- **Bundled SoundFont** — No default `.sf2` shipped; users load their own SoundFont files.

## [v0.01-alpha.1] - 2026-03-02

### Added

- Initial alpha release of NextStudio
- Cross-platform DAW built with JUCE and Tracktion Engine
- MIDI and audio track support
- Plugin chain with sectioned layout (MIDI plugins, instrument, audio effects)
- Piano roll MIDI editor
- Simple Synth built-in instrument
- Drum sampler built-in instrument
- Audio clip editing with fade in/out
- Mixer view with gain and pan controls
- Transport controls with loop and punch recording
- Project save/load
- Undo/redo system
- Light and dark theme support
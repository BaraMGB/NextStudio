# v0.03-alpha — Release Highlights

## New Features

- **Direct Parameter Value Entry** — Click any value label (volume, pan, gain, etc.) and type in a numeric value directly. No more fiddling with knobs for precise adjustments. (#29)
- **Ctrl+S Save Shortcut** — Save the current project with Ctrl+S. Also added a Save As action for saving to a new file. (#27)
- **Editable Position Display** — BPM and time signature are now editable via double-click directly in the position display. The display layout has been refined with better interaction handling and stabilized widths. (#19)

## Bug Fixes

- **MIDI Note Duplication (Ctrl+D)** — Selecting MIDI notes in the piano roll and pressing Ctrl+D now correctly duplicates them. (#23)
- **Drum Sampler Gain/Pan Sync** — Fixed two issues with drum sampler controls: gain and pan no longer show wrong default values (-48 / 0) when navigating back from the master track, and text values now stay in sync with knob positions when switching between samples. (#24, #25)
- **Project Load/Cancel Lifecycle** — Cancelling the "Load Project" dialog no longer discards the current project and starts a new one. The project lifecycle (new/load/save) has been hardened overall.
- **Plugin Window Stability** — Plugin editor windows now stay open during chain view rebuilds instead of being destroyed and recreated. Native plugin editors are hidden rather than destroyed on close. Global modal input blocking during plugin window creation has been fixed. On Linux, plugin windows now follow normal stacking order.
- **Position Display Undo** — Editing BPM and time signature now creates proper undo transactions, so changes can be correctly undone/redone.

## Improvements

- **Source Tree Restructured** — Flattened to `include/` + `src/` layout (LMMS-style) for cleaner project structure.
- **GCC 15 Build Fix** — Suppressed `-Wtemplate-body` warnings for tracktion nanorange.hpp to fix GCC 15 compilation.
- **Tests Added** — Project lifecycle tests added to the test suite.

## What's Changed

* Full Changelog: https://github.com/BaraMGB/NextStudio/compare/v0.02-alpha...v0.03-alpha
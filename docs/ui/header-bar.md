# Header Bar

## Overview

The Header Bar spans the top of the main window and contains global transport controls, time display, and project-wide switches.

## Transport Controls

| Control | Action | Shortcut |
|---|---|---|
| Play/Pause | Starts or pauses playback; toggles recording if armed | `Space`, `NumPad 0` |
| Play (no pause) | Starts playback without toggle | `Enter` |
| Stop | Stops playback/recording, returns playhead to start | `Shift+Space`, `NumPad .` |
| Record | Starts recording on all armed tracks | `NumPad *` |
| Return to Start | Moves playhead to beginning | — |

A second click on Stop when already stopped moves the playhead to position zero.

## Time Display

Shows the current playhead position. Click to switch between formats:

- **Time format:** hours:minutes:seconds:frames
- **Musical time:** bars:beats:ticks

Additional display fields:

- Current tempo (BPM)
- Current time signature (e.g. 4/4)
- Current position in bars.beats.ticks
- Current position in minutes:seconds.milliseconds
- Loop start and end positions in bars.beats.ticks

All time-display values can be changed by clicking and vertical-dragging.

### Position Display Editing

BPM and time signature are editable via double-click directly in the position display.

- **BPM field:** accepts numeric input.
- **Time signature field:** allows setting numerator/denominator.

These edits participate in undo/redo.

## Global Switches

| Switch | Action | Shortcut |
|---|---|---|
| Loop | Enables loop playback for the selected loop range | `Cmd/Ctrl+L` |
| Metronome | Enables metronome click during playback and recording | `Cmd/Ctrl+M` |
| Follow Playhead | Song Editor and MIDI Editor scroll automatically with the playhead | — |

## Automation Controls

Global automation read/write toggles affect all automatable parameters across the project.

- **Automation Read:** When enabled, parameter values are read from existing automation data during playback. When disabled, parameters stay at their manual values regardless of recorded automation.
- **Automation Write:** When enabled, parameter changes made during playback are recorded as automation data. Captures movements on sliders, knobs, and toggles in real time.

## Pre-roll Counter

A pre-roll counter can be enabled before recording starts. When activated, pressing Record triggers a count-in (metronome clicks) before the actual recording begins.

- **Enable:** Toggles the pre-roll count-in on/off.
- **Count-in:** The number of beats counted before recording starts.

## Related documents

- [Side Browser](side-browser.md)
- [Song Editor](song-editor.md)
- [Getting Started](../user/getting-started.md)
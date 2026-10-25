# Header Bar

## Overview

The Header Bar spans the top of the main window and contains global transport controls, the time display, and project-wide transport switches. Timeline-specific follow controls belong to the Song Editor and Piano Roll ToolHeaders.

## Project and Save Information

The widened position display has a right information section using the same label/value rows as the tempo and position fields: **NAME:** on the first line and **STATUS:** on the second. The name value shows the current edit name; long names are shortened with an ellipsis. The status value reports **Unsaved changes** for a new temporary edit or after a musical change, and **Up to date** for an unchanged persistent project. At compact widths labels are removed while all values remain visible. At smaller widths position, time and project/save information take priority over tempo, signature and loop fields; the complete project name and status remain available as a tooltip.

After a successful **Save**, **Save As**, or `Ctrl/Cmd+S`, the second line temporarily changes to **Saved** for three seconds. The confirmation starts after Save As releases its existing interaction lock. It does not take keyboard focus, add a dialog or block controls. Saving again restarts the display time. Starting another save/workflow, cancelling, failing, or replacing the project clears previous feedback. Recovery autosave stays silent; existing inline save errors remain in the Projects sidebar.

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

## Automation Controls

Global automation read/write toggles affect all automatable parameters across the project.

- **Automation Read:** When enabled, parameter values are read from existing automation data during playback. When disabled, parameters stay at their manual values regardless of recorded automation.
- **Automation Write:** When enabled, parameter changes made during playback are recorded as automation data. Captures movements on sliders, knobs, and toggles in real time.

## Pre-roll Counter

A pre-roll counter can be enabled before recording starts. When activated, pressing Record triggers a count-in (metronome clicks) before the actual recording begins.

- **Enable:** Toggles the pre-roll count-in on/off.
- **Mode:** Click the pre-roll symbol to open a menu with the count-in modes: Off, 1 Beat, 2 Beats, 1 Bar, 2 Bars.
- **Countdown:** During the count-in, the pre-roll button displays the remaining beats on a red background.
- **Cancel:** Pressing Stop during the count-in cancels the pending recording.

## Related documents

- [Side Browser](side-browser.md)
- [Song Editor](song-editor.md)
- [Getting Started](../user/getting-started.md)
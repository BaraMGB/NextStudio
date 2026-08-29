# Keyboard Shortcuts

## Overview

Reference of default keyboard shortcuts. `Cmd` means `Ctrl` on Windows/Linux and `Command` on macOS. Command availability can depend on keyboard layout, focused component, and selected objects.

## Transport

| Action | Shortcut |
|---|---|
| Play/Pause | `Space`, `NumPad 0` |
| Play (without pause) | `Enter` |
| Stop | `Shift+Space`, `NumPad .` |
| Record | `NumPad *` |
| Toggle Loop | `Cmd+L` |
| Loop around selection | `Cmd+Shift+L` |
| Loop around everything | `Cmd+Shift+Alt+L` |
| Toggle Metronome | `Cmd+M` |

## Global Editing

| Action | Shortcut |
|---|---|
| Undo | `Cmd+Z` |
| Redo | `Cmd+Shift+Z` |
| Save | `Cmd+S` |

## Song Editor

| Action | Shortcut |
|---|---|
| Delete Selected Clips/Time Range | `Backspace`, `Delete`, `Cmd+X` |
| Duplicate Selected Clips/Time Range | `Cmd+D` |
| Select All Clips | `Cmd+A` |
| Render Selected Time Range to New Track | `Cmd+R` |
| Transpose Clip Up/Down | `Cmd+Up/Down` |
| Reverse Clip | `Cmd+B` |

## Track List

| Action | Shortcut |
|---|---|
| Delete Selected Tracks | `Backspace`, `Delete`, `Cmd+X` |
| Duplicate Selected Tracks | `Cmd+D` |

## MIDI Editor

| Action | Shortcut |
|---|---|
| Delete Selected Notes | `Backspace`, `Delete`, `Cmd+X` |
| Copy Selected Notes | `Cmd+C` |
| Paste Notes (provisional) | `Cmd+V` |
| Confirm Pending Paste | `Enter` |
| Cancel Pending Paste | `Escape` |
| Duplicate Selected Notes | `Cmd+D` |
| Nudge Notes Up/Down/Left/Right | Arrow keys |
| Nudge Notes Octave Up/Down | `Cmd+Up/Down` |

## Virtual MIDI Keyboard (Computer Keyboard)

| Octave | Keys |
|---|---|
| Upper Octave (C4) | `Q, 2, W, 3, E, R, 5, T, 6, Z, 7, U` |
| Lower Octave (C3) | `Y, S, X, D, C, V, G, B, H, N, J, M` |
| Highest C (C5) | `I` |

These are the defaults. You can reassign them in Settings → Keys.

`Q` and `,` are aliases for the same upper C in the default layout. These performance keys are handled by the dedicated computer MIDI keyboard and can be customized in Settings → Keys. The exact physical key positions depend on the keyboard layout.

## Settings → Keys panel

The Keys panel contains two sections:

1. **Virtual MIDI keyboard keys** for assigning the computer-keyboard performance keys.
2. **Application shortcuts** for assigning shortcuts to application commands.

Both sections belong to one vertically scrollable page. The scrollbar at the right moves the complete page, so the virtual-keyboard settings can be scrolled out of the way to expose the application shortcuts. The shortcut list does not display a separate scrollbar.

Shortcut categories remain expandable. Expanding or collapsing a category updates the page height while preserving the shared scrolling model.

The layout is implemented by `KeyboardSettingsComponent` in `App/include/KeyboardSettingsComponent.h` and `App/src/KeyboardSettingsComponent.cpp`. Its outer `juce::Viewport` owns the visible scrollbar, while the embedded `juce::KeyMappingEditorComponent` is sized to the full height of its tree content.

## Related documents

- [Getting Started](../user/getting-started.md)
- [Header Bar](header-bar.md)
- [Song Editor](song-editor.md)
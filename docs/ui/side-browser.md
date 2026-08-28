# Side Browser

## Overview

The Side Browser is the vertical panel on the left side of the main window. It provides access to projects, instruments, effects, samples, files, settings, and rendering. Clicking the active icon toggles between collapsed and expanded states. The width is adjustable via the adjacent splitter.

## Sections

### Projects

- Lists project files (`.tracktionedit`) in the project directory.
- **Buttons:** New (create project), Load (open file chooser), Save (save current project).
- Sort and search functions.
- Double-click a file to load it.

### Instruments

- Lists all detected instrument plugins (VST, AU, etc.) and internal instruments.
- Plugins can be dragged from here to a track header or to the Track Chain instrument slot.
- Sort and search functions.

### Effects

- Lists all detected effect plugins and internal effects.
- Works analogously to the Instruments Browser.
- Drag to a `[+]` slot in the Track Chain or directly into the chain.

### Samples

- Shows audio files (WAV, MP3, AIFF, FLAC) from the configured sample directory.
- Sort and search functions.
- Selected samples appear in the lower sample preview window.
- Samples can be dragged to the Song Editor onto an audio track.

### Home

- General file browser starting in the configured working directory.
- Folder navigation.
- Supports sample preview and drag-and-drop of audio files.

### Settings

#### Audio
Configuration of audio hardware: drivers, inputs/outputs, latency (sample rate, buffer size).

#### MIDI
- Default MIDI input device selection.
- **Exclusive MIDI Focus** toggle — when enabled, only the focused track receives MIDI input.

#### Plugins
Plugin list management. Scan for new plugins, clear the list, or manage individual plugins.

#### General

| Setting | Description |
|---|---|
| Scaling Factor | UI scaling slider (0.2x to 3.0x) |
| Mouse Cursor Scaling | Adjusts cursor size for high-resolution displays |
| Time-Stretch Algorithm | Select algorithm for time-stretching audio clips |
| Content Folder | Default folder for projects and samples |
| Version | Current NextStudio version |
| Theme Presets | Dropdown for saved color themes; save/load custom themes |
| Theme Colors | Color buttons to customize UI colors |

#### Keys
- Virtual MIDI keyboard mapping for the computer keyboard, including the optional upper-C alias.
- Application shortcut configuration.

### Render

- Opens a dialog for exporting the project as an audio file.
- Options: destination folder, filename, time range (Entire Edit, Loop range, or manual range).

## Sample Preview

Visible when the Samples or Home browser is active.

- Shows the waveform of the selected sample.
- **Buttons:** Play, Stop, Loop, Sync Tempo (adjusts sample tempo to project if possible).
- Volume control.

## Related documents

- [Header Bar](header-bar.md)
- [Song Editor](song-editor.md)
- [Getting Started](../user/getting-started.md)
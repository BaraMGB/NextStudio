# Side Browser

## Overview

The Side Browser is the vertical panel on the left side of the main window. It provides access to projects, instruments, effects, samples, files, settings, and rendering. Clicking the active icon toggles between collapsed and expanded states. The width is adjustable via the adjacent splitter.

## Sections

### Projects

The Projects view is a directory browser rooted initially at the configured Projects directory. It always shows folders and filters files to `.tracktionedit` projects. It provides:

- **New** — create a project;
- **Save** — write directly to an existing persistent path, or enter Save As for a new project;
- **Save As** — save under a new name in the currently displayed directory;
- folder navigation, sorting and search;
- double-click loading through the guarded project workflow.

Browsing remains non-modal; there is no separate Load mode or Load button. Save As dims and disables the editor, lower range, application commands, MIDI keyboard routing, and plugin windows while its sidebar controls remain active; transport/audio processing for the edit is suspended. The sidebar splitter stays usable so the browser can be widened. Clicking outside Save As, pressing `Escape`, or pressing **Cancel** closes it without changing the project. Previously active playback resumes after cancellation or saving, but recording does not.

Existing targets require an inline **Overwrite** confirmation. Invalid names, unreadable projects, unavailable paths, and write failures are reported inside the Projects sidebar rather than in a project-specific alert window.

See [Embedded Project File Browser and Save-As Interaction Boundary](../changes/embedded-project-file-browser.md) for the complete behavior and implementation.

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
- Uses the same asynchronous directory-navigation component as Projects, without a project-file filter.
- Selection is forwarded to the edit-aware sample preview; the browser itself has no Engine or Edit dependency.
- Supports sample preview and drag-and-drop of audio files.

### Settings

#### Audio
Configuration of audio hardware: drivers, inputs/outputs, latency (sample rate, buffer size).

#### MIDI
- Default MIDI input device selection.
- **Exclusive MIDI Focus** toggle — when enabled, the default input follows exactly one selected MIDI track. A selected MIDI clip keeps focus on its track. Changing the default or toggling focus reconciles the routing immediately.
- The virtual PC-keyboard MIDI input always follows the selected MIDI track, independently of this toggle.
- Inputs selected manually from a track header remain permanently assigned and are independent of focus. An automatic default route yields to a manual input on the focused track to prevent duplicate delivery through aggregate defaults such as **All MIDI Ins**.

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
- [MIDI Input Routing and Exclusive Focus](../architecture/midi-input-routing.md)
- [Getting Started](../user/getting-started.md)
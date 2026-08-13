# Getting Started

## About this guide

This guide introduces the current NextStudio alpha workflow: first launch, project management, the main window, tracks, the lower editor area, saving, and recovery.

NextStudio is under active development. Features, project compatibility, and UI behavior may change. Keep independent backups of important projects and source audio.

## First launch

On first launch, or when the configured content root no longer exists, NextStudio opens the Setup Wizard.

The wizard configures:

- the user content folder;
- GUI scale;
- a built-in theme;
- plug-in locations/scanning settings;
- audio and MIDI devices.

### User content folder

The default root is:

```text
~/NextStudio
```

Choosing another root creates or uses this folder structure:

```text
<root>/
├── Clips/
├── Presets/
├── Projects/
├── Renders/
└── Samples/
```

NextStudio validates that the root can be prepared before accepting it. Bundled content such as the included drum samples is populated into the selected content area.

If setup is closed without completion, the current fallback behavior selects `~/NextStudio`, applies the default built-in theme, and marks setup complete.

### GUI scale

The wizard allows a scale from `0.2x` to `3.0x`. The change is applied immediately. Very small or very large values may be impractical depending on monitor resolution.

### Theme

Choose one of the embedded `.nxttheme` presets. Theme colors affect the main frame, editor areas, track colors, buttons, text, timeline, and supported component icons.

### Plug-ins

Configure the locations and formats relevant to the operating system. NextStudio can host VST3, LADSPA, LV2, and Audio Unit formats where supported by the platform and installed plug-ins.

### Audio and MIDI devices

The wizard embeds JUCE's audio-device selector. Configure at least:

- an audio output device;
- sample rate and buffer size appropriate for the system;
- audio inputs needed for recording;
- MIDI inputs needed for controllers.

Lower buffer sizes reduce monitoring latency but increase CPU pressure. Start with a conservative value if audio breaks up.

## Main window

The window contains four major areas:

```text
┌───────────────┬────────────────────────────────────┐
│ Sidebar       │ Header / transport                 │
│               ├────────────────────────────────────┤
│               │ Arrangement editor                 │
├───────────────┴────────────────────────────────────┤
│ Lower range: Mixer / MIDI Editor / Plugins         │
└────────────────────────────────────────────────────┘
```

### Header

The header contains transport and global project controls, including playback, recording, loop/metronome-related controls, position display, and undo/redo integration.

### Arrangement editor

The arrangement editor shows tracks, clips, timeline, playhead, automation controls, and editing tools. Track headers are on the left and lanes on the right.

Track controls above the list can add:

- audio tracks;
- MIDI tracks;
- folder tracks.

The editor also provides controls to expand or collapse tracks.

### Sidebar

The vertical sidebar switches between:

- **Projects** — create, load, save, Save As, search, and select projects;
- **Instruments** — browse instrument plug-ins;
- **Effects** — browse effect plug-ins;
- **Samples** — browse configured sample content and preview audio;
- **Home** — browse the configured NextStudio content root;
- **Settings** — audio/MIDI, plug-in, appearance, and behavior settings;
- **Render** — export/render controls.

Clicking the currently active sidebar button toggles the sidebar between collapsed and expanded states. Drag the adjacent splitter to resize it.

### Lower range

The lower range has tabs for:

- **Mixer**;
- **MIDI Editor**;
- **Plugins**.

The active track follows the current track or clip selection. The MIDI Editor tab is enabled when a MIDI clip is selected.

Drag the splitter above the lower range to resize the Piano Roll. Mixer and plug-in views currently use a fixed lower-range height.

## Project workflow

### Create a project

Open the Projects sidebar and press **New**. If the current project has unsaved changes, NextStudio asks whether to save, discard, or cancel.

A new project starts with a temporary recovery file and the window title `Untitled`. The first normal Save asks for a persistent project filename.

### Load a project

Use either:

- **Load** in the Projects sidebar and choose a `.tracktionedit` file; or
- double-click a project in the project list.

Before loading, NextStudio validates that the file:

- exists;
- has the expected extension;
- is not empty;
- contains a valid Tracktion `EDIT` state.

An invalid file does not replace the current project.

### Save

Use **Save** in the Projects sidebar or:

- Windows/Linux: `Ctrl+S`;
- macOS: `Command+S`.

A project that has never been persistently saved opens a file chooser. Saved projects use the existing path.

### Save As

Use **Save As** to select a new target while preserving the original project file. The project browser refreshes after a successful save.

### Unsaved changes

When creating/loading another project or quitting, NextStudio may show:

- **Yes** — save, then continue only if saving succeeds;
- **No** — discard changes and continue;
- **Cancel** — keep the current project and stop the action.

Cancelling a Save dialog also stops the pending project switch or quit.

## Autosave and crash recovery

NextStudio writes recovery snapshots to Tracktion Engine's temporary directory. Autosave is skipped while recording and while specific model operations hold a save lock.

A normal successful save removes obsolete recovery snapshots. A clean application shutdown removes the temporary directory.

If NextStudio terminates unexpectedly and a recovery snapshot remains, the next launch asks whether to restore the crashed project. Choose:

- **Yes** to open the recovery edit;
- **No** to discard recovery data and start a new project.

Recovery is a safety net, not a replacement for persistent saves and external backups.

## Basic track workflow

### Audio track

1. Add an audio track.
2. Select/configure its input in the track controls or settings.
3. Arm the track as required.
4. Verify monitoring and input level.
5. Start recording from the transport.

NextStudio disables wave inputs by default during edit initialization and enables MIDI inputs. Confirm audio input state before recording.

### MIDI track

1. Add a MIDI track.
2. Insert/select an instrument in the Plugins lower view or instrument browser.
3. Select/arm the track so MIDI focus is routed appropriately.
4. Record MIDI or create a MIDI clip.
5. Select the MIDI clip and open **MIDI Editor**.

See [Piano Roll](piano-roll.md) for note editing.

### Plug-in chain

Select a track and open **Plugins** in the lower range. The chain view is organized around the selected track and supports instrument/effect insertion and built-in or external plug-in editors.

Plug-in windows are designed to remain open across chain-view rebuilds. Closing a native editor hides it rather than necessarily destroying its internal UI state.

### Mixer

Open **Mixer** in the lower range to view channel strips for the current edit. Mixer controls use the shared automatable parameter system where applicable.

## Position display editing

The header position display presents fields such as BPM, time signature, transport position, clock time, and loop boundaries. Editable fields support direct text entry and segmented dragging.

Bars/beats/ticks are displayed as:

```text
bar.beat.tick
```

For the standard 960 ticks per quarter note, `6.2.240` means bar 6, beat 2, tick 240.

Tempo and time-signature edits participate in undo/redo.

## Useful default shortcuts

`Command` below means `Ctrl` on Windows/Linux and `Command` on macOS unless noted.

| Action | Shortcut |
|---|---|
| Play/pause | `Space` or numpad `0` |
| Play | `Enter` |
| Stop | `Shift+Space` or numpad decimal |
| Record | numpad multiply |
| Save | `Command+S` |
| Undo | `Command+Z` |
| Redo | `Command+Shift+Z` |
| Toggle loop | `Command+L` |
| Loop around selection | `Command+Shift+L` |
| Toggle metronome | `Command+M` |

Command availability can depend on keyboard layout, focused component, and selected objects.

## Virtual MIDI keyboard

The computer keyboard can send notes to NextStudio's virtual MIDI input. The default mapping begins at MIDI note 48 and uses rows such as:

```text
Lower row: Y S X D C V G B H N J M
Upper row: Q 2 W 3 E R 5 T 6 Z 7 U I
```

The exact physical key positions depend on keyboard layout because the mapping uses the configured key descriptions. Select an appropriate MIDI/instrument track so exclusive MIDI focus routes notes as expected.

## Settings and persistence

Application settings are saved to JUCE's user application-data directory under:

```text
NextStudio/AppSettings.xml
```

They include window bounds, content paths, theme, GUI scale, sidebar state, autosave interval, and behavior options.

Edit-specific view state—such as timeline zoom, scroll position, lower-range selection, and Piano Roll dimensions—is stored with the edit state and is not intended to create undo steps.

## Troubleshooting

### No sound

- Confirm the audio output device in Settings.
- Check sample rate and buffer configuration.
- Verify the selected track has an instrument or audible audio clip.
- Check track/master gain, mute, and monitoring.
- Confirm the transport is playing.

### MIDI controller does not trigger the selected instrument

- Confirm the MIDI input is enabled.
- Select the intended MIDI/instrument track.
- Check exclusive MIDI focus behavior in Settings.
- Verify the instrument is present in the track's plug-in chain.

### MIDI Editor tab is disabled

Select a MIDI clip. Selecting only a track is not sufficient for the tab-enable condition in the current UI.

### Plug-in is missing

- Open Settings and verify scan paths.
- Rescan plug-ins.
- Confirm the format is supported on the platform.
- Check whether the plug-in architecture matches NextStudio.

### Project will not load

Only valid `.tracktionedit` project files are accepted through normal loading. Empty, corrupt, missing, or wrong-format files are rejected without replacing the current project.

### UI is too large or small

Adjust GUI scale in Settings or rerun setup as appropriate. The accepted range is broad; choose a practical value for the monitor.

## Related documents

- [Piano Roll](piano-roll.md)
- [Peak Limiter](peak-limiter.md)
- [Project Lifecycle](../architecture/project-lifecycle.md)

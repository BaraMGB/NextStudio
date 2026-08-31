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

A clip-properties row above the timeline displays the number of selected clips and exact `START`, `END`, and `DURATION` values. Mixed values appear as `—`. Double-click a field to type a bars/beats/ticks position, note fraction, or tick duration. Mouse-wheel stepping and vertical drag scrubbing use the current arrangement snap interval; drag scrubbing previews the resulting clip positions before committing on release.

The same row provides independent arrangement controls:

- **SNAP** — Off, Adaptive, or a fixed value from `1/1` through `1/128`;
- **INSERT LENGTH** — Adaptive or a fixed value used when creating MIDI clips.

With the Knife tool active, hovering a clip displays a vertical split preview at the current arrangement snap position. Clicking cuts at that exact position; hold `Shift` to preview and cut without snapping.

The lower fifth of the arrangement timeline is a darkened loop lane. Hovering its free area shows a pencil cursor and a draw hint. Existing ranges provide separate move, start, and end hints. The loop range is dimmed while loop playback is disabled.

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

The sidebar starts expanded on the **Projects** view. Drag the adjacent splitter to resize it. Its initial expanded width is 400 pixels and it can be reduced continuously to 250 pixels on every display. Dragging another 100 pixels toward the menu rail crosses the collapse resistance and snaps it closed. Clicking a sidebar menu button opens the selected view; clicking the active button again also collapses the sidebar.

### Lower range

The lower range has tabs for:

- **Mixer**;
- **MIDI Editor**;
- **Plugins**.

The active track follows the current track or clip selection. The MIDI Editor tab is enabled when a MIDI clip is selected. Double-clicking a MIDI clip in the arrangement opens the MIDI Editor for that track; if the lower range is collapsed, the double-click also expands it.

Drag the splitter down to close the lower range, or back up to expand it. When the Piano Roll is visible, upward drags enlarge it freely and downward drags shrink it continuously back to the standard 350-pixel height before the collapse snap zone begins. The maximum Piano Roll height stops at the bottom edge of the Song Editor timeline. The panel snaps only after crossing the active transition target, and the direction can be reversed without releasing the mouse button. The collapsed bar provides buttons for Mixer, MidiEditor, and PluginChain; MidiEditor is disabled unless a MIDI clip is selected. The collapsed state is restored in the next application session. Mixer and plug-in views currently use a fixed lower-range height.

## Project workflow

### Create a project

Open the Projects sidebar and press **New**. If the current project has unsaved changes, NextStudio asks whether to save, discard, or cancel.

A new project starts with a temporary recovery file and the window title `Untitled`. The first normal Save asks for a persistent project filename.

### Load a project

Use either:

- **Load** in the Projects sidebar and choose a `.tracktionedit` file in the embedded browser; or
- double-click a project in the project list.

Load does not open another window and does not block the rest of NextStudio. Folders and supported project files are shown directly in the sidebar.

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

A project that has never been persistently saved enters Save As in the Projects sidebar. Saved projects use the existing path directly.

### Save As

Use **Save As** to select a folder and new project name while preserving the original project file. `.tracktionedit` is added automatically. Existing files require inline overwrite confirmation.

While Save As is active, the rest of the main window is dimmed and blocked. The sidebar splitter remains usable for widening the browser. Click outside Save As, press `Escape`, or press **Cancel** to close it without changing the project.

### Unsaved changes

When loading another project with unsaved changes, the Projects sidebar offers **Save & Open**, **Discard & Open**, and **Back**. A pending load continues only after a successful save.

Other replacement or shutdown paths may show the corresponding save, discard, and cancel decision. Cancelling Save As also stops a pending project switch.

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

See [Piano Roll](piano-roll.md) for note editing and [ClipPropertiesBar](../components/clip-properties-bar.md) for exact arrangement clip editing.

### Plug-in chain

Select a track and open **Plugins** in the lower range. The chain view is organized around the selected track and supports instrument/effect insertion and built-in or external plug-in editors.

Track Presets and Modifiers start collapsed to leave more width for plug-in editors. Use the arrow at the left of either panel header or its narrow collapsed rail to expand/collapse it. The two choices are independent and persist across application sessions. Collapsing Modifiers also temporarily hides an open modifier detail panel.

The horizontal scrollbar remains visible and can reach the full rack content on narrow windows. Plug-in windows are designed to remain open across chain-view rebuilds. Closing a native editor hides it rather than necessarily destroying its internal UI state.

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

`Q` and `,` are aliases for the same upper C in the default layout. The mapping is handled by the dedicated computer MIDI keyboard controller and can be customized in Settings → Keys. Note names follow Tracktion's middle-C convention, so the default range is `C3` to `C5` (`48 = C3`, `60 = C4`, `72 = C5`). The exact physical key positions depend on the keyboard layout. Select an appropriate MIDI/instrument track so exclusive MIDI focus routes notes as expected.

## Settings and persistence

Application settings are saved to JUCE's user application-data directory under:

```text
NextStudio/AppSettings.xml
```

They include window bounds, content paths, theme, GUI scale, sidebar state, autosave interval, behavior options, and the custom computer MIDI keyboard mapping.

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

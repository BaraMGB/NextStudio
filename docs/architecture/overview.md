# Architecture Overview

## Scope

This document gives contributors a map of NextStudio's current architecture. It focuses on ownership, state boundaries, the main component hierarchy, and the flow between JUCE, Tracktion Engine, and the application-specific UI.

NextStudio is a desktop Digital Audio Workstation implemented in C++20. JUCE provides the application framework, GUI, audio-device abstractions, plug-in hosting infrastructure, and utility types. Tracktion Engine provides the editable music model, transport, tracks, clips, MIDI data, automation, plug-ins, selection objects, playback, and undo support.

## Architectural layers

The code can be read as five cooperating layers:

1. **Process and window lifetime** — `NextStudioApplication` and its `MainWindow` create and destroy the application UI.
2. **Application services and ownership** — `MainComponent` owns the Tracktion engine, current edit, global selection manager, command manager, and the top-level UI objects.
3. **Persistent state** — `ApplicationViewState` stores machine/user settings; `EditViewState` stores UI state associated with the current edit; the Tracktion `Edit` stores musical project state.
4. **Editor components** — the arrangement editor, header, sidebar, lower range, Piano Roll, mixer, and plug-in chain present and manipulate the model.
5. **Feature components and helpers** — tools, plug-in editors, browsers, position parsing, project lifecycle validation, presets, themes, and shared utilities.

The architecture is model-driven: UI components generally mutate Tracktion objects or `ValueTree` properties, listen for resulting changes, and repaint or rebuild only the affected area.

## Process lifetime

### `NextStudioApplication`

`App/src/Main.cpp` defines the JUCE application object. During `initialise()` it creates one `MainWindow`. During `shutdown()` it releases that window.

The application also routes the operating system's quit request through `MainComponent::handleUnsavedEdit()`. Quitting can therefore be cancelled if the user cancels the unsaved-project dialog or if a requested save does not complete successfully.

### `MainWindow`

`MainWindow` is a `juce::DocumentWindow` that:

- restores the saved window bounds from `ApplicationViewState` on desktop platforms;
- uses full-screen mode on iOS and Android;
- owns `MainComponent` as its content component;
- delegates its close button to `JUCEApplication::systemRequestedQuit()`.

## Top-level ownership

`MainComponent` is the central composition root. Its member declaration order and explicit teardown are significant.

It owns:

- `ApplicationViewState&` — application-wide persistent settings, owned by the application object;
- `NextLookAndFeel` — theme-aware JUCE look-and-feel;
- `tracktion_engine::Engine` — Tracktion's process-level engine and service container;
- `juce::ApplicationCommandManager` — command registration and key mappings;
- `tracktion_engine::SelectionManager` — shared selection for the current UI;
- `std::unique_ptr<tracktion_engine::Edit>` — the current project model;
- `EditViewState` — view/editor state bound to that edit;
- `EditComponent` — arrangement editor;
- `HeaderComponent` — transport and global controls;
- `EditorContainer` — layout wrapper for header and arrangement editor;
- `LowerRangeComponent` — mixer, Piano Roll, or plug-in chain;
- `SidebarComponent` — projects, files, samples, plug-ins, settings, and rendering.

### Why explicit teardown matters

Most UI components keep references to `Edit`, `EditViewState`, tracks, clips, selection objects, or Tracktion services. When switching projects, `MainComponent::setupEdit()` destroys all objects that refer to the old edit before replacing `m_edit`:

```text
EditorContainer
HeaderComponent
EditComponent
LowerRangeComponent
SidebarComponent
EditViewState
Edit
```

The destructor follows the same principle. This prevents listeners, components, or cached pointers from outliving the model they reference.

## Main UI composition

At a high level, the window is divided into:

```text
MainComponent
├── SidebarComponent
├── Sidebar splitter
├── EditorContainer
│   ├── HeaderComponent
│   └── EditComponent
└── LowerRangeComponent
    ├── LowerRangeTabBar
    ├── MixerComponent
    ├── PianoRollEditor
    └── PluginChainView
```

### Header and arrangement

`EditorContainer` reserves 60 pixels for `HeaderComponent`, adds a small gap, and gives the remaining area to `EditComponent`.

`EditComponent` is the main arrangement editor. It owns:

- the song timeline and playhead;
- track headers and track lanes;
- arrangement tools and automation controls;
- horizontal and vertical scrollbars;
- the footer and snap description;
- the autosave worker and timer.

It listens to the edit, selection manager, automation record manager, and transport. Structural changes are coalesced through `FlaggedAsyncUpdater` before expensive view rebuilds.

### Sidebar

`SidebarComponent` provides switchable views for:

- Projects;
- instrument plug-ins;
- effect plug-ins;
- samples and preview;
- the configured home/content directory;
- settings;
- rendering.

Selecting the same sidebar button again collapses or expands the sidebar. Its width and collapsed state are stored in `ApplicationViewState`.

### Lower range

`LowerRangeComponent` hosts one of three views selected by `LowerRangeTabBar`:

- `MixerComponent`;
- `PianoRollEditor`;
- `PluginChainView`.

The active track follows the global selection manager. A selected track has priority; if a clip is selected, its owning track is used. If neither is available, the component may fall back to a track marked with `IDs::showLowerRange`.

The MIDI Editor tab is enabled only when the first selected object is a MIDI clip. The lower range height is edit-local state for the Piano Roll and a fixed 350 pixels for the mixer and plug-in chain in the current layout.

## Model boundaries

### Tracktion `Engine`

The engine is process-level and survives project changes. It owns or exposes services such as:

- audio and MIDI device management;
- plug-in management and scanning;
- temporary-file management;
- UI behavior integration;
- playback infrastructure.

Built-in NextStudio plug-ins are registered with the engine's plug-in manager in `MainComponent` before the first edit is opened.

### Tracktion `Edit`

An `Edit` is the current project. It contains the authoritative musical and playback model:

- tracks and folders;
- audio and MIDI clips;
- MIDI notes and controllers;
- tempo and time-signature sequence;
- transport and loop range;
- plug-ins and automation;
- undo manager;
- project `ValueTree` state.

There is one active edit at a time.

### `SelectionManager`

The shared `SelectionManager` coordinates selected tracks, clips, and selectable aggregate objects such as `SelectedMidiEvents`. Components use typed queries such as `getFirstItemOfType<T>()` and `getItemsOfType<T>()` rather than maintaining an unrelated global selection.

The Piano Roll's note selection is represented by `te::SelectedMidiEvents`, which is itself inserted into the global selection manager. This makes MIDI-note commands and property UI participate in the same selection ecosystem as the rest of the application.

## State ownership summary

| State | Owner/lifetime | Persistence | Examples |
|---|---|---|---|
| Application settings | `ApplicationViewState` | `AppSettings.xml` | content paths, window bounds, theme, GUI scale, autosave interval |
| Musical project | Tracktion `Edit` | `.tracktionedit` | tracks, clips, notes, plug-ins, tempo, automation |
| Edit UI state | `EditViewState` children in edit state | with edit/recovery state | zoom, scroll, lower view, track heights, Piano Roll scale |
| Transient component state | individual components | not intentionally persisted | hover flags, active drag, pending editor text |
| Recovery snapshot | `EditComponent` autosave | `.nextTemp` in engine temp directory | copy of current edit state for crash recovery |

See [State and Event Model](state-and-events.md) for details.

## Command routing

NextStudio uses `juce::ApplicationCommandManager` and `ApplicationCommandTarget` implementations at several levels:

- `MainComponent` handles transport, global undo/redo, save, and virtual MIDI keyboard commands;
- `EditComponent` handles arrangement and track commands;
- `TrackListView` handles track-list-specific commands;
- `PianoRollEditor` handles MIDI-note deletion, duplication, and nudging.

`MainComponent` registers all of these targets after creating the current edit UI. Targets can delegate to their first parent command target, allowing keyboard commands to resolve in context.

## Piano Roll subsystem

The Piano Roll is composed of:

- `PianoRollEditor` — orchestration, layout, toolbar, commands, active track, and refresh scheduling;
- `MidiViewport` — note rendering, hit testing, selection, note operations, scrolling, and tool delegation;
- `ToolStrategy` implementations — pointer, draw, range, eraser, knife, and lasso behavior;
- `KeyboardView` — pitch display, audition, pitch-wide selection, and vertical zoom interaction;
- `VelocityEditor` — velocity display and drag editing;
- `NotePropertiesBar` — exact multi-note property editing;
- `TimeLineComponent`, `TimelineOverlayComponent`, and `PlayheadComponent` — time axis, overlays, and playhead.

The tools use a strategy/factory design. `MidiViewport` forwards mouse events to the active `ToolStrategy`, and `ToolFactory` creates the implementation for a `Tool` enum value.

## Plug-ins and resources

The application hosts external plug-ins through JUCE/Tracktion and registers built-in NextStudio plug-ins explicitly. Current compile definitions enable VST3, LADSPA, LV2, and Audio Unit hosting; actual availability also depends on platform and installed SDK/runtime support.

Binary resources are declared in `App/resources/CMakeLists.txt`:

- SVG icons become the `Pictures` binary-data target;
- `.nxttheme` files become `ThemePresets`;
- bundled 707, 808, and 909 samples become separate binary-data targets.

These targets are linked into the application.

## Threading model

Most UI and model interaction occurs on the JUCE message thread. Important exceptions include:

- audio processing, managed by Tracktion/JUCE real-time infrastructure;
- autosave file writing, performed by a one-thread pool owned by `EditComponent`;
- asynchronous callbacks posted through `juce::MessageManager::callAsync()`;
- plug-in scanning and other library-managed background work.

Code running away from the message thread must not dereference destroyed components. Autosave callbacks use `juce::Component::SafePointer` before returning to the message thread.

## External dependencies

Git submodules are declared in `.gitmodules`:

- `modules/tracktion_engine` — Tracktion Engine and its JUCE dependency;
- `modules/rubberband` — optional higher-quality time stretching;
- `modules/tinysoundfont` — SoundFont playback support.

If Rubber Band is present, CMake enables it; otherwise Tracktion's SoundTouch support remains enabled.

## Important design rules for contributors

1. Destroy UI objects before the `Edit` they reference.
2. Treat Tracktion model state as authoritative; do not create parallel unsynchronized models.
3. Separate application-wide settings from edit-local view state.
4. Do not put view-only changes into the project undo history unless they are intended user edits.
5. Remove listeners before replacing or destroying broadcasters.
6. Coalesce repeated `ValueTree` notifications before expensive rebuilds.
7. Validate a complete multi-object operation before modifying the first object.
8. Keep file I/O off the message thread where practical, but marshal UI completion back safely.
9. Group user operations into meaningful undo transactions.
10. Update the relevant document when changing a documented subsystem.

## Related documents

- [State and Event Model](state-and-events.md)
- [Project Lifecycle](project-lifecycle.md)
- [Source Layout](../development/source-layout.md)
- [Piano Roll](../user/piano-roll.md)
- [NotePropertiesBar](../components/note-properties-bar.md)

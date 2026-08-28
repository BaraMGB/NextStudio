# State and Event Model

## Purpose

NextStudio combines several state systems: Tracktion's editable model, JUCE `ValueTree` data, cached values, selection broadcasters, component callbacks, and asynchronous refresh scheduling. This document explains which state belongs where and how changes reach the UI.

## State domains

### Application-wide state: `ApplicationViewState`

`ApplicationViewState` exists for the lifetime of the application and is owned by `NextStudioApplication`. It is passed by reference to `MainComponent` and, through `EditViewState`, to most editor components.

It stores settings that are not part of one musical project:

- content-root and derived folders for projects, samples, clips, renders, and presets;
- favorites;
- window position and size;
- theme colors;
- GUI and mouse-cursor scale;
- autosave interval;
- sidebar width and collapsed state;
- lower-range collapsed state;
- Track Chain preset-panel and modifier-panel collapsed states;
- preview settings;
- exclusive MIDI focus behavior;
- time-stretch mode;
- setup completion;
- scrollbar thickness.

The state is represented by `m_applicationStateValueTree` and `juce::CachedValue` members. It is loaded from and written to:

```text
<user application data>/NextStudio/AppSettings.xml
```

The exact base directory is provided by JUCE and is platform-dependent.

#### Settings tree

The primary child trees are:

```text
AppSettings
├── FileBrowser
├── Favorites
├── FavoriteTypes
├── WindowState
├── ThemeState
├── Behavior
└── ComputerMidiKeyboard
```

Components that need live theme or behavior changes listen to `m_applicationStateValueTree` or a relevant child. The Track Chain's preset and modifier side panels use `Behavior/TrackPresetPanelCollapsed` and `Behavior/ModifierPanelCollapsed`; both default to collapsed and apply across tracks and projects. The computer MIDI keyboard layout is persisted under `ComputerMidiKeyboard` and hot-reloaded by `MainComponent` into the global `ComputerMidiKeyboardController`.

### Project model: `tracktion_engine::Edit`

The `Edit` is the authoritative project model. Its `state` tree and Tracktion object wrappers represent musical data and engine configuration. User operations that change the project should generally use Tracktion APIs and the edit's undo manager.

Examples:

- `MidiNote::setStartAndLength(...)`;
- `MidiNote::setNoteNumber(...)`;
- `MidiNote::setVelocity(...)`;
- sequence `addNote(...)` and `removeNote(...)`;
- tempo/time-signature setters;
- track and clip operations;
- plug-in and automation changes.

Directly writing arbitrary properties is appropriate only where Tracktion or NextStudio explicitly models data that way.

### Edit-local UI state: `EditViewState`

`EditViewState` is constructed for one `Edit` and one shared `SelectionManager`. It stores view and editor preferences that should follow the edit:

- arrangement and Piano Roll zoom/scroll;
- track heights and minimized state;
- visibility of editor areas;
- lower-range view and Piano Roll height;
- independent arrangement and Piano Roll snap settings;
- arrangement MIDI-clip and Piano Roll note insertion-length modes;
- playhead follow mode;
- timeline, keyboard, velocity-editor, footer, and clip-header dimensions;
- last note length and velocity;
- plug-in preset-manager UI state;
- per-track plug-in-chain UI state.

Most values are `juce::CachedValue` objects referring to children of the edit state:

```text
EDIT
├── EDITVIEWSTATE
│   ├── pluginPresetManagerUIStates
│   └── trackPluginChainViewState
└── viewData
    ├── SongEditor
    └── ID<sanitized-track-id>
```

Timeline IDs distinguish the arrangement view from track-specific Piano Roll views.

#### No undo for view state

`EditViewState` deliberately passes a null `UndoManager` to view-state changes. Scrolling, zooming, resizing, selecting a lower view, or changing a UI panel should persist as editor state without appearing in musical undo/redo history.

### Transient component state

Short-lived interaction state belongs to components or tools. Examples include:

- active mouse drag mode;
- clicked/hovered MIDI note;
- pending lasso state;
- temporary text-editor contents;
- provisional clip/note property edit plans;
- the Piano Roll's cached note-under-pointer value;
- asynchronous update flags;
- current layout bounds;
- cached clip pointers that can be regenerated;
- the Piano Roll keyboard's active live-MIDI note bits.

Transient state must be reset when the underlying edit, track, clip set, or selection changes.

## `ValueTree` as an event source

Many classes implement `te::ValueTreeAllEventListener` or `juce::ValueTree::Listener`. A model mutation can produce several notifications: property changes, child additions/removals, order changes, parent changes, or redirections.

Directly rebuilding UI in each callback would create redundant work and can be unsafe while a model operation is still producing related notifications. NextStudio therefore frequently uses `FlaggedAsyncUpdater`.

## Coalesced asynchronous refreshes

`FlaggedAsyncUpdater` wraps `juce::AsyncUpdater` with named boolean flags:

```cpp
void markAndUpdate(bool& flag)
{
    flag = true;
    triggerAsyncUpdate();
}
```

The component's `handleAsyncUpdate()` then consumes each flag with `compareAndReset()`.

Advantages:

- multiple identical notifications collapse into one refresh;
- related work is deferred until the current message-stack operation finishes;
- independent refresh categories remain explicit;
- components can guard work based on whether dependent objects still exist.

Examples:

- `EditComponent` separately tracks structural track rebuilds, zoom changes, vertical layout, and note-off work;
- `PianoRollEditor` tracks note repainting, velocity repainting, note-property refresh, clip-set changes, active-track removal, keyboard layout, theme changes, and scrollbar changes;
- `PluginChainView` separates plug-in reconstruction from layout refresh; rack-content layout, drag-and-drop, list rows, and panel-toggle rendering are implemented by focused companion components rather than one monolithic source file.

### Flag handling rule

A flag should be reset even when its target object no longer exists, otherwise stale work may run after a new object is created. Conditions should therefore be structured carefully. Existing components commonly combine pointer checks with `compareAndReset`; changes to this pattern should be reviewed for stale flags.

## Selection model

### Global selection

`tracktion_engine::SelectionManager` is owned by `MainComponent` and shared through `EditViewState`. It can contain tracks, clips, automation selections, and aggregate MIDI-event selections.

Components listen to its `ChangeBroadcaster` interface and query the current typed selection when notified. They should not assume that a previously cached raw pointer remains selected or alive.

### MIDI note selection

`MidiViewport` owns a `te::SelectedMidiEvents` instance covering its cached MIDI clips. It:

1. creates the selection object for the current clip set;
2. registers itself as a `ChangeListener`;
3. forwards selection changes via its own `sendChangeMessage()`;
4. inserts the `SelectedMidiEvents` object into the global selection manager when notes are selected;
5. removes its listener before replacing or destroying the object.

`PianoRollEditor` listens both to the viewport and to the global selection manager. The viewport signal captures note-level changes; the global signal captures broader selection transitions and active-track changes.

### Pointer validity

Tracktion collections often expose raw pointers to objects owned by model containers. Those pointers become invalid when the model object is removed.

The `NotePropertiesBar` selection provider therefore does not blindly call `SelectedMidiEvents::clipForEvent()` for a potentially stale selected pointer. It walks the current cached clips and includes a note only if that clip's sequence still contains the pointer. Deleting selected notes also clears selection before removal.

`ClipPropertiesBar` queries the current typed clip selection on every refresh or application instead of treating its cached array as authoritative.

General rule: when a notification may arrive after deletion, re-resolve objects from the current model rather than trusting a cache.

## Component callback styles

NextStudio uses several callback mechanisms:

- `juce::ChangeBroadcaster` / `juce::ChangeListener` for coarse “something changed” notifications;
- `ValueTree` listeners for model/property details;
- direct `std::function` callbacks for tightly coupled child-to-parent UI events;
- `juce::Button::Listener` or `Button::onClick` for controls;
- `ApplicationCommandTarget` for commands and keyboard shortcuts;
- `MessageManager::callAsync()` for deferred message-thread work;
- timers for periodic autosave and view following.

Choose the narrowest mechanism that matches ownership:

- direct callback when the parent owns the child and the event has a clear semantic meaning (for example, `MidiViewport::NoteUnderMouseHandler`, which emits only when the pitch row changes);
- broadcaster when multiple observers or late binding are useful;
- `ValueTree` listener when the model itself is the source of truth;
- command manager when the action needs keyboard mapping or routing.

## Undo and state changes

### Model edits

User-visible project changes should use `m_edit.getUndoManager()`. A semantic operation starts a named transaction, performs all related mutations, and may call `beginNewTransaction()` afterward to prevent unrelated later edits from merging.

Examples from `NotePropertiesBar`:

- `Move MIDI Notes`;
- `Change MIDI Note Duration`;
- `Change MIDI Note Pitch`;
- `Change MIDI Note Velocity`.

A continuous scrub drag begins one transaction at drag start and applies subsequent deltas without starting new transactions.

### View edits

View-only changes use no undo manager. This includes timeline scroll/zoom and editor layout state.

### Automation touch state

Automatable controls use `AutomationWriteGuard` to identify touched parameters during write automation. That touch state is separate from selection and from the project undo manager.

### Save locking

`ScopedSaveLock` temporarily sets `EditViewState::m_isSavingLocked`. Operations that transiently rewrite source data or model structure can use this guard to prevent autosave from taking an inconsistent snapshot.

## Theme propagation

Theme values live in `ApplicationViewState::ThemeState`. `MainComponent` listens for theme changes, reapplies look-and-feel colors, and asks major children to refresh icons or local colors.

Some components also listen directly because they own cached drawables or colors. For example, `PianoRollEditor` schedules an icon/color update and forwards current text colors to `NotePropertiesBar`.

When adding a themed component:

1. obtain colors from `ApplicationViewState`, not hard-coded global state;
2. decide whether repainting is enough or resources must be regenerated;
3. register and remove listeners symmetrically;
4. ensure disabled and error states remain distinguishable in both built-in themes.

## Thread boundaries

### Message thread

JUCE components, selection changes, most Tracktion model mutations, and `ValueTree` callbacks are expected on the message thread unless an API explicitly documents otherwise.

### Autosave worker

`EditComponent` copies the edit state on the message thread, rewrites source paths in the copy, and passes that copy to a one-thread pool for serialization and atomic replacement. Completion is posted back with `MessageManager::callAsync()`.

`juce::Component::SafePointer` prevents the completion callback from using an `EditComponent` that has already been destroyed during project switching or shutdown.

### Audio thread

Audio callbacks are managed by Tracktion/JUCE. GUI listeners and file I/O must not be introduced into real-time processing paths. Plug-in DSP code should avoid allocation, locks, and message-thread APIs in audio callbacks.

## Event-flow examples

### Editing a note velocity in `NotePropertiesBar`

```text
User commits text
→ NotePropertiesBar validates all selected notes
→ starts undo transaction
→ MidiNote::setVelocity for each note
→ Tracktion updates NOTE state
→ PianoRollEditor receives ValueTree property notifications
→ flags note, velocity, and properties refresh
→ handleAsyncUpdate repaints and refreshes displayed values
```

### Selecting notes in the Piano Roll

```text
Tool calls MidiViewport::setNoteSelected
→ SelectedMidiEvents changes
→ SelectedMidiEvents broadcasts
→ MidiViewport::changeListenerCallback
→ MidiViewport broadcasts
→ PianoRollEditor refreshes NotePropertiesBar
→ SelectionManager also reflects SelectedMidiEvents for global command context
```

### Live MIDI feedback in the Piano Roll

```text
Physical or virtual MidiInputDevice receives a note change
→ Tracktion batches key-state changes in MidiKeyChangeDispatcher
→ PianoRollEditor receives the destination track and note arrays
→ callback is ignored unless the destination is the displayed track
→ KeyboardView applies note-ons followed by note-offs
→ PianoKeyboardDisplay updates transient active-note bits
→ affected key rectangles repaint with ApplicationViewState PrimeColour
```

The listener is attached to the long-lived `PianoRollEditor` and removed symmetrically in its destructor. The track-specific `KeyboardView` does not own the global subscription and may be safely replaced by `setTrack()`/`clearTrack()`.

### Changing the theme

```text
Settings/wizard mutates ApplicationViewState ThemeState
→ MainComponent ValueTree listener flags theme update
→ MainComponent reapplies look-and-feel and refreshes major children
→ components with direct theme listeners update cached icons/colors
→ repaint
```

## Contributor checklist

When introducing state or a new event path, answer these questions:

1. Is the state application-wide, project data, edit-local view state, or transient?
2. Should it persist, and where?
3. Should changing it create an undo step?
4. Which object is authoritative?
5. Who owns any pointers being cached?
6. Which listeners must be removed during teardown?
7. Can several notifications be coalesced?
8. Can the callback arrive after the target component or model object is gone?
9. Does any work cross the message/audio/background thread boundary?
10. Is there a focused test for parsing, validation, or lifecycle logic that can be separated from the GUI?

## Related documents

- [Architecture Overview](overview.md)
- [Project Lifecycle](project-lifecycle.md)
- [NotePropertiesBar](../components/note-properties-bar.md)
- [Testing](../development/testing.md)

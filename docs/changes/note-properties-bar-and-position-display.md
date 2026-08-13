# Change: Note Properties Bar and Position Display Helpers

## Summary

This change adds exact multi-note property editing to the Piano Roll and extracts bars/beats/ticks conversion from `PositionDisplayComponent` into shared, testable helpers.

It also strengthens MIDI selection lifetime, note deletion, active-track switching, and narrow-layout behavior.

## New files

### `App/include/NotePropertiesBar.h`

Declares `NotePropertiesBar`, including:

- the Start, End, Duration, Pitch, and Velocity property enum;
- an injected provider for `(MidiClip*, MidiNote*)` selection pairs;
- a specialized `PropertyEditor` supporting keyboard navigation, mouse wheel, and vertical scrubbing;
- parsing, formatting, validation, mutation, and undo helpers;
- state for multiple selection, mixed values, invalid input, focus, and layout.

### `App/src/NotePropertiesBar.cpp`

Implements:

- selected-note count;
- common and mixed values across a multiple selection;
- absolute and relative editing of all five properties;
- global/clip-internal beat conversion;
- bars/beats/ticks, note fraction, tick, pitch-name, MIDI-number, and velocity input;
- validation of the complete selection before mutation;
- semantic undo transactions;
- text entry through double-click, Enter/F2, Tab, and Escape;
- wheel and vertical-drag scrubbing;
- theme colors, invalid-state display, and proportional narrow-width layout.

See [NotePropertiesBar](../components/note-properties-bar.md) for its full contract.

## Modified files

### `App/include/MidiViewport.h`

`MidiViewport` now privately implements `juce::ChangeListener`. The callback forwards changes from its internal `te::SelectedMidiEvents` through the viewport's existing `ChangeBroadcaster` interface.

### `App/src/MidiViewport.cpp`

Selection lifecycle changes:

1. The viewport registers as a listener after creating `SelectedMidiEvents`.
2. It removes the listener before replacing or destroying that object.
3. Note-selection changes call the viewport's `sendChangeMessage()`, allowing `PianoRollEditor` to refresh dependent UI.
4. Deleting selected notes first captures valid `(clip, note)` pairs, then clears selection before removing model objects. This prevents the selection manager and properties UI from retaining deleted note pointers.

### `App/include/PianoRollEditor.h`

The Piano Roll now owns:

- a `NotePropertiesBar` member;
- an `m_updateNoteProperties` asynchronous refresh flag;
- `getNotePropertiesRect()` for the new layout row;
- selection handling for both global `SelectionManager` and `MidiViewport` changes.

### `App/src/PianoRollEditor.cpp`

Integration work:

1. Constructs and displays `NotePropertiesBar`.
2. Installs a defensive selection provider that resolves selected notes only against current cached MIDI clips.
3. Registers/removes the Piano Roll as a global selection-manager listener.
4. Allocates a 30-pixel properties row below the tool header.
5. Moves timeline, timeline helper, keyboard, grid, and playhead layout below the new row.
6. Paints the row background and lower separator.
7. Sends theme updates to `NotePropertiesBar::updateColours()`.
8. Schedules property refreshes for NOTE changes and note removal.
9. Clears stale note-property selection when the active track is removed or switched.
10. Removes the old `SelectedMidiEvents` from global selection directly during track clearing rather than calling `MidiViewport::unselectAll()`, which would reselect the old track.

### `App/include/PositionDisplayHelpers.h`

Adds two reusable functions:

```cpp
juce::String formatBarsBeatsTicks(
    const tracktion::tempo::Sequence&,
    tracktion::TimePosition,
    int ticksPerQuarterNote);

std::optional<tracktion::TimePosition> parseBarsBeatsTicks(
    const tracktion::tempo::Sequence&,
    const juce::String&,
    int ticksPerQuarterNote);
```

The API depends only on a tempo sequence and tick resolution, allowing reuse by both the transport position display and the note properties editor.

### `App/src/PositionDisplayHelpers.cpp`

Moves bars/beats/ticks conversion into the shared helper module.

Formatting behavior:

- bar and beat are displayed one-based;
- tick is zero-based and formatted with at least three digits;
- tick is limited to the valid PPQ range.

Parsing behavior:

- accepts `bar`, `bar.beat`, and `bar.beat.tick`;
- defaults missing beat to 1 and missing tick to 0;
- rejects empty components, more than three components, bar/beat below one, negative ticks, and ticks at or above PPQ;
- converts through `tracktion::tempo::Sequence` to `TimePosition`.

### `App/src/PositionDisplayComponent.cpp`

Removes the component-local bars/beats/ticks parser and formatter. Shared helpers are now used for:

- current transport position;
- Loop In display and commit;
- Loop Out display and commit;
- manually entered transport position.

Visible behavior remains equivalent while conversion becomes reusable and directly testable.

### `App/tests/PositionDisplayTests.cpp`

Adds bars/beats/ticks tests for:

- round-trip of `6.2.240`;
- abbreviated `6` and `6.2` inputs;
- too many components;
- bar zero;
- tick equal to PPQ and therefore outside the valid range.

## Robustness fixes made during review

### Active-track switching

Calling `MidiViewport::unselectAll()` while clearing the viewport deselects MIDI events but then ensures the viewport's track is selected. During a track switch, that could reintroduce the old track and interfere with lower-range active-track synchronization.

The fix removes the viewport's `SelectedMidiEvents` directly from the selection manager during teardown and does not select the old track.

### Deleted-note selection

Removing notes while they remained in `SelectedMidiEvents` could leave stale raw pointers in the property selection. Later `clipForEvent()` calls could assert or display obsolete values.

The fix captures ownership pairs, clears selection, removes notes, schedules a properties refresh on NOTE child removal, and filters provider output against current clip sequences.

### Narrow layout

The initial field layout consumed fixed widths left-to-right. On narrow windows, Pitch and Velocity could receive zero width.

The fix calculates preferred field widths and proportionally distributes all remaining width, preserving a non-exclusive share for every field.

## Documentation added later

The initial change introduced:

- `docs/components/note-properties-bar.md` (originally at `docs/note-properties-bar.md`);
- this change document;
- a documentation index.

The documentation was subsequently reorganized into user, component, architecture, development, and change sections and standardized on English.

## Validation

The implementation was validated with the portable public workflow:

```bash
./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

Parallelism may be tuned for a specific machine with `BUILD_JOBS=<count>`.

At the time of the change, both registered test suites passed. The `NotePropertiesBar` UI itself still relies on build/manual validation; dedicated parsing and model-application tests remain a recommended follow-up.

# Piano Roll Editor

## Purpose

The Piano Roll Editor is the MIDI note editor of NextStudio. It displays and edits MIDI notes on the active MIDI track, combining a note grid, piano keyboard, timeline, playhead, velocity lane, exact note-property editor, tool bar, scrollbar, and status footer.

This document describes the implementation: component ownership, the Tracktion data model, coordinate conversion, rendering, hit testing, the tool architecture, and how each note-editing operation is implemented. User-visible behavior is documented separately in [Piano Roll](../user/piano-roll.md).

## Source files

| Area | Files |
|---|---|
| Editor frame, layout, commands | `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp` |
| Note grid, hit testing, note operations | `App/include/MidiViewport.h`, `App/src/MidiViewport.cpp` |
| Pending paste state machine | `App/include/MidiPendingPaste.h`, `App/src/MidiPendingPaste.cpp` |
| Tool base and factory | `App/include/ToolStrategy.h`, `App/src/ToolFactory.cpp` |
| Pointer tool | `App/include/PointerTool.h`, `App/src/PointerTool.cpp` |
| Draw tool | `App/include/DrawTool.h`, `App/src/DrawTool.cpp` |
| Eraser tool | `App/include/EraserTool.h`, `App/src/EraserTool.cpp` |
| Knife tool | `App/include/KnifeTool.h`, `App/src/KnifeTool.cpp` |
| Lasso tool | `App/include/LassoTool.h`, `App/src/LassoTool.cpp` |
| Range tool | `App/include/RangeTool.h`, `App/src/RangeTool.cpp` |
| Lasso rectangle | `App/include/LassoSelectionTool.h`, `App/src/LassoSelectionTool.cpp` |
| Velocity lane | `App/include/VelocityEditor.h`, `App/src/VelocityEditor.cpp` |
| Exact note properties | `App/include/NotePropertiesBar.h`, `App/src/NotePropertiesBar.cpp` |
| Piano keyboard | `App/include/KeyboardView.h`, `App/src/KeyboardView.cpp` |
| Time axis | `App/include/TimeLineComponent.h`, `App/src/TimeLineComponent.cpp` |

## Component hierarchy and ownership

`LowerRangeComponent` owns one `PianoRollEditor`. The editor is created once and reused; `setTrack()` swaps the track-specific children when the active track changes.

```text
PianoRollEditor
├── MenuBar (tool bar)
├── NotePropertiesBar
├── TimeLineComponent
├── TimelineOverlayComponent
├── MidiViewport
│   ├── ToolStrategy (current tool)
│   └── LassoSelectionTool
├── VelocityEditor
├── KeyboardView
├── PlayheadComponent
└── juce::ScrollBar (horizontal)
```

`PianoRollEditor` is the composition root for the subsystem. It owns the layout rectangles, the tool-bar buttons, the application commands, the MIDI-note clipboard, and the refresh scheduling. `MidiViewport` owns the note grid, the current tool, the lasso rectangle, the `SelectedMidiEvents` object, the active pending-paste preview, and the clip cache.

The track-specific children (`MidiViewport`, `TimelineOverlayComponent`, `VelocityEditor`, `KeyboardView`) are created in `PianoRollEditor::setTrack()` and destroyed in `clearTrack()`. `clearTrack()` deselects the old `SelectedMidiEvents` object, removes listeners, and resets the unique pointers before the track reference is dropped.

## Data model

The editor does not keep its own note model. It operates on Tracktion Engine objects:

- `te::MidiClip` — a MIDI clip on the active track;
- `te::MidiList` — the clip's note sequence, obtained via `clip->getSequence()`;
- `te::MidiNote` — a single note, backed by a `juce::ValueTree` state plus cached `startBeat`, `lengthInBeats`, `noteNumber`, `velocity`, `colour`, and `mute` members;
- `te::SelectedMidiEvents` — the note selection, mapping each selected note to its owning clip.

The primary mutation APIs used by the editor are:

```cpp
clip->getSequence().addNote(pitch, startBeat, length, velocity, colour, &undoManager);
clip->getSequence().removeNote(note, &undoManager);
note->setStartAndLength(startBeat, length, &undoManager);
note->setNoteNumber(number, &undoManager);
note->setVelocity(velocity, &undoManager);
```

All model mutations pass the edit's undo manager. Multi-note operations open a named transaction first.

### Clip-content vs. project coordinates

Tracktion stores a note's start beat relative to the clip content. The editor displays notes in project coordinates. The conversion used by `MidiViewport` and `NotePropertiesBar` is:

```text
global beat = clip start + note start - clip offset
internal start = global start - clip start + clip offset
```

`EngineHelpers::getNoteStartBeat()` / `getNoteEndBeat()` (`App/include/Utilities.h`) provide the clip-relative note range used for rendering and hit testing.

## Coordinate systems

`MidiViewport` converts between three coordinate spaces:

1. **Pixels** — mouse positions and drawing rectangles.
2. **Beats / time** — musical position on the horizontal axis.
3. **MIDI note number** — pitch on the vertical axis.

### Horizontal

The timeline owns the horizontal mapping:

- `m_timeLine.xToBeatPos(x)` — pixel to beat;
- `m_timeLine.beatsToX(beats)` — beat to pixel;
- `m_timeLine.xToTimePos(x)` — pixel to edit time;
- `m_evs.beatsToX(beats, timeLineID, width)` — beat to pixel for a given view width.

### Vertical

Pitch mapping uses the edit-local vertical scroll and scale:

```cpp
float getKeyWidth() const { return m_evs.getViewYScale(m_timeLine.getTimeLineID()); }
float getStartKey() const { return m_evs.getViewYScroll(m_timeLine.getTimeLineID()); }

double getKeyForY(int y);   // pixel -> fractional MIDI note number
int getYForKey(double key); // MIDI note number -> pixel
```

`getNoteRect(noteNum, x1, x2)` computes the note rectangle from the note number and the two horizontal pixel edges.

## Rendering

`MidiViewport::paint()` draws, in order:

1. the track background;
2. key lines (`drawKeyLines`) — alternating shading for black/white keys;
3. bar and beat lines (`drawBarsAndBeatLines`);
4. for each cached clip: the clip range (`drawClipRange`) and every note (`drawNote`);
5. tool-specific overlays: dragged-note previews (`PointerTool`), the in-progress draw rectangle (`DrawTool`), and the knife split line (`KnifeTool`).

`drawNote()` clips the note rectangle to the viewport and, when `m_evs.m_editNotesOutsideClipRange` is false, to the owning clip. Note color is derived from the track color, darkened by velocity; hovered notes are brightened, and notes outside the clip range are grey. Selected notes get a white outline. The note name is drawn inside the note when the vertical scale is large enough.

`paintOverChildren()` draws the lasso rectangle via `LassoSelectionTool::drawLasso()`.

## Hit testing

`MidiViewport::getNoteByPos(pos)` is the central hit test. For every cached clip and every note it checks:

1. the note number matches `getNoteNumber(pos.y)`;
2. the click beat (converted to project coordinates) lies inside the note's start/end beat range.

It returns the first matching note. The implementation iterates linearly over all clips and notes; there is no spatial index.

Clip hit testing is separate:

- `getClipAt(x)` — clip whose edit time range contains the time at `x`;
- `getMidiClipAt(x)` — clip whose beat range contains the beat at `x`;
- `getNearestClipBefore(x)` / `getNearestClipAfter(x)` — nearest clip outside the click position.

## Tool architecture

Tools implement the strategy pattern. `MidiViewport` forwards mouse events to the active `ToolStrategy`:

```cpp
void MidiViewport::mouseDown(const juce::MouseEvent& e)
{
    if (m_currentTool)
        m_currentTool->mouseDown(e, *this);
    // double-click is forwarded separately
}
```

`ToolFactory::createTool(Tool, EditViewState&)` maps the `Tool` enum to a concrete implementation. The enum is declared in `App/include/Utilities.h`:

```cpp
enum class Tool { pointer, draw, range, eraser, knife, lasso, timestretch };
```

`timestretch` is declared but not implemented; the factory falls back to `PointerTool` for unknown values.

`MidiViewport::setTool()` deactivates the old tool, creates the new one, activates it, and broadcasts a change message so `PianoRollEditor` can update the tool-bar button states.

### Tool lifecycle

- `toolActivated()` — set the tool cursor;
- `toolDeactivated()` — cancel pending state, clear highlights, restore the cursor;
- `mouseDown/Drag/Up/Move/DoubleClick` — the interaction.

## Note operations

### Create

Two paths create notes:

1. **Draw tool** (`DrawTool`): `mouseDown` records the start pixel, the note number, and the minimum interval width derived from the current snap level. `mouseDrag` extends the end pixel. `mouseUp` converts start/end pixels to clip-relative beats, quantises them when snapping is active, and calls `MidiViewport::addNewNote()`.
2. **Pointer double-click** (`PointerTool::insertNoteAtPosition`): calls `MidiViewport::addNewNoteAt()`, which derives the note number and beat from the click position.

`MidiViewport::addNewNote()`:

1. resolves the length (remembered last note length, falling back to `0.25` beats);
2. calls `cleanUnderNote()` to clear conflicting same-pitch material;
3. calls `clip->getSequence().addNote(...)` with `m_evs.m_lastVelocity` as velocity.

The new note becomes selected. The draw tool also stores the resulting length as the remembered last note length.

### Select

`MidiViewport::setNoteSelected(note, addToSelection)` adds the note to the `SelectedMidiEvents` object and inserts that object into the global `SelectionManager`.

Selection paths:

- **Pointer click** — `PointerTool::mouseDown` hit-tests a note, clears the selection unless `Shift` is held or the note is already selected, then selects the note.
- **Lasso** — `LassoTool`/`RangeTool` drive `LassoSelectionTool`; `MidiViewport::updateLassoSelection()` selects every note whose pitch range and edit time range intersect the lasso rectangle.
- **Piano key** — `PianoRollEditor::handleKeyboardKeyClick()` selects all notes of a pitch in the selected MIDI clips on the active track. `Shift` toggles the pitch.

`MidiViewport::unselectAll()` deselects the `SelectedMidiEvents` object and reselects the track if it is not already selected.

### Move, resize, and copy

`PointerTool` implements all three with a single drag state machine. `mouseDown` classifies the gesture by the click position relative to the note rectangle:

- near the left edge → `DragMode::resizeLeft`;
- near the right edge → `DragMode::resizeRight`;
- otherwise → `DragMode::moveNotes`.

The edge tolerance is 10 pixels, or one third of the note width for narrow notes.

During the drag, `PointerTool` only computes deltas:

- `m_draggedTimeDelta` — horizontal time delta for moving;
- `m_draggedNoteDelta` — vertical pitch delta;
- `m_leftTimeDelta` / `m_rightTimeDelta` — edge resize deltas.

`MidiViewport::drawDraggedNotes()` renders a preview from these deltas without mutating the model. Guide notes audition pitch changes.

`mouseUp` commits the operation in three phases:

1. **Plan** — for every selected note, compute the destination start beat, length, and note number, and capture a full copy of the note state. Time conversion goes through `tempoSequence.toTime()` / `toBeats()` so tempo changes are respected. Resize deltas (`m_leftTimeDelta` / `m_rightTimeDelta`) only affect the note whose edge is being dragged; other selected notes keep their start and length. When `Ctrl` is held, the originals are kept (copy); otherwise they are removed.
2. **Clear** — group the planned destinations by clip and pitch, then call `cleanUnderNoteRanges()` once per group.
3. **Create** — rebuild each note from its captured state copy with the new pitch, start, and length, then select it. Rebuilding from the state copy preserves mute, colour, velocity, and any custom note properties.

The transaction is named `Copy MIDI Notes` or `Move MIDI Notes`. Resizing a single note updates the remembered last note length.

During a copy, the source notes are still present when the destination is cleared. `cleanUnderNote()` therefore also trims or removes a source note when the destination overlaps it, keeping the pitch monophonic in the affected range. This is intentional: dragging right trims the source's end, dragging left trims its start, and an exactly covering destination removes the source.

### Delete

- **Eraser tool** (`EraserTool`): a single click deletes immediately. A drag collects notes and commits them as one `Delete MIDI Notes` transaction. A double-click deletes all notes in the same clip whose start beat matches within a small tolerance.
- **Keyboard** (`PianoRollEditor::perform` → `MidiViewport::deleteSelectedNotes()`): collects selected note/clip pairs, clears the selection first, then removes each note.

Clearing selection before removal prevents the properties bar from retaining deleted pointers.

### Split

`KnifeTool::mouseDown` hit-tests a note, computes the split beat from the click position (snapped unless `Shift` is held), and verifies the split lies strictly inside the note. It then:

1. truncates the original note with `setStartAndLength()`;
2. adds the second segment from a full state copy, preserving mute, colour, velocity, and any custom note properties.

The operation is grouped as `Split MIDI Note`. A vertical preview line is drawn while hovering over a note.

### Provisional copy and paste in place

`PianoRollEditor` registers `Command+C` and `Command+V` as note commands. The clipboard contains each source clip ID and a deep copy of the note state.

`Command+V` calls `MidiViewport::beginPendingPaste()`. It validates the source clips, clears the real note selection, starts `MidiPendingPaste::State`, and stores the clipboard entries as transient preview data. No Tracktion note is created and no undo transaction begins.

`drawPendingPasteNotes()` renders each copied state with the pending beat/pitch deltas as a translucent selected outline. Arrow commands are intercepted before normal `SelectedMidiEvents::nudge()`:

- horizontal steps use the same `TimecodeSnapType::roundTimeDown/Up()` calculation as Tracktion's note nudge;
- vertical steps accumulate semitone or octave deltas while clamping the group to `0..127`;
- the state machine records whether any effective movement occurred.

The pending state resolves as follows:

| Event | Result |
|---|---|
| Deselect/click without movement | Cancel; no model or undo change |
| Deselect/click after movement | Commit copies, leave them deselected |
| `Enter` | Commit positively, even at zero offset; select the new notes |
| `Escape` | Cancel regardless of movement |
| Tool or track change | Resolve using deselect semantics |

`Enter` is also handled by `MainComponent` before its global Play command, so an active pending paste is confirmed instead of starting playback.

Commit resolves clips by ID, begins one `Paste MIDI Notes` transaction, groups destination ranges by clip/pitch, clears all existing same-pitch material under those ranges, and creates notes from full copied states. The pasted destination has priority: a source or right-hand note that still overlaps it is trimmed/removed, never allowed to cover or shorten the pasted note. Source notes remain complete only when the moved preview no longer overlaps them. An explicit zero-offset `Enter` therefore replaces the notes at the same ranges and selects the replacements instead of stacking duplicate note events.

The pure `MidiPendingPaste::State` contains only active/moved flags and accumulated offsets. It is independent of JUCE/Tracktion model mutation and is covered by `MidiPendingPasteTests`.

### Duplicate

`MidiViewport::duplicateSelectedNotes()` (bound to `Command+D`) copies the selection by the length of the selected time range:

1. capture each source note's full state and owning clip;
2. clear the old selection;
3. clear all destination ranges (grouped by clip and pitch) before creating any notes;
4. rebuild the copies from the captured state and select them.

Clearing all destinations first prevents one duplicate from erasing another when several selected notes share a pitch. Rebuilding from the captured state preserves mute, colour, velocity, and any custom note properties.

### Nudge

`PianoRollEditor::perform()` delegates arrow-key nudging to `MidiViewport`. Horizontal steps use the selected fixed note value, the current adaptive snap type, or one tick when snapping is off. Pitch nudges use one semitone, or twelve semitones with `Command`.

### Velocity

`VelocityEditor` draws one vertical stem and handle per note. `mouseDown` records the hovered note and, if it belongs to the current `SelectedMidiEvents`, all selected notes with their starting velocities. `mouseDrag` applies the same vertical delta to each note via `setVelocity()`, clamped to `0..127`, and updates `m_evs.m_lastVelocity`.

The exact properties bar clamps committed velocity to `1..127`, so velocity-zero behavior differs between the two paths.

### Exact properties

`NotePropertiesBar` edits start, end, duration, pitch, and velocity as text fields with absolute and relative input, wheel stepping, and vertical drag scrubbing. It is documented in detail in [NotePropertiesBar](note-properties-bar.md).

## Overlap handling

Overlap clearing is split into a pure planning step and a mutation step.

`MidiNoteOverlap::subtractIntervals()` (`App/include/MidiNoteOverlap.h`, `App/src/MidiNoteOverlap.cpp`) is a pure function that subtracts a set of clear intervals from a note interval and returns the remaining pieces. It merges overlapping/adjacent clears, drops sub-epsilon pieces, and has no Tracktion or JUCE dependency, so it is unit-tested in isolation.

`MidiViewport::cleanUnderNoteRanges(noteNumb, ranges, clip)` applies the plan to a clip:

1. convert the clear ranges to intervals;
2. iterate over a copy of the clip's notes (the sequence may be mutated);
3. for each same-pitch note, compute the remaining pieces;
4. remove the note if nothing remains, deselecting it first;
5. otherwise trim the original note to the first piece and add the remaining pieces as new notes from a full state copy.

`cleanUnderNote()` delegates to `cleanUnderNoteRanges()` with a single range. The batch variant is used by move/copy and duplication, which group destinations by clip and pitch so each clip is scanned once per pitch instead of once per note.

In the copy path, source notes are intentionally not excluded, so a destination that overlaps a source note trims or removes the source.

## Selection model

`MidiViewport` owns a `te::SelectedMidiEvents` instance covering its cached clips. It:

1. creates the object in `updateSelectedEvents()`;
2. registers itself as a `ChangeListener`;
3. forwards selection changes through its own `sendChangeMessage()`;
4. inserts the object into the global `SelectionManager` when notes are selected;
5. removes its listener before replacing or destroying the object.

`PianoRollEditor` listens to both the viewport and the global selection manager. The viewport signal captures note-level changes; the global signal captures broader selection transitions and active-track changes.

`SelectedMidiEvents::clipForEvent(note)` maps a selected note to its owning clip. Because Tracktion exposes raw pointers, the `NotePropertiesBar` selection provider re-resolves notes against the current clip sequences instead of trusting a possibly stale pointer.

## Clip cache

`MidiViewport` caches the track's MIDI clips in `m_cachedClips` with a validity flag:

- `getCachedMidiClips()` refreshes the cache when invalid or when the track is null;
- `refreshClipCache()` calls `EngineHelpers::getMidiClipsOfTrack(*m_track)`;
- `invalidateClipCache()` marks the cache invalid.

The cache is invalidated when a `MIDICLIP` child is added to or removed from the track state. This avoids re-querying the track on every paint and hit test.

## Undo transactions

| Operation | Transaction name |
|---|---|
| Move notes | `Move MIDI Notes` |
| Copy notes by drag | `Copy MIDI Notes` |
| Paste notes in place | `Paste MIDI Notes` |
| Add note | `Add MIDI Note` |
| Duplicate notes | `Duplicate MIDI Notes` |
| Delete notes (drag) | `Delete MIDI Notes` |
| Split note | `Split MIDI Note` |
| Property start | `Move MIDI Notes` |
| Property end/duration | `Change MIDI Note Duration` |
| Property pitch | `Change MIDI Note Pitch` |
| Property velocity | `Change MIDI Note Velocity` |

View-only changes (scroll, zoom, tool selection, hover) do not create undo steps.

## Event flow and refresh scheduling

`PianoRollEditor` implements `te::ValueTreeAllEventListener` and `FlaggedAsyncUpdater`. Model notifications set named flags, and `handleAsyncUpdate()` consumes them with `compareAndReset()`:

- NOTE property changes → note repaint, velocity repaint, properties refresh;
- timeline property changes → keyboard layout, scrollbar update;
- clip child added/removed → clip-set update (`updateSelectedEvents`);
- track state removed → `clearTrack()`;
- theme changes → button colour update.

This coalesces the many notifications produced by a single multi-note operation into one deferred refresh.

## Known limitations

- `PointerTool::updateCursor()` is dead code; `mouseMove()` performs cursor handling directly.
- `EraserTool::highlightNoteForDeletion()` and `clearHighlights()` are empty placeholders; the custom cursor is the primary deletion feedback.
- `VelocityEditor::mouseWheelMove()` is empty.
- `Tool::timestretch` is declared but not implemented.
- `getNoteByPos()` iterates linearly over all notes; there is no spatial index, which may become slow for large clips.
- Velocity-zero behavior differs between `VelocityEditor` (allows 0) and `NotePropertiesBar` (clamps to 1).

## Related documents

- [Piano Roll](../user/piano-roll.md)
- [NotePropertiesBar](note-properties-bar.md)
- [Architecture Overview](../architecture/overview.md)
- [State and Event Model](../architecture/state-and-events.md)

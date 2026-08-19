# Piano Roll

## Overview

The Piano Roll edits MIDI notes on the active MIDI track. It combines a note grid, piano keyboard, timeline, playhead, velocity editor, exact note-property editor, tool bar, scrollbar, and status footer.

The MIDI Editor tab in the lower range becomes available when a MIDI clip is selected.

## Open the Piano Roll

1. Select a MIDI clip in the arrangement.
2. Open **MIDI Editor** in the lower range.
3. Resize the lower range by dragging its upper splitter if needed.

The editor follows the selected clip's track. It may display and edit MIDI material from multiple clips on that track. Keyboard-wide pitch selection specifically operates on selected MIDI clips belonging to the active track.

If no active track is available, the grid displays a prompt to select a MIDI clip.

## Layout

From top to bottom:

1. **Tool bar** — pointer, draw, range, eraser, knife, and lasso.
2. **Note properties bar** — selection count, exact note properties, position snapping, and inserted-note length.
3. **Timeline** — musical time and overlay above the note grid.
4. **Piano keyboard and note grid** — pitch vertically, time horizontally.
5. **Velocity editor** — velocity stems/handles for notes.
6. **Horizontal scrollbar and footer** — horizontal position and note name under the pointer.

The playhead overlays the timeline/grid region.

## Note selection

### Pointer selection

With the pointer tool:

- click a note to select it;
- `Shift`-click adds a note to the current selection;
- click empty space to clear the MIDI-note selection;
- drag from empty space to begin lasso selection.

The note selection is represented by a Tracktion `SelectedMidiEvents` object and is shared with command handling and the properties bar.

### Lasso selection

Choose the lasso tool or drag empty space with the pointer tool. Drag a rectangle over notes and release. The dedicated lasso tool returns to the pointer tool after completion.

### Range selection

The range tool uses a time-range-style lasso. It remains the active tool after release.

### Select notes by piano key

Clicking a key in the left keyboard selects notes of that pitch in the selected MIDI clips on the active track and auditions the pitch.

- normal click replaces the note selection with all matching notes;
- `Shift`-click toggles that pitch: if any matching notes are selected, all matching notes are removed; otherwise they are added.

Dragging across the keyboard auditions successive pitches.

## Tools

### Pointer

The pointer tool selects, moves, copies, resizes, and inserts notes.

#### Select and move

Click the body of a note and drag:

- horizontal movement changes time;
- vertical movement changes pitch;
- selected notes move together;
- guide notes audition pitch changes during the drag.

Snapping is enabled during drag. Hold `Shift` to temporarily disable it.

#### Copy and paste in place

`Command+C` copies the selected notes. `Command+V` starts a provisional in-place paste: preview blocks appear exactly over the source notes, but the project model and undo history are not changed yet.

While the paste is pending:

- arrow keys move the preview using the same snap and pitch steps as normal note nudging;
- `Command+Up` / `Command+Down` move the preview by one octave;
- `Enter` positively commits the paste, including an unshifted in-place paste;
- `Escape` always cancels the paste;
- clicking/deselecting without moving cancels it, leaving only the originals;
- clicking/deselecting after moving commits the copies at their preview positions.

The pasted destination always has priority during commit. Source notes remain unchanged when the moved preview no longer overlaps them; if a preview still overlaps a source or another same-pitch note, the existing note is trimmed or removed so the pasted note remains complete. Confirming an unshifted preview with `Enter` replaces the notes at the same ranges rather than stacking duplicate note events. The complete commit is one `Paste MIDI Notes` undo transaction.

#### Copy while dragging

Hold `Ctrl` during mouse release after dragging to create copies instead of removing the original notes. This path currently checks the physical Control modifier directly.

Dragging only previews the copy; the model is changed on drop. The destination range is cleared of same-pitch material before the copy is inserted. If the destination overlaps the original note, the overlapping part of the original is removed:

- dragging right trims the original's end;
- dragging left trims the original's start;
- a destination that exactly covers the original removes it.

This keeps the pitch monophonic in the affected range.

#### Resize

Drag the left or right edge of a note. The cursor changes near resizable edges. Resizing does not change the remembered last-inserted length.

#### Insert by double-click

Double-click empty grid space over a MIDI clip to insert a note using the length selected by **INSERT LENGHT**. The previous MIDI-note selection is cleared and the inserted note becomes selected.

#### Overlap handling

Move/copy/create operations clear conflicting note material of the same pitch in the destination range. Depending on the overlap, existing notes may be removed, trimmed, or split so same-pitch destinations do not overlap.

### Draw

The draw tool creates a note by dragging from its start to end.

- drawing is possible only over an existing MIDI clip;
- the initial minimum width follows the value selected by **INSERT LENGHT**;
- dragging right extends the note;
- `Shift` temporarily disables position snapping;
- the resulting inserted length becomes the remembered **Last Inserted** length;
- the new note becomes selected.

Double-click performs a minimal draw operation using the selected inserted-note length.

### Range

Drag to select a time/pitch range. This tool delegates to the Piano Roll's range-aware lasso implementation.

### Eraser

- click a note to delete it immediately;
- drag across notes to collect and delete them as one named undo transaction;
- double-click a note to delete all notes in the same clip whose start beat matches within a small tolerance.

The cursor indicates eraser mode. Hover deletion highlighting is currently limited; the custom cursor is the primary feedback.

### Knife

Click inside a note to split it at the cursor.

- the split follows the grid;
- hold `Shift` to bypass snapping;
- the split must lie strictly inside the note;
- the original note becomes the first segment and a second segment is added with the same pitch, velocity, and color;
- the operation is grouped as `Split MIDI Note` in undo history.

A vertical split preview is drawn while hovering over a note.

### Lasso

Drag to select notes in a rectangular area. Releasing switches back to the pointer tool.

## Snapping

Use the **SNAP** selector in the note-properties bar to choose **Off**, a fixed note value from **1/1** through **1/128**, or **Adaptive**. Adaptive snapping derives its resolution from the timeline zoom and is the default.

Hold `Shift` during pointer move/resize, drawing, or knife splitting to bypass enabled snapping temporarily.

## Inserted-note length

Use the independent **INSERT LENGHT** selector next to **SNAP** to choose:

- **Adaptive** — the zoom-dependent interval that adaptive snapping would use, regardless of the selected SNAP mode;
- **Last Inserted** — the actual duration of the most recently created note on the active track timeline;
- a fixed note value from **1/1** through **1/128**.

The selected length is the default and minimum duration for click/double-click drawing. Dragging farther creates a longer note. Creating a note updates **Last Inserted**; resizing an existing note does not. The default mode is **Last Inserted**.

View, snap, and inserted-note-length state are stored in edit-local UI state and do not create musical undo steps.

## Exact note properties

The properties bar displays:

- number of selected notes;
- `START`;
- `END`;
- `DURATION`;
- `PITCH`;
- `VELOCITY`.

If selected notes have different values, the field displays `—`. Entering a value applies it to all selected notes.

### Editing a field

- double-click to type;
- or focus the read-only field and press `Enter`/`F2`;
- press `Enter` to commit;
- press `Escape` to cancel;
- use `Tab` or `Shift+Tab` to commit and move between fields;
- use the mouse wheel for one-step changes;
- drag vertically for continuous scrubbing.

Invalid active input is shown in red.

### Supported values

| Property | Examples |
|---|---|
| Start/end | `6.2.240`, `6.2`, `6`, `+1/16`, `-120 ticks` |
| Duration | `1/4`, `1/16`, `960 ticks`, `+1/16` |
| Pitch | `60`, `C3`, `G#4`, `Bb2`, `+1 st`, `-12 st` |
| Velocity | `100`, `+5`, `-10` |

Detailed parsing and validation behavior is documented in [NotePropertiesBar](../components/note-properties-bar.md).

## Velocity editor

The velocity lane draws one vertical stem and handle for each visible note.

- move over a handle to mark its note as hovered;
- drag a handle vertically to change velocity;
- if the hovered note belongs to the current selected MIDI events, all selected notes are changed by the same delta from their individual starting velocities;
- otherwise only the hovered note changes;
- values are clamped to `0..127` in the velocity-lane drag path;
- the reference value updates the remembered last velocity.

Dragging the Velocity field in the exact properties bar previews the changed stems and handles immediately in the velocity lane, before the values are committed on release.

The exact properties bar uses a `1..127` clamp for committed property values, so velocity zero behavior differs between these two editing paths in the current implementation.

## Navigation and zoom

### Horizontal scroll

Use the bottom horizontal scrollbar to move through musical time.

In the note grid:

- `Shift` + mouse wheel scrolls horizontally;
- `Command` modifier + mouse wheel zooms horizontally around the pointer position (`Ctrl` on Windows/Linux, `Command` on macOS through JUCE's command modifier);
- plain mouse wheel scrolls vertically through pitches.

Horizontal zoom is limited to a broad safe range to prevent invalid or unusably extreme view lengths.

### Vertical keyboard zoom

Dragging from the keyboard area into its surrounding component can change vertical scale and scroll. The calculation keeps the initially clicked pitch anchored while clamping visible pitch count and MIDI range.

The Piano Roll's vertical scroll and scale are stored per active-track timeline ID.

## Note name under the pointer

The footer displays the pitch row currently under the pointer while the pointer is inside the note grid. It updates when the pointer crosses into another pitch row and clears when the pointer leaves the grid. Vertical scrolling, zooming, and layout changes recalculate the value even when the pointer itself has not moved. Displayed values are limited to valid MIDI notes `0..127`.

## Keyboard shortcuts

`Command` means `Ctrl` on Windows/Linux and `Command` on macOS.

| Action | Shortcut |
|---|---|
| Delete selected notes | `Backspace`, `Delete`, or `Command+X` |
| Copy selected notes | `Command+C` |
| Paste notes in place (pending) | `Command+V` |
| Confirm pending paste | `Enter` |
| Cancel pending paste | `Escape` |
| Duplicate selected notes | `Command+D` |
| Nudge pitch up/down | `Up` / `Down` |
| Nudge time left/right | `Left` / `Right` |
| Nudge one octave | `Command+Up` / `Command+Down` |

Time nudging uses the current best snap type. Pitch nudges use one semitone or twelve semitones.

## Duplication behavior

`Command+D` duplicates the current MIDI-note selection by the selected time range length:

1. source note properties and owning clips are captured;
2. the old selection is cleared;
3. all destination ranges are cleared before any new notes are created;
4. copied notes are added and selected.

Clearing all destinations first prevents one newly created duplicate from erasing another duplicate when several selected notes share a pitch.

## Undo and model updates

Piano Roll operations use the edit's undo manager. Named transactions are used for multi-note changes, copying, movement, splitting, deletion, and property editing. Copying and the provisional `Command+V` preview do not change the model; committing the preview creates one `Paste MIDI Notes` transaction.

Model changes trigger deferred refreshes for:

- note rendering;
- velocity rendering;
- exact property values;
- clip cache/selection coverage;
- keyboard layout;
- scrollbar state.

Deleting selected notes clears selection before removing model objects so property UI cannot retain deleted pointers.

## Clip offsets and displayed time

MIDI notes store positions in clip-content coordinates. The Piano Roll displays notes in project coordinates by accounting for clip start and content offset.

The properties bar uses:

```text
global beat = clip start + note start - clip offset
```

When setting a global start, it converts back to clip coordinates before updating the note.

## Limitations and current behavior

- The MIDI Editor tab requires a selected MIDI clip.
- The active view is track-oriented and may show multiple MIDI clips on that track.
- Some operations use selected clips, while the viewport's note selection can cover all cached MIDI clips on the active track; be mindful of selection context.
- Velocity lane dragging permits zero, while the exact velocity property clamps to one or higher.
- Eraser hover preview is minimal.
- Time-stretch is an arrangement tool and is not implemented as a Piano Roll `ToolStrategy`.

## Related documents

- [NotePropertiesBar](../components/note-properties-bar.md)
- [Getting Started](getting-started.md)
- [Architecture Overview](../architecture/overview.md)
- [State and Event Model](../architecture/state-and-events.md)

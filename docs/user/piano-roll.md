# Piano Roll

## Overview

The Piano Roll edits MIDI notes on the active MIDI track. It combines a note grid, piano keyboard, timeline, playhead, velocity editor, exact note-property editor, tool bar, scrollbar, and status footer.

The MIDI Editor tab in the lower range becomes available when a MIDI clip is selected.

## Open the Piano Roll

1. Select a MIDI clip in the arrangement.
2. Open **MIDI Editor** in the lower range, or double-click the MIDI clip in the arrangement. If the lower range is collapsed, a double-click expands it automatically.
3. Resize the lower range by dragging its upper splitter if needed. Drag upward to enlarge the Piano Roll, or drag downward to shrink it back toward the standard height before collapsing the lower range. The maximum height stops at the bottom edge of the Song Editor timeline.

The editor follows the selected clip's track. It may display and edit MIDI material from multiple clips on that track. Keyboard-wide pitch selection uses the explicitly selected MIDI clips belonging to the active track and retains that clip scope during note selection.

If no active track is available, the grid displays a prompt to select a MIDI clip.

## Layout

From top to bottom:

1. **Tool bar** — the note-audition toggle is left-aligned; pointer, draw, range, eraser, knife, and lasso remain grouped in the center; Follow Playhead is right-aligned.
2. **Note properties bar** — selection count, exact note properties, position snapping, and inserted-note length.
3. **Timeline** — musical time and overlay above the note grid.
4. **Piano keyboard and note grid** — pitch vertically, time horizontally.
5. **Velocity editor** — velocity stems/handles for notes.
6. **Horizontal scrollbar and footer** — horizontal position and note name under the pointer.

The playhead overlays the timeline/grid region. Click the timeline ruler to position it; clicks in the note grid edit or select notes and do not reposition the playhead. The right-aligned Follow Playhead button enables automatic scrolling for the current MIDI timeline and defaults to off. Right-click it to select smooth Page or centered Continuous scrolling. Its state is independent from the Song Editor and other MIDI timelines. Alternating timeline bands provide orientation without hiding piano-key rows, clip-range shading, notes, or editing previews. The band tint follows the selected theme while its subtle intensity is controlled by the editor.

## Live timing and snap feedback

Start, End and Duration follow the note being moved/copied/resized, and Pitch
follows vertical movement. Draw shows the new note's values from mouse-down without
adding it to the selection count. During dragging only the text color changes
subtly; there is no extra Preview label. Release/cancel restores normal color and
the current selection display. `NOTES (REF):` identifies the grabbed reference in
a multi-note gesture while retaining the real selection count.

A held snap target shows a thin guide and outlined diamond at the manipulated
start/end edge, with status below SNAP. Between targets the held guide disappears;
Shift/Off removes it immediately even without pointer movement. Duration/start
limits are not snap targets. Knife accents its existing split line. These signals
do not change the grid setting, magnetic strength or keyboard/numeric snap policy.

## Note selection

### Pointer selection

With the pointer tool:

- click a note to select it;
- `Shift`-click adds a note to the current selection;
- right-click a note to delete only that note; right-clicking empty space leaves the selection unchanged;
- click empty space to clear the MIDI-note selection;
- drag from empty space to begin lasso selection.

### Lasso selection

Choose the lasso tool or drag empty space with the pointer tool. Drag a rectangle over notes and release. Normal dragging replaces the selection; hold `Shift` to add or `Ctrl/Command` to toggle enclosed notes. Changing modifiers or shrinking the rectangle recomputes from the original selection. Escape or changing tools cancels and restores that selection. The musical start stays anchored while navigating the view. The dedicated lasso tool returns to Pointer; an empty-space Pointer gesture stays on Pointer.

### Range selection

The range tool independently selects a musical time/pitch interval and displays a translucent preview. It remains active after release. Escape cancels the gesture and restores the previous note selection.

### Select notes by piano key

Clicking a key in the left keyboard selects notes of that pitch in the targeted MIDI clips on the active track and auditions the pitch. The last explicit clip selection remains the keyboard's target after grid/velocity lassos, Range selection or clearing notes. Selecting different clips updates that target; it does not automatically expand to every clip on the track.

- normal click replaces the note selection with all matching notes;
- `Shift`-click toggles that pitch: if any matching notes are selected, all matching notes are removed; otherwise they are added.

Dragging across the keyboard auditions successive pitches.

### Live MIDI key feedback

The keyboard lights keys for live MIDI routed to the track currently displayed in the Piano Roll. This includes external MIDI devices and NextStudio's virtual MIDI input. Held white keys use the application's PrimeColour; held black keys use a darker variant. Releasing a note restores the normal key color.

The feedback is track-aware: MIDI routed to a different track does not light the displayed keyboard. Several simultaneously held notes can be shown at once. Lighting indicates only held/released state and does not vary with velocity. It visualizes live input, not notes played back from MIDI clips in the arrangement.

Fast dragging across the clickable keyboard auditions successive pitches. Releasing the mouse stops all notes triggered by that gesture.

### Note audition

The left-aligned music-note-and-pencil button in the tool bar enables or disables note audition. It is enabled by default and its state persists across application sessions. When enabled:

- Draw sounds the fixed pitch for as long as the mouse button is held;
- Pointer double-click insertion gives the new note a short preview;
- moving notes auditions pitch changes.

Release, Escape, tool or track changes, disabling the button, and editor/application teardown stop active guide notes on the MIDI channel assigned to each clip. The toggle affects editing previews only; clicking or dragging the piano keyboard continues to audition pitches directly.

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

`Command+C` copies the selected notes. `Command+V` starts a provisional in-place paste: preview blocks appear over the source notes, but the project is not changed until the paste is confirmed.

While the paste is pending:

- arrow keys move the preview using the same snap and pitch steps as normal note nudging;
- `Command+Up` / `Command+Down` move the preview by one octave;
- `Enter` positively commits the paste, including an unshifted in-place paste;
- `Escape` always cancels the paste;
- clicking/deselecting without moving cancels it, leaving only the originals;
- clicking/deselecting after moving commits the copies at their preview positions.

The pasted destination has priority. If a preview overlaps another note of the same pitch, the existing note is trimmed or removed so the pasted note remains complete. Confirming an unshifted preview with `Enter` replaces notes at the same ranges instead of stacking duplicates. The complete paste can be undone in one step.

#### Copy while dragging

Hold the physical `Ctrl` key while releasing the mouse after a drag to create copies instead of moving the original notes.

Dragging previews the copy; the project changes when the mouse is released. The destination range is cleared of same-pitch material before the copy is inserted. If the destination overlaps the original note, the overlapping part of the original is removed:

- dragging right trims the original's end;
- dragging left trims the original's start;
- a destination that exactly covers the original removes it.

This keeps the pitch monophonic in the affected range.

#### Resize

Drag the left or right edge of a note. The cursor changes near resizable edges. If the dragged note is part of a multi-note selection, the same edge delta is applied to every selected note, matching the drag preview. Dragging a left edge changes each selected note's start and duration; dragging a right edge changes each duration while preserving its start. Resizing does not change the remembered last-inserted length.

#### Auto-scroll while dragging

Moving notes into a grid edge automatically scrolls the Piano Roll horizontally and vertically. The speed increases as the pointer approaches or passes the edge, and scrolling continues while the pointer is held outside the grid. The grabbed reference note remains at the visible boundary. Horizontal auto-scroll also applies while resizing a note edge; resize gestures do not scroll vertically. Scrolling stops in a direction when the selection cannot move any farther, including at its left movement or minimum-duration limit. For multi-note moves, vertical scrolling stops as soon as any selected note reaches MIDI pitch 0 or 127. Beat zero and the visible timeline and piano-key ranges also limit scrolling without changing the existing move, copy, snap, or undo behavior.

#### Insert by double-click

Double-click empty grid space over a MIDI clip to insert a note using the length selected by **INSERT LENGHT**. The previous MIDI-note selection is cleared and the inserted note becomes selected. With note audition enabled, the inserted pitch plays briefly.

#### Overlap handling

Move/copy/create operations clear conflicting note material of the same pitch in the destination range. Depending on the overlap, existing notes may be removed, trimmed, or split so same-pitch destinations do not overlap.

### Draw

The draw tool creates a note over an existing MIDI clip. Its pencil cursor is shown over clip time ranges; outside clips and in clip gaps, the normal pointer remains visible. Clicking there does not create a note or clip. An ongoing draw retains the pencil while crossing a clip boundary.

- clicking shows the selected **INSERT LENGHT** default immediately and, when note audition is enabled, sustains that pitch until release;
- with snapping enabled, the start aligns downward and the default end aligns to the first snap point at or after that duration. A coarser snap therefore immediately gives the note an end at the next snap point;
- dragging changes this initial end relatively: right extends, left shortens, including below both the insert length and snap interval;
- nearby snap points are magnetic detents. Continue moving to pull the end free and reach any intermediate position;
- the start and pitch stay fixed during drawing; the minimum duration is one tick (`1/960` beat);
- `Shift` bypasses magnetic snapping. Holding it at mouse-down also bypasses initial alignment;
- Escape or changing tools cancels the provisional draw;
- the actual inserted duration becomes **Last Inserted**, and the new note becomes selected.

Double-click uses the same default-range rule. Right-clicking an existing note deletes only that note as an undoable edit; right-clicking empty space does not begin drawing or change selection. Draw otherwise remains a creation tool with overlap cleanup; use Pointer edges to resize an existing note in place.

### Range

Drag to select a time/pitch interval using the independent Range gesture. Its bounds are not magnetically snapped, and it does not use object-lasso modifier rules.

### Eraser

- click a note to delete it immediately;
- drag across notes to collect and delete them in one undoable action;
- double-click a note to delete all notes in the same clip whose start beat matches within a small tolerance.

The eraser cursor remains visible throughout MIDI clip time ranges, including between notes. Outside clips and in clip gaps it becomes the normal pointer, like Draw and Knife. Hover deletion highlighting is currently limited.

### Knife

Click inside a note to split it at the cursor.

- the split position is magnetically attracted to nearby snap points but can lie between them;
- hold `Shift` to bypass snapping;
- the split must lie strictly inside the note;
- the original note becomes the first segment and a second segment is added with the same pitch, velocity, and color;
- the split can be undone in one step.

The Knife cursor stays visible throughout MIDI clip time ranges, including between notes; outside clips and in clip gaps it becomes the normal pointer. A vertical split preview is drawn only over a note at a valid split position; a missing note within a clip does not change the tool cursor.

### Lasso

Drag to select notes in a rectangular area. Releasing switches back to the pointer tool. Lasso and Range show their selection cursors over empty space too, since a selection can begin there. Pointer retains its normal cursor in empty space and move/resize feedback over notes. Ongoing drags retain the cursor of their current action.

## Snapping

Use the **SNAP** selector in the note-properties bar to choose **Off**, a fixed note value from **1/1** through **1/128**, or **Adaptive**. Adaptive snapping derives its resolution from the timeline zoom and is the default.

Mouse move/resize, drawing, and knife preview use **soft snapping**: the edited position stays on a nearby snap point until you pull it free, then moves continuously toward the next point. The mouse pointer itself is not moved.

Hold `Shift` to bypass enabled snapping temporarily. Changing the modifier can produce a small bounded position change, without accumulating offsets. Keyboard nudging, explicit quantization, exact property edits, and playhead positioning on the timeline ruler retain discrete behavior.

## Inserted-note length

Use the independent **INSERT LENGHT** selector next to **SNAP** to choose:

- **Adaptive** — the zoom-dependent interval that adaptive snapping would use, regardless of the selected SNAP mode;
- **Last Inserted** — the actual duration of the most recently created note on the active track timeline;
- a fixed note value from **1/1** through **1/128**.

The selected length is a click default, **not a drag minimum**. With snapping enabled, the default end rounds upward to the next valid snap point, including fractional Last Inserted durations; Off/Shift keeps the raw duration. Drawing can shorten below this default. Creating a note updates **Last Inserted** with the actual committed duration; resizing an existing note does not. The default mode is **Last Inserted**.

View, snap, and inserted-note-length settings are restored with the project and do not create musical undo steps.

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
- press `Enter` to commit; leaving the field also commits valid input, including clicks into the note grid or velocity lane;
- press `Escape` to cancel;
- use `Tab` or `Shift+Tab` to commit and move between fields;
- use the mouse wheel for one-step changes;
- drag vertically for continuous scrubbing.

Invalid active input is shown in red. Invalid Enter/Tab keeps the field open for correction; leaving the field discards invalid text and restores its displayed value.

### Supported values

| Property | Examples |
|---|---|
| Start/end | `6.2.240`, `6.2`, `6`, `+1/16`, `-120 ticks` |
| Duration | `1/4`, `1/16`, `960 ticks`, `+1/16` |
| Pitch | MIDI number such as `60`, or relative `+1 st`, `-12 st` |
| Velocity | `100`, `+5`, `-10` |

Use MIDI numbers for unambiguous absolute pitch entry. Note-name input currently differs by an octave from the displayed names: displayed C4 is MIDI 60, but typing `C4` enters MIDI 72.

## Velocity editor

The velocity lane draws one vertical stem and handle for each visible note. Selected handles keep a white outline.

Use Lasso in this lane, or drag empty space with Pointer, to select notes by their circular handle centres (time and velocity), not by stems or pitch. The selection is shared with the note grid. Shift adds; Ctrl/Command toggles; shrinking and live modifier changes recompute from the original selection. Escape or changing tools cancels and restores it. Explicit Lasso returns to Pointer on release. Outside Lasso, clicking a handle selects its note on release: a normal click replaces the selection, Shift adds, and Ctrl/Command toggles. Clicking does not change velocity. Direct handle dragging retains its previous behavior without changing selection:

- move over a handle to mark its note as hovered;
- drag a handle vertically to change velocity;
- if the hovered note is selected, all selected notes are changed by the same delta from their individual starting velocities;
- otherwise only the hovered note changes;
- values are clamped to `0..127` in the velocity-lane drag path;
- the reference value updates the remembered last velocity.

Dragging the Velocity field in the exact properties bar previews the changed stems and handles immediately in the velocity lane, before the values are committed on release.

The exact properties bar accepts values from `1..127`, while direct velocity-lane dragging can reach zero.

## Navigation and zoom

### Two-dimensional pan

Hold the middle mouse button over the note grid and drag in any direction. The grid follows the pointer horizontally through musical time and vertically through pitches. Both axes move together and stop at beat zero, the timeline end-scroll range, and the available MIDI-key range. Follow Playhead pauses until the gesture ends. The gesture works independently of the selected note tool and does not change notes, clips, selection, pending paste, or undo history.

### Horizontal scroll

Use the bottom horizontal scrollbar to move through musical time.

In the note grid:

- `Shift` + mouse wheel scrolls horizontally;
- `Ctrl` + mouse wheel on Windows/Linux or `Command` + mouse wheel on macOS zooms horizontally around the pointer position;
- plain mouse wheel scrolls vertically through pitches.

Interactive horizontal zoom is limited to a practical range. Zooming keeps the musical position under the pointer anchored unless the view reaches beat zero.

Opening a clip fits it to the current Piano Roll layout. Very long clips may open at a wider view than interactive zoom normally allows so the complete clip remains visible.

Panning keeps the grid, notes, clips, velocity, automation, and playhead aligned. The visible grid remains adaptive regardless of the selected SNAP mode. Fixed snapping can still place notes between visible grid lines.

### Vertical keyboard zoom

Dragging from the keyboard area into the surrounding space changes vertical scale and scroll while keeping the initially clicked pitch anchored.

Vertical scroll and scale are remembered for each track.

## Note name under the pointer

The footer displays the pitch row currently under the pointer while the pointer is inside the note grid. It updates when the pointer crosses into another pitch row and clears when the pointer leaves the grid. Vertical scrolling, zooming, and layout changes recalculate the value even when the pointer itself has not moved. Displayed values are limited to valid MIDI notes `0..127`.

## Keyboard shortcuts

`Command` means `Ctrl` on Windows/Linux and `Command` on macOS.

| Action | Shortcut |
|---|---|
| Pointer / Draw / Range / Eraser / Split / Lasso tool | `Command+1` / `Command+2` / `Command+3` / `Command+4` / `Command+5` / `Command+6` |
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

`Command+D` duplicates the selected MIDI notes by the current selection length. The new notes become selected. Conflicting same-pitch material at the destination is trimmed or removed so the duplicate remains complete.

## Undo behavior

Moves, copies, resizing, splitting, deletion, and property edits are undoable. A provisional paste does not change the project until it is committed.

## Limitations and current behavior

- The MIDI Editor tab requires a selected MIDI clip.
- The active view is track-oriented and may show multiple MIDI clips on that track.
- Some operations use explicitly selected clips, while note selection can cover several MIDI clips on the active track; be mindful of selection context.
- Velocity lane dragging permits zero, while the exact velocity property clamps to one or higher.
- Eraser hover preview is minimal.
- Time Stretch is available in the Song Editor, not in the Piano Roll.
- Piano-key lighting represents routed live MIDI input only and does not encode velocity or arrangement playback.

## Related documents

- [Lower Zone](../ui/lower-zone.md)
- [Getting Started](getting-started.md)

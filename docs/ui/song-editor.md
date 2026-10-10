# Song Editor

- Type: reference
- Audience: users
- Scope: current arrangement controls

## Overview

The Song Editor is the central workspace in the main window. It consists of the Track List (left) and the Arrangement Area (right), sharing a common timeline and playhead.

## Track List

Shows the headers of all tracks arranged vertically.

### Track Header

| Element | Description |
|---|---|
| Color stripe (left) | Shows track color. Click the arrow to minimize/maximize the track. For folder tracks, this shows/hides contained tracks. |
| Track name | Editable by double-clicking. |
| Type icon | Audio (wave), MIDI (piano), or Folder (folder). |
| Arm (A) | Arms the track for recording (audio/MIDI only). |
| Mute (M) | Mutes the track. |
| Solo (S) | Mutes all other tracks. |
| Volume/Pan Knob | Rotary control for volume (click & drag). Right-click opens automation options. Not for folder tracks. |
| Level Meter | Shows current audio level. Not for folder tracks. |

### Context Menu (Right-click on header)

- Delete track.
- Enable/disable input monitoring (if an input is assigned).
- Select/deselect audio/MIDI inputs. MIDI checkmarks represent persistent manual assignments only; the automatic default input used by Exclusive MIDI Focus is not checked. Multiple permanent MIDI inputs can be assigned to a track.

### Track Operations

- **Change track height:** Click and drag the bottom edge of the Track Header (not for minimized or folder tracks).
- **Move tracks:** Click and drag a Track Header up or down. Tracks can be dragged into folder tracks.
- **Add tracks:** Buttons in the toolbar above the Track List for audio, MIDI, and folder tracks. Alternatively, right-click on empty area in the Track List.
- **Minimize/maximize all tracks:** Buttons in the toolbar.

### Folder Track

Allows grouping of tracks. Tracks below are displayed indented. Clicking the arrow in the folder track header shows or hides all contained tracks.

### Automation Lanes

When automation exists for a parameter, a separate header for this automation lane appears below the main header. The height of the automation lane can be changed by dragging the bottom edge.

### Master Track

The last track in the Track List is always the Master Track. It controls overall volume and panning. Has no Mute, Solo, or Record buttons — only Volume/Pan controls and level meters. In the Mixer, the Master Track appears as the last channel strip.

## Arrangement Area

Shows clips and automation data on tracks along the timeline.

### Magnetic mouse snapping

With Off/Fixed/Adaptive settings in the arrangement SNAP control, clip movement/resizing/stretching, Knife previews, range/loop endpoints, automation time movement, and audio-file drag/drop share magnetic positioning. An edited edge locks exactly near a target; keep moving to pull it free and reach any position between targets. The attraction region accounts for display scaling and is capped on dense grids, so intermediate positions remain reachable. The OS pointer is not warped. Shift/Snap Off bypass attraction.

An actively held edge shows a thin target guide and outlined diamond; the SNAP label reports held/free/Shift/off/limit without changing the grid choice. Pulling free or bypassing removes the held guide immediately. Limits and invalid placements are not shown as snap detents. Knife highlights its existing cut line.

Grabbed off-grid edges do not jump at mouse-down. Preview and release use the same resolved position, including positive-duration, source-offset, selected-clip collision, and automation-neighbour limits. Ranges are not hard-quantized again on release. Ctrl still locks automation time, and automation retains its existing live-edit undo behavior.

Keyboard nudging, explicit quantization, numeric property steps, ordinary geometric lasso, MIDI-clip click insertion, and playhead clicks retain their discrete behavior. See [Timeline snapping](../components/timeline-snapping.md) for implementation details.

### Timeline

- Shows bars and beats. Alternating bands use the theme's timeline shadow tint at a subtle editor-controlled intensity, so clips, automation, and lane backgrounds remain visible.
- **Loop lane:** The lower fifth of the timeline is the loop-editing lane. A black overlay at 30% opacity distinguishes it from the rest of the timeline even when the loop range has zero length.
- **Draw loop range:** Hover an unused part of the loop lane to show the pencil cursor and the `draw loop range` hint. Click and drag to create a range. Enabled snapping applies while drawing; hold `Shift` to draw without snapping.
- **Move loop range:** Hover the body of an existing range to show the hand cursor and the `move loop range` hint, then drag it horizontally.
- **Resize loop range:** Hover the first or last 10 pixels to show the corresponding resize cursor and the `set loop range start` or `set loop range end` hint. Drag to resize; hold `Shift` to bypass snapping.
- **Loop state:** The range uses the theme's prime colour at 50% opacity while loop playback is enabled and 20% opacity while it is disabled.
- **Playhead Position:** Double-click on the timeline sets the playhead to the mouse position. Click or drag the playhead to set the playback position.
- **Zoom:** Click and drag vertically in the timeline, or use `Ctrl`/`Command` + mouse wheel, to zoom the horizontal view. Zoom uses nearby discrete scales with evenly rasterized grid subdivisions. The pointer's beat remains anchored unless the view reaches beat zero; small drag movements accumulate rather than being discarded.
- **Grid alignment:** Grid, clips, notes, automation, and playhead use the same horizontal mapping. Panning moves the complete scene together without rounding individual musical positions. The visible grid remains adaptive even with fixed or disabled tool snapping; its display interval no longer depends on the current transport tempo.

### Clips

Rectangular blocks containing audio or MIDI data. Audio clips show a waveform, MIDI clips show a mini preview of notes. Double-clicking a MIDI clip opens the Piano Roll for its track and expands the lower range first if that area is currently collapsed.

Clip fills and normal/selected frames share the same unrounded time edges. Adjoining clips are not independently rounded to pixels; antialiasing remains possible at fractional screen positions, and the existing frame widths and selection colours are retained.

### Toolbar

| Tool | Description |
|---|---|
| Pointer (Arrow) | Select, move, resize. |
| Lasso | Selects individual clips by dragging a frame. Selected clips can be moved vertically between tracks. |
| Range | Selects a time range across multiple tracks. Range can be moved, copied, deleted, or rendered. |
| Time Stretch (Clock) | Stretch or shrink audio clips in time. Drag the right edge — right = longer (slower), left = shorter (faster). |
| Knife | Split clips. |
| Delete (Trash) | Delete selected. Shortcut: `Backspace`, `Delete`, `Cmd/Ctrl+X`. |
| Reverse (Back Arrow) | Reverse audio. Shortcut: `Cmd/Ctrl+B`. |

### Lasso and Range selection

Lasso selects clips by their displayed rectangles, or automation points when started in an automation lane. Normal dragging replaces selection; hold `Shift` to add or `Ctrl/Command` to toggle hits relative to the original selection. Shrinking the frame removes transient hits. Escape/tool changes cancel and restore the previous valid selection. The dedicated Lasso returns to Pointer after release and does not create a TimeRange.

Range independently selects a musical interval over tracks/automation lanes, including empty space. Its existing snapping, move/copy/resize/delete operations remain separate from object selection. It returns to Pointer after completion; Escape restores the previous range/selection.

### Editing (Pointer Tool)

- **Select:** Click on clip/automation point. `Shift`+click for multiple selection. `Ctrl/Cmd`+click to add/remove.
- **Move:** Click & drag. `Ctrl/Cmd`+drag copies. `Shift` disables snap.
- **Resize (Clips):** Drag edges.
- **Time Stretch (Audio):** Drag right edge with Time Stretch Tool. Alternatively `Cmd/Ctrl`+drag on right edge.
- **Automation:** Click on line = new point. Drag point = move. Right-click on point = delete. `Ctrl/Cmd`+drag on segment = change curve shape.

### Editing (Knife Tool)

- The split cursor indicates Knife mode throughout the clip grid, including gaps between clips. Hovering a clip additionally shows a vertical preview line across that clip.
- The preview line follows the arrangement **SNAP** setting. Clicking splits the clip at exactly the previewed position.
- Hold `Shift` while hovering or clicking to bypass enabled snapping for both the preview and the actual split.
- No preview line is drawn over empty track space; the Knife cursor remains visible there.
- Lasso and Range show selection cursors throughout their respective selection area, including empty space. Pointer/Time Stretch retain contextual move/resize feedback over editable objects and the normal pointer elsewhere. Active drags keep the cursor of the ongoing action.

### Clip Properties Bar

Start, End and Duration follow the visible clip while moving, copying, resizing or stretching, before release. Only a subtly different text color is used during dragging; normal color returns on release or cancellation. With several selected clips, `CLIPS (REF):` means the values belong to the grabbed clip and the count still covers the real selection. Outside the gesture, the existing common-value/mixed display returns. Piano Roll clip-overlay gestures update these clip values too, but use the Piano Roll snap status.

When one or more clips are selected, a properties bar appears above the toolbar.

| Field | Description |
|---|---|
| Selected Clips | Number of clips currently selected. |
| Start | Start position in bars.beats.ticks. Editable. |
| End | End position. Editable. |
| Duration | Length as musical note value or bars.beats.ticks. Editable. |
| Snap | Grid mode: Off, Adaptive, or fixed (1/1–1/128). |
| Insert Length | Default length for new MIDI clips: Adaptive or fixed (1/1–1/128). |

**Multi-clip editing:** At rest, shared values are shown normally and differing values as `—`. A field edit derives one shared move/resize delta from the reference clip and applies it to the selection, retaining its relative differences. During a canvas gesture, live values instead follow the clip actually grabbed, identified by `CLIPS (REF):`.

**Drag scrubbing:** Click and drag on any value field to scrub in real time. Non-destructive preview while dragging; committed on mouse release.

### Footer Bar

Shows information about the current grid/snap type.

### Navigation

- **Scroll:** Mouse wheel (vertical), `Shift`+mouse wheel (horizontal). Scrollbars.
- **Zoom:** `Cmd/Ctrl`+mouse wheel (horizontal). Mouse wheel over timeline/keyboard (vertical). Vertical dragging in timeline (horizontal).

## Related documents

- [Header Bar](header-bar.md)
- [Side Browser](side-browser.md)
- [Mixer](mixer.md)
- [Track Chain](track-chain.md)
- [Clip Properties Bar](../components/clip-properties-bar.md)
- [Song Editor implementation](../components/song-editor.md)
- [MIDI Input Routing and Exclusive Focus](../architecture/midi-input-routing.md)
- [Getting Started](../user/getting-started.md)
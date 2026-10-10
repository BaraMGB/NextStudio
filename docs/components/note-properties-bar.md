# NotePropertiesBar

- Type: reference
- Audience: contributors
- Scope: current exact note-property planning, selection and Piano Roll integration

## Purpose and source

`PianoRollEditor` owns this compact MIDI-note inspector between toolbar and timeline. It edits global Start/End, Duration, Pitch and Velocity for selected note/clip pairs; it also presents independent snap and insertion-length controls. User instructions are in [Piano Roll](../user/piano-roll.md#exact-note-properties).

| Area | Source |
|---|---|
| Fields, parsing, plans and presentation | `App/include/NotePropertiesBar.h`, `App/src/NotePropertiesBar.cpp` |
| Provisional numeric edits | `App/include/MidiNotePropertyEdit.h` |
| Defensive selection provider and commit routing | `App/src/PianoRollEditor.cpp` |
| Timing/pitch ghosts and overlap cleanup | `App/src/MidiViewport.cpp` |
| Velocity preview/commit display | `App/src/VelocityEditor.cpp` |
| Shared numeric-field conventions | [Property-field input](property-field-input.md) |

## Fields and creation controls

| Field | Absolute meaning | Relative meaning |
|---|---|---|
| Start | Set each note's global start, keeping its duration | Move each note by the same beat delta |
| End | Give each note the same global endpoint | Add the same duration delta |
| Duration | Give each note the same length | Add the same length delta |
| Pitch | Set MIDI number or parsed name | Add semitone delta |
| Velocity | Set integer value | Add integer delta |

`SELECTED NOTES:` counts valid selected pairs. Idle values are common-value/mixed rather than an arbitrary primary note's values; comparison uses the local `1e-7` epsilon. Absolute End may therefore produce different lengths, while relative edits preserve the corresponding differences.

`SNAP` chooses Off/Fixed/Adaptive for this MIDI timeline. `INSERT LENGHT` (current UI spelling) chooses Adaptive, Last Inserted or a fixed `1/1`–`1/128` default. The settings are edit-local; Last Inserted is the default length mode. Adaptive length is independent of the selected SNAP mode. The [shared creation contract](timeline-snapping.md#note-defaults-and-creation) owns ceiling/default/tick-floor semantics; an insert default is not a draw minimum.

The [shared input reference](property-field-input.md) owns text/focus/Tab/wheel/scrub conventions and position/duration formats. **Pitch-name entry currently differs by an octave from displayed names**; use MIDI numbers for exact absolute input. This is a source limitation, not a newly changed behavior.

## Selection and lifetime

The bar does not discover or own selection. `PianoRollEditor` injects `SelectionProvider`, resolving selected raw note pointers against the viewport's current cached clip sequences. Only still-present `(MidiClip*, MidiNote*)` pairs reach the bar; null pairs are filtered again. Never trust a stale pointer simply because an earlier `SelectedMidiEvents` object retained it.

`refreshFromSelection(discardActiveEdit)` abandons input only for a genuine ordered-selection change when discard is requested. Tool notifications and unrelated delayed model refreshes must not overwrite an active text edit. A changed selection returns fields to read-only and releases focus before displaying new values. `clearSelection()` clears cached pairs, invalid state and enabled values.

Track clearing deselects the old `SelectedMidiEvents` directly rather than calling `unselectAll()`, which would reselect the old track. Selection listeners are removed before replacement/destruction. Clearing selection before deleting notes and filtering NOTE-removal notifications prevent deleted-pointer assertions and obsolete values.

## Coordinates and all-or-nothing planning

Use the [Piano Roll model-coordinate contract](piano-roll-editor.md#clip-content-vs-project-coordinates) to convert clip-content starts/offsets into global displayed beats and back. Keep duration separate from absolute time conversion; do not interpret a duration as an absolute TempoSequence position.

`createEditPlan()` parses once and checks the complete current selection before applying any mutation. A scrub plan captures original complete note state and destination start/length/pitch/velocity; subsequent scrub deltas derive from that base rather than compounding previous previews.

Constraints:

- Start must remain at/after global beat zero and retains duration.
- Absolute End must lie after each affected start; relative End is a duration delta.
- Every resulting duration must be positive. Property validation is not an assertion that all numeric edits share Draw's one-tick floor.
- Absolute pitch must be in `0..127`; relative pitch clamps each result there.
- Absolute/relative velocity clamp to `1..127`. The separate velocity lane permits zero (`0..127`); preserve this documented difference until a product decision changes it.
- Invalid input/any invalid pair rejects the complete operation, not a prefix of the selection.

## Preview and commit

Numeric scrubbing builds `MidiNotePropertyEdit` plans without mutating Tracktion or touching undo. `PianoRollEditor` forwards timing/pitch plans to the viewport and velocity plans to the velocity editor so ghosts and stems agree. Release commits only an effective final change; a return-to-origin/no-op opens no transaction.

For Start, End, Duration and Pitch, the integrated Piano Roll commit path:

1. captures full source states and removes edited source notes;
2. clears grouped clip/pitch destination ranges through `cleanUnderNoteRanges()`;
3. resolves conflicts between planned destinations in selection order;
4. recreates notes from full state copies and selects the results.

Destination-priority overlap handling retains custom properties and prevents same-pitch conflicts. Velocity edits use `setVelocity()` directly and explicitly repaint the lane. All mutations use the edit's UndoManager; do not replace owner-installed commit routing with the component's direct fallback when overlap/selection integration is required.

| Property | Undo transaction |
|---|---|
| Start | `Move MIDI Notes` |
| End / Duration | `Change MIDI Note Duration` |
| Pitch | `Change MIDI Note Pitch` |
| Velocity | `Change MIDI Note Velocity` |

Text/wheel changes apply per effective edit; a scrub has one transaction at release, never one per preview update.

## Canvas interaction integration

`MidiViewport` publishes pointer-free primary `NoteTimingPreview` values through `TimeLineComponent::onNoteInteractionPreview`; `PianoRollEditor` connects them to `setInteractionPreview()`. These values come from Pointer's feasible timing or provisional Draw, not the numeric commit handler. Start/End account for clip offsets and Pitch/Velocity refer to the same reference as the ghost.

During a multi-note canvas gesture, `NOTES (REF):` identifies the grabbed note and retains the real count. Draw adds no dummy note/count and is not labeled as an existing selected reference. Idle mixed values/numeric group semantics return after the gesture.

[Shared feedback](timeline-snapping.md#snap-feedback-and-live-values) owns preview precedence, subtle tint, field-edit completion and cleanup. The bar's `finishActiveEdit()` invokes synchronous completion for writable fields, including already-queued focus loss. Owner commit scopes distinguish intentional note recreation from external deletion; callbacks disconnect during teardown. Snap status belongs to the originating Piano Roll context.

## Refresh, layout and theme

Piano Roll flags coalesce NOTE/model/selection/clip/theme updates. Velocity previews and committed changes repaint the same lane. No model listener may resurrect an ended snapshot or overwrite writable text during an unrelated refresh.

Fields use measured preferred widths and proportional allocation after count, gaps and compact snap/length controls. The last field takes the rounding remainder; narrow widths must not simply starve Pitch/Velocity. Duration allocation includes long tick strings. Exact sizing constants remain in source rather than duplicated as a second layout policy.

Labels, enabled/disabled values and subtle preview tint derive from `ApplicationViewState`; invalid/focus feedback remains distinct. Transparent editor backgrounds preserve the row surface. Separators distinguish count, property fields and controls. Programmatic editor mutations must respect the recursion guard described in the shared input reference.

## Validation and limitations

[Testing](../development/testing.md) distinguishes shared position/length/creation/helper coverage from complete bar integration. There is no dedicated property-bar parser/model-application suite. Raw-pointer filtering remains mandatory for future asynchronous changes. Integer overflow diagnostics, pitch-name mismatch, velocity-zero inconsistency, accessibility and readability at extreme widths remain open limitations.

Future production-code tests should cover duration/pitch parsing, mixed-value planning, offset conversions, rejection of one invalid group member, no-op scrub, recreation/selection/undo and narrow layout. A list of recommended checks is not evidence they have been run.

## Related references

- [Property-field input](property-field-input.md)
- [Piano Roll implementation](piano-roll-editor.md)
- [State and events](../architecture/state-and-events.md)

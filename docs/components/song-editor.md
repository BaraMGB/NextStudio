# Song Editor implementation

- Type: reference
- Audience: contributors
- Scope: current arrangement interaction contracts

## Purpose and boundaries

The arrangement uses Tracktion clips/tracks and the shared edit selection. `SongEditorView` coordinates clip/range gestures, drag/drop and preview rendering. `TrackLaneComponent` handles lane-local clip hits; `AutomationLaneComponent` handles automation. `EditComponent` composes the timeline, arrangement and [clip inspector](clip-properties-bar.md). This reference describes integration contracts, not every command or UI control; see [Song Editor](../ui/song-editor.md) for operation instructions.

## Source map

| Area | Source |
|---|---|
| Composition and inspector wiring | `App/include/EditComponent.h`, `App/src/EditComponent.cpp` |
| Arrangement, time-range overlay and previews | `App/include/SongEditorView.h`, `App/src/SongEditorView.cpp` |
| Clip body/edge/fade hits and lane input | `App/include/TrackLaneComponent.h`, `App/src/TrackLaneComponent.cpp` |
| Automation input | `App/include/AutomationLaneComponent.h`, `App/src/AutomationLaneComponent.cpp` |
| Shared feasibility and ghost timing | `App/include/ClipGestureLimits.h`, `App/src/ClipGestureLimits.cpp` |
| Timeline and shared snap resolver | `App/include/TimeLineComponent.h`, `App/src/TimeLineComponent.cpp`, `App/include/TimelineSnapResolver.h` |

## Geometry, gesture and model boundaries

All lanes use the common [timeline view transform](timeline-view-transform.md). Clip bodies and frames retain floating-point edges; previews project actual time endpoints through the TempoSequence rather than translating a fixed pixel width across tempo changes. View-only navigation does not quantize musical data or create edit undo steps.

The authoritative [snapping contract](timeline-snapping.md) classifies hard/discrete and magnetic mouse consumers, inverse anchors, constraints, replay and feedback. Do not re-snap a feasible preview during release. `ClipGestureLimits` shares group feasibility and preview ranges with commit. A valid primary lane alone cannot authorize a group move; every selected destination must pass the same type/bounds checks as commit.

`SongEditorView::setSelectedTimeRangeRaw()` validates an already resolved mouse range without another rounding pass. The discrete setter remains available for explicitly quantized commands. Ordinary lasso remains geometric rather than magnetic.

Arrangement placement/overwrites retain the [central clip overwrite command](../architecture/clip-overwrite-command.md) and its atomic undo/model safeguards. Automation retains its existing live-mutation lifecycle; clip ghost and live-header previews must not be interpreted as approval for a new automation transaction model.

## Cursor working areas and stationary changes

- Knife uses the split cursor throughout clip-capable lanes, including clip gaps, without requiring an individual clip hit. Non-clip lanes such as Master use the normal pointer. The cut line still needs a valid clip/split hit.
- Lasso/Range retain selection cursors in empty space. Pointer/Time Stretch use contextual body/edge/fade feedback over editable objects and the normal pointer elsewhere.
- Active clip drags retain action cursors rather than having hover policy reapplied.

`TrackLaneComponent::refreshCursor(mods)` obtains the actual current mouse position, recomputes clip/edge/fade hits and reuses the private cursor selector. It skips active clip drags. A previous Range overlay hover cannot stand in for lane hit state after a tool change.

`SongEditorView::setTool()` cancels an interaction when the mode changes, synchronizes toolbar state and refreshes lanes. Master is external to its lane collection and is refreshed directly when it owns the pointer. The time-range overlay shares its position-based cursor update between hover and mode changes. JUCE may keep the previous component as cursor owner until the mouse moves, so `setTool()` resolves the effective new target and supplies its cursor to the stationary previous owner. Pointer's selected-range body and both edges must take precedence over underlying clip/gap cursors where the overlay is hit.

This is a direct cursor assignment/hit-test contract, not a separate policy or gesture layer. Do not introduce fake hover events or alter editing actions, snapping, persistence or undo to refresh a cursor.

## Live values and feedback

`SongEditorView` publishes pointer-free clip timing through the transient edit-local callback connected by `EditComponent`; the inspector uses the same feasible range as the ghost. [Shared feedback](timeline-snapping.md#snap-feedback-and-live-values) owns snap state/rendering and [ClipPropertiesBar](clip-properties-bar.md#live-canvas-values) owns field display/focus handling. A Piano Roll clip-overlay gesture can update arrangement clip values, but its snap signal belongs to the originating Piano Roll timeline.

## Validation and limitations

Use [Testing](../development/testing.md#timeline-cursor-validation) for native cursor checks; a helper returning booleans would not exercise JUCE cursor ownership or OS cursor images. The [#90 historical record](../archive/changes/tool-cursor-working-areas.md) preserves original before/after results and coverage limits, not current setup instructions.

Helper/model coverage does not certify every event dispatch, automation/stretch/overlay combination, platform or monitor transition. Scope each runtime result explicitly. UI behavior and the existing product workflow still require maintainer review.

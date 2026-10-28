# Song Editor implementation

- Type: reference
- Audience: contributors
- Scope: current arrangement interaction contracts

## Purpose and boundaries

The arrangement uses Tracktion clips/tracks and the shared edit selection. `SongEditorView` coordinates clip/range gestures, drag/drop and preview rendering. `TrackLaneComponent` handles lane-local clip hits; `AutomationLaneComponent` handles automation. `EditComponent` composes the timeline, arrangement and [clip inspector](clip-properties-bar.md). This reference describes integration contracts, not every command or UI control; see [Song Editor](../ui/song-editor.md) for operation instructions.

## Source map

| Area | Source |
|---|---|
| Composition, inspector and follow-control wiring | `App/include/EditComponent.h`, `App/src/EditComponent.cpp`, `App/include/TimelineFollowButton.h`, `App/src/TimelineFollowButton.cpp` |
| MIDI clip join validation and command | `App/include/MidiClipJoinCommand.h`, `App/src/MidiClipJoinCommand.cpp`, `App/tests/MidiClipJoinCommandTests.cpp` |
| Track headers, list and insertion policy | `App/include/TrackHeadComponent.h`, `App/src/TrackHeadComponent.cpp`, `App/include/TrackListView.h`, `App/src/TrackListView.cpp`, `App/include/TrackInsertion.h`, `App/src/TrackInsertion.cpp` |
| Shared drag-edge scroll policy | `App/include/DragEdgeScroll.h`, `App/src/DragEdgeScroll.cpp` |
| Middle-button pan geometry | `App/include/PanDragGesture.h`, `App/tests/PanDragGestureTests.cpp` |
| Arrangement, time-range overlay and previews | `App/include/SongEditorView.h`, `App/src/SongEditorView.cpp` |
| Clip body/edge/fade hits, context actions and lane input | `App/include/TrackLaneComponent.h`, `App/src/TrackLaneComponent.cpp` |
| Track-aware Wave drops, cancellation and sampler-pad acceptance | `App/include/DragState.h`, `App/include/MainComponent.h`, `App/src/MainComponent.cpp`, `App/include/DrumPadComponent.h`, `App/src/DrumPadComponent.cpp`, `App/src/DrumSamplerView.cpp` |
| Automation input | `App/include/AutomationLaneComponent.h`, `App/src/AutomationLaneComponent.cpp` |
| Shared feasibility and ghost timing | `App/include/ClipGestureLimits.h`, `App/src/ClipGestureLimits.cpp` |
| Timeline and shared snap resolver | `App/include/TimeLineComponent.h`, `App/src/TimeLineComponent.cpp`, `App/include/TimelineSnapResolver.h` |

## Geometry, gesture and model boundaries

The Song Editor ToolHeader owns a right-aligned `TimelineFollowButton`; the global Header Bar no longer owns timeline following. Its enabled state and Page/Continuous mode live in the `SongEditor` view-data node and are independent from track-derived Piano Roll timeline nodes. Legacy global follow properties migrate once to this Song Editor node.

All lanes use the common [timeline view transform](timeline-view-transform.md). Clip bodies and frames retain floating-point edges; previews project actual time endpoints through the TempoSequence rather than translating a fixed pixel width across tempo changes. View-only navigation does not quantize musical data or create edit undo steps.

The authoritative [snapping contract](timeline-snapping.md) classifies hard/discrete and magnetic mouse consumers, inverse anchors, constraints, replay and feedback. Do not re-snap a feasible preview during release. `ClipGestureLimits` shares group feasibility and preview ranges with commit. A valid primary lane alone cannot authorize a group move; every selected destination must pass the same type/bounds checks as commit.

`SongEditorView::setSelectedTimeRangeRaw()` validates an already resolved mouse range without another rounding pass. The discrete setter remains available for explicitly quantized commands. Ordinary lasso remains geometric rather than magnetic.

Arrangement placement/overwrites retain the [central clip overwrite command](../architecture/clip-overwrite-command.md) and its atomic undo/model safeguards. Automation retains its existing live-mutation lifecycle; clip ghost and live-header previews must not be interpreted as approval for a new automation transaction model.

## Clip context actions, Wave export and track-aware plug-in drops

A plain clip press records pending selection instead of immediately replacing the current Track List selection. A release without dragging applies the normal click/Ctrl-toggle result. Once a drag begins, `TrackLaneComponent` selects the grabbed clip as required by arrangement move/copy helpers while retaining selected tracks; this keeps the currently displayed Track Chain available. Fade gestures remain on their direct path. Drop completion clears pending click state so a pad drop cannot also move the source clip.

Wave body drags start a JUCE drag with the `WaveClip` description in parallel with the arrangement preview. `DragState::targetTrack` is the host-owned routing target and `dropTargetAcceptsDrop` is the current target's acceptance signal. `TrackHeaderComponent` accepts that description, updates the target track, keeps the dragged clip selected during hover and makes itself the sole selected track. If the Lower Zone is showing Mixer or MIDI Editor, the header activates `LowerRangeView::pluginRack` through `EngineHelpers::setLowerRangeTrack()`. Plug-ins do not own global selection.

A sampler pad reports valid-pad enter/move/exit through the shared drag source but does not select tracks. On release, `TrackLaneComponent` confirms that the JUCE `WaveClip` drag is still active, ends the arrangement gesture first and commits the accepted target track selection. This ordering prevents the Wave clip from moving in the arrangement and keeps the destination chain visible before JUCE dispatches `itemDropped()`. `MainComponent::dragOperationEnded()` notifies the source lane when JUCE ends first (Escape or external cancellation); the lane then cancels its parallel arrangement gesture so the later mouse-up cannot commit a move or stale target.

The JUCE clip snapshot is an external-target affordance, not the arrangement preview. `TrackLaneComponent` replaces it with a transparent image while the pointer is inside any arrangement `TrackLaneComponent` and restores it over headers, the Lower Zone and other non-lane components. The normal arrangement ghost remains authoritative inside lanes.

`TrackLaneComponent` provides one body context menu for Wave and MIDI clips while fade-handle right-click remains reserved for fade curves. Rename uses an asynchronous `AlertWindow`, rejects an empty trimmed name, creates one `Rename Clip` undo transaction and explicitly focuses/selects the text editor. Delayed focus retries account for window managers that activate the newly shown window after the first request. Delete acts on the clicked clip rather than the current multi-selection, starts one `Delete Clip` transaction and removes track automation in that clip's range before detaching it.

**Render to New Track** calls `EngineHelpers::renderCliptoNewTrack()` with the clicked clip. The helper validates edit membership, positive range and track membership; constructs the render mask with `BigInteger::setBit(trackIndex)`; and supplies the exact clip list to `Renderer`. The resulting WAV is inserted on a new audio track at the source start while the source remains unchanged. Invalid or failed rendering does not insert a track and is reported by the menu callback.

`EngineHelpers::renderWaveClipToFile()` validates clip membership/range/track and renders only that clip over its edit range. It renders to a non-conflicting temporary WAV beside the requested destination and replaces the destination only after success, preserving an existing file when rendering fails. The Wave-only **Save As...** action uses an asynchronous `FileChooser` retained by the callback and a `SafePointer` to the lane. `ApplicationViewState` keeps the chosen parent directory as transient session state; the first chooser and any missing remembered directory fall back to the configured Samples directory. This value is intentionally not serialized.

Drum-pad drop rendering chooses a non-conflicting final file in the configured Samples directory. Rendering and sampler state changes may run nested callbacks and synchronously rebuild the plug-in view, so `DrumPadGridComponent` clears component-owned hover feedback before those operations, performs sampler mutation through retained engine objects and only revisits the UI through `SafePointer`. Pad selection is applied only after the source file is valid or rendering succeeded; a lifetime-independent view-state callback persists the index before sampler mutation, a surviving view refreshes the Sound Editor afterwards, and a rebuilt view restores the same index. Pad text changes must repaint immediately.

## Joining MIDI clips

`MidiClipJoining` accepts at least two explicitly selected MIDI clips only when they belong to one track and occupy consecutive positions in that track's time-sorted clip list. A temporal gap does not break adjacency, but any unselected intervening clip does. Frozen tracks and mixed or cross-track selections are rejected before mutation. MIDI channel, MPE mode, quantisation, groove, clip volume, mute and bank-change settings must also match because one result clip cannot preserve different per-source playback settings. Rejections are shown as a five-second message bubble in the Song Editor.

The command consolidates events with Tracktion MIDI-list operations: the result spans the earliest start through the latest end, gaps remain silent, visible loop repetitions are expanded, and notes/controllers/SysEx outside each source clip boundary are trimmed. Source offsets are converted from both timeline endpoints so clips trimmed across tempo changes retain the correct visible events and positions. Missing common playback properties are copied explicitly to the new non-looped clip, which replaces the sources and becomes the sole clip selection. Validation, insertion, event copying and source removal are enclosed in one `Join MIDI clips` undo transaction; a failed merge rolls back the current transaction and restores the source selection.

## Track insertion hierarchy

Global add controls and empty-list actions append at the top-level end. Non-Master header actions resolve an explicit `TrackInsertPoint`: **Add Track Below** inserts after the target among its siblings, while **Add Track to Folder** appends to the folder's direct children. The Master header exposes neither insertion action. Folder targets are expanded after successful child creation. Instrument, audio and folder creation share the same placement policy, selection/lower-range update and one undo transaction; nested folders remain supported. Audio and instrument creation stops an active recording before modifying the track model.

Dropping a track on empty Track List space uses the same top-level-end policy. Dropping on a normal header retains the existing before-target behavior, while dropping on a folder appends to its direct children. Placement must use sibling/parent relationships rather than the last item in the recursively flattened track list.

## Middle-button panning

`EditComponent` receives recursive mouse-listener events from the scrollable Arrangement Area and the fixed Master lane, and owns the complete middle-button pan lifetime. It captures the pointer and both view origins at mouse-down, then evaluates every drag from screen coordinates so vertically moving lane components cannot distort the delta. Horizontal pixels are converted with the captured beats-per-pixel scale and vertical pixels use the arrangement scrollbar's native units. Both requested starts are clamped to the existing beat and scrollbar ranges before being applied through `EditViewState::setNewStartAndZoom()` and `setVerticalScrollPosition()`, keeping Track List headers, lanes and scrollbars synchronized.

A middle-button press is rejected when another mouse button owns an edit gesture, and releasing the middle button ends panning even if another button remains held. Lane, automation and selected-range handlers do not start selection or model interactions for middle-button events. While panning, the timeline's transient position follower is suspended without changing its persisted enabled state or mode; release clears any stale Page interpolation before following resumes from the manually chosen view. The gesture displays the hand cursor, restores the prior contextual cursor on release, changes only edit-local view state and creates no undo transaction. `PanDragGesture` uses absolute anchors so clamping cannot accumulate view-state drift; its direction, independent axes, limits, reverse movement and invalid-input handling are covered by `PanDragGestureTests`.

## Track reorder auto-scroll

Once `TrackHeaderComponent` starts a Track drag, it forwards the owning mouse source to `TrackListView`. The list samples the current screen pointer at 60 Hz and feeds its vertical position to the shared `DragEdgeScroll` policy, so moving header bounds never make cached local coordinates stale. The edge velocity ramps inside the top or bottom 40 pixels and remains capped when the pointer is outside the visible list.

`TrackListView` delegates each requested pixel delta to `EditComponent`, which clamps it through the existing vertical scrollbar and synchronously applies the resulting position to `EditViewState`, the Track List and the arrangement lanes. Scrolling is view-only and never mutates track order or creates undo state; the existing drop target alone performs the reorder. Because JUCE updates `DragAndDropTarget` hover state only on pointer movement, each applied timer scroll also hit-tests the newly positioned headers and synchronizes their visual destination feedback. This keeps the highlighted drop target under a stationary pointer accurate without synthesizing mouse events or changing final drop resolution.

The timer stops in the quiet area, at a scroll limit, on release/drop/cancellation, when the source or drag container becomes invalid, and during list teardown. At a scroll limit, the last valid target feedback remains visible until the next pointer event or drag teardown. Leaving the component does not itself stop the timer because outside-pointer scrolling is part of the contract. Track-height resizing and non-Track drags do not participate.

## Object lasso and TimeRange

The [shared selection contract](selection-gestures.md) separates object lasso from range creation. `SongEditorView` projects beat/lane anchors, delegates automation marker hits to their lanes, and applies a snapshot-based replace/add/toggle set through the existing selection manager. Ordinary lasso never creates a hidden TimeRange on release. Range has independent begin/update/end/cancel APIs and retains the existing snapped interval, selected-track/automation and overlay editing behavior. Escape/tool changes restore valid original selection and suppress late release events.

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

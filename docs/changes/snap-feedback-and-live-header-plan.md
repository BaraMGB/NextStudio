# Snap feedback and live gesture values — implementation plan

Status: **implemented and approved by the maintainer for commit/push**.
Approval: “setze das um”, including the font-color-only refinement for live values.
See [implementation and validation](snap-feedback-and-live-header-validation.md).
Baseline: `54bb3a0` (shared magnetic mouse snapping and #77).

## 1. Goals and boundaries

Two related UI improvements:

1. Clearly distinguish an enabled snap grid from an edge that is actually held at
   a snap target.
2. Show the effective provisional Start, End and Duration in the editor property
   header while moving/resizing a clip or note, without writing preview values to
   the Tracktion model.

Use one resolved interaction snapshot for ghost geometry, header values and snap
feedback. Keep the current 18-physical-pixel profiles, interval caps, raw intent,
Shift/Off behavior, constraints, discrete editing and commit/undo policies.
No new persisted preference, project field, cursor warp or audio feedback.
This plan concerns the clip/note property bars, not transport position or loop IN/OUT.

## 2. Current source and root cause

- `TimelineSoftSnap::map()` returns only a position. The plateau/target state is
  not exposed to renderers. `TimelineSnapResolver` and `TimelineMouseGesture`
  retain correct timing, but consumers cannot directly distinguish held, free,
  bypassed and constrained results.
- Arrangement preview timing is available in
  `SongEditorView::updateClipMouseGesture()`, `updateDragGhost()` and `DragState`.
  It is not passed to `ClipPropertiesBar`.
- MIDI Pointer timing is already shared by `PointerTool::previewTiming()`,
  `MidiViewport::drawDraggedNotes()` and the commit path. Draw holds provisional
  timing in `PianoRollDrawGesture`, without a model note.
- `ClipPropertiesBar::refreshFromSelection()` and
  `NotePropertiesBar::refreshFromSelection()` read model objects. Selection/model
  notifications therefore cannot expose a canvas gesture's provisional values.
- Both bars already display their own numeric-scrub previews through private
  `showPlan()` methods. Reuse their formatting/display logic, but do not call an
  edit/commit handler to supply an external canvas preview.
- Existing multi-selection fields show common values or an em dash. That behavior
  must remain unchanged outside canvas gestures and for numeric group editing.

## 3. Recommended visual language

### Actual snap hold

Only while a relevant interaction is active and its effective manipulated edge
really equals a resolved snap target:

- Draw a thin, non-interactive vertical guide at the **actual target beat**.
- Highlight the manipulated start/end edge with a small filled diamond or tick.
  Movement uses Start; right resize and Draw use End; left resize uses Start.
- Show a compact held/free/bypass status in the existing SNAP label area, without
  changing the selected grid value in the combo box.

Suggested labels (localized at implementation): `SNAP · held`, `SNAP · free`,
`SNAP · Shift`, `SNAP · off`, `SNAP · limited` and, where relevant, an invalid
placement indication. The diamond and line style provide non-color cues; color
alone must not convey the state. Derive colors from the current theme and use a
contrasting outline for bright and dark themes. Reserve label space so status
changes do not shift the header layout.

### Guide policy

- One primary guide, not a full-grid flash or highlights on every selected item.
- Use fractional timeline X and the existing grid projection. Keep musical
  coordinates unchanged; do not independently round object/guide geometry.
- Contain the guide within the originating editor's relevant lane/viewport and
  ruler; do not cover the other editor or the property controls.
- Between detents: ordinary ghost, no held guide. Shift/Off: remove the held guide
  immediately, including modifier changes with a stationary pointer.
- A minimum-duration, project-start, source-offset or collision limit is not a
  snap detent. If a constraint changes the candidate away from its target, clear
  the held state and indicate the limit separately. Invalid placements must not
  be presented as a valid held destination. For group moves/copies, validate all
  selected destinations, not only the primary reference's lane.
- Knife uses its existing split line: accent that line and add the marker instead
  of painting a second competing line. Suppress feedback for an invalid split.
- No animation, persistent glow, release-afterglow or cursor replacement in the
  first version. The feedback ends with the interaction.

The target is obtained from the resolver, not the nearest drawn grid column.
Adaptive snap targets and visible grid lines can differ around meter changes.
An identical formatted tick value is also not sufficient evidence of a snap hold.

## 4. Live header behavior

### Single object

- Clip move/copy: display the provisional clip Start, End and Duration.
- Clip resize/stretch: display the actual feasible endpoint and resulting duration.
- Note move/copy/resize: display timing from the same `previewTiming()` result as
  the ghost; synchronize Pitch for vertical note movement. Velocity stays unchanged.
- Draw: show the provisional new note from mouse-down, without inserting a dummy
  note into the model or claiming that it belongs to the selected-note count.
- Update Duration with the same existing formatting as model values. A moved
  seconds-length clip can change its displayed beat duration under variable tempo;
  do not assume every move leaves the displayed Duration unchanged.

Maintainer refinement: provisional values use only a subtly different font
color. Do not add a Preview label, badge, underline or explanation of model versus
preview state. Derive the temporary text color from the theme, retain readable
contrast in dark/light themes, and keep it distinct from validation errors and
snap hold. Restore the normal text color on commit/cancel.
Use the existing bars/beats/ticks and duration formatters; display rounding must
never be fed back into the musical result.

### Multiple objects

During a canvas gesture, show the **object actually grabbed** as the reference,
not the first arbitrary selection item or an undocumented selection envelope.
Identify the reference without a Preview label and keep the real selection
count visible. Use the same subtle temporary font color as for single objects.
The reference remains fixed for the gesture, including copy.
This ensures Start/End remain useful even when selected objects have mixed values.
The snapshot still contains all affected objects for ghost rendering and checking
common feasibility. Outside the gesture, restore existing common-value/em-dash
presentation. Numeric multi-selection edits retain their present group semantics.

### Preview precedence and lifecycle

- Display-only preview takes precedence over model refreshes while its interaction
  is active. Delayed selection/value notifications may not overwrite it.
- A pending text edit follows its existing focus/commit policy synchronously before a canvas
  gesture begins, including fields that already lost focus but still have a queued
  JUCE focus-loss notification. The later notification must be idempotent. Preview updates must not trigger text-change, focus-loss or
  commit callbacks. Do not allow a second property edit to start during a canvas
  preview; keep snap controls available for supported context changes.
- On release, first resolve the final event, then perform the existing commit.
  Clear the preview and read the resulting model/selection only after the operation
  completes, avoiding a frame that falls back to old model values.
- On cancel, tool change, selection invalidation, removed source object, editor
  destruction or project replacement: clear preview and feedback together.
- On a failed/rejected commit: restore model values and do not leave provisional
  data in the header.
- Hover-only Knife feedback must not replace selected-object timing in the header.
  Range/loop/automation/audio-drop feedback likewise must not masquerade as selected
  clip/note property values.
- Piano Roll clip-overlay gestures may publish clip timing to the clip property bar,
  but their snap signal belongs to the originating Piano Roll grid, not the Song
  Editor's separately configured snap combo.

## 5. Technical approach

### A. Typed snap resolution, retaining scalar APIs

Add a detailed result alongside existing scalar resolution, provisionally:

```text
SnapResolution:
  rawBeat
  mappedBeat
  optional targetBeat
  state: disabled | bypassed | free | held | invalid

InteractionFeedback:
  SnapResolution
  effectiveEdgeBeat
  constrained / destinationValid
  manipulatedEdge
  originating editor/view
```

Have the kernel/resolver report whether the raw input lies in a plateau using the
same interval/profile computation as `map()`. Keep existing scalar methods as
wrappers so positioning does not change. Relative gestures report their actual
inverse-anchored raw candidate, not the unanchored absolute pointer beat.
Consumers finalize feedback **after** feasibility constraints. Compare the
candidate target and feasible edge with a small numerical tolerance derived for
beat/time conversion, not a pixel-wide or one-tick tolerance. Bypass remains bypass
when its raw edge happens to lie exactly on a target.

### B. Editor-local immutable display snapshots

Introduce small, transient typed clip/note preview snapshots with numeric timing,
mode, primary reference, count, validity and feedback. A new Draw note has no source
note object. Existing `ClipPropertyEdit`/`MidiNotePropertyEdit` values can be adapted
where suitable; do not create fake model objects or reuse commit handlers for display.

Publish on the JUCE message thread from the gesture's effective timing calculation.
Do not reconstruct timing from ghost rectangles, query it only inside `paint()`,
or add another snapping pass in the header. Validate source handles before reading
model data; copied display values must not dereference removed notes.

`EditComponent` connects arrangement clip previews to `ClipPropertiesBar`.
`PianoRollEditor` connects note previews from `MidiViewport` to `NotePropertiesBar`.
Use explicit clip-preview routing for the Piano Roll clip overlay. Each originating
editor owns its own feedback renderer and cleanup; do not persist the channel in
`Edit.state` or broadcast all gestures as global selection/model changes.

Expose display-only `setInteractionPreview()` / `clearInteractionPreview()` methods
on the bars. Factor their current selection/`showPlan()` formatting into a shared
value-display path so normal selection, numeric scrub and external previews do not
acquire diverging formatting. Keep preview source/ownership explicit; existing scrub
previews must not overwrite or recursively republish a canvas preview.

### C. Update scheduling and cleanup

Update the in-memory snapshot for every processed gesture/modifier/context event.
Repaint only affected header fields/guide regions when their contents change; avoid
layout recalculation or full-editor polling. If delivery is coalesced, retain the
latest snapshot and an interaction generation so a queued callback cannot restore
a preview after commit/cancel or editor replacement. Read unchanged scalar timing
through the existing commit paths; this feature adds no undo transaction.

## 6. Implementation sequence

1. **Snapshot and live headers first.** Implement display-only snapshots, formatting
   reuse and lifecycle/ownership tests. Wire arrangement move/copy/resize/stretch,
   MIDI Pointer move/copy/resize and Draw. Verify preview equals commit and old
   model refreshes cannot overwrite the header.
2. **Detailed snap state.** Extend kernel/resolver/relative-gesture reporting without
   changing mapping. Test plateau classification, bypass and constraint overrides.
3. **Visual feedback for those gestures.** Add the reusable guide/marker renderer
   and compact status. Include clip-overlay numeric routing and correct editor ownership.
4. **Other mouse snap consumers.** Reuse feedback for Knife, ranges, loops, automation
   time movement and browser drop; retain their existing model/undo lifecycles and
   header scope. Do not show a second Knife line or interfere with hit testing.
5. **Documentation, full validation and delivery.** Update component/user/testing
   docs and changelog after implementation. Build with `BUILD_JOBS=12 ./build.sh rd`,
   run `BUILD_JOBS=12 ./test.sh rd`, perform focused native checks, and generate the
   test artifact with `BUILD_JOBS=12 ./build_and_copy_shared.sh`. Obtain visual
   acceptance before commit/push; those operations require a separate instruction.

## 7. Tests and acceptance criteria

Automated production-helper/display tests:

- Detailed resolution maps to exactly the old scalar result over dense sweeps.
- Correct held/free classification at both plateau boundaries, narrow-grid caps,
  fixed/adaptive grids, different profiles, large coordinates and 100/125/150/200% scales.
- Shift/Off at an exact grid beat are not reported as held; stationary modifier
  transitions clear/restore feedback correctly.
- Off-grid grabs, context re-anchors, limits and invalid placements do not produce
  misleading held states. Draw targets the provisional End, not its fixed Start.
- Header previews and committed timing agree for clips and notes, including copy,
  both resize edges, variable tempo/meter, clip offsets and multiple selections.
- Reference selection, provisional Draw count, unchanged/mixed model fields,
  formatting and preview-source precedence are deterministic.
- Model refresh during a preview, failed commit and late queued callbacks cannot
  leave stale fields. Cancel/tool/project/editor/object transitions restore model display.
- Preview-only interaction does not change model state, persistence, selection
  membership or undo history. Numeric text/wheel/scrub behavior stays intact.
- Renderer tests retain fractional geometry; markers/lines do not change layout
  or intercept input. Verify snap cues in dark and light themes, not color alone.
  Header provisional values use only the subtle theme-derived font color, with
  readable contrast and normal-color restoration after commit/cancel.

Focused native acceptance in both editors:

- Start/End/Duration follow the visible ghost throughout a slow gesture, including
  movement outside detents and stationary Shift changes.
- Held signal appears only on the effective manipulated edge at its target;
  it disappears immediately on escape, bypass, a limiting result or interaction end.
- Header values do not flash back to the old model on release. One commit/undo/redo
  preserves the existing behavior; copy shows destination rather than source values.
- Multi-selection clearly identifies the grabbed reference while keeping count;
  Draw displays a preview without falsely increasing the selection count.
- Verify real variable-tempo and offset cases, partially off-screen guides, narrow
  layouts, rapid pointer movements, keyboard-driven context changes and 100/125%
  native application scale. Automated higher-scale tests are not native certification.

## 8. Implementation refinements

The delivered implementation uses small primary-value snapshots and the existing
shared per-selection constraints/timing helpers for the other ghosts, rather than
replacing all drawing/commit paths with a new aggregate edit plan. Clip preview
routing is an explicit transient callback owned by the edit-local view state and
connected/disconnected by the arrangement inspector owner; it is not serialized.
Status is stacked below SNAP inside fixed label space to preserve combo width.

Delivery is synchronous, so no preview-event queue or extra generation counter is
needed. The existing timeline AsyncUpdater replays context changes through safe
component callbacks; clearing feedback also clears the callback. Geometry replay
reads the actual pointer through the new transform and a JUCE movement watcher
observes ancestor translation/peer changes. A transient geometry context revision
and re-anchored Pitch prevent pointer drift and pitch changes from layout alone;
automation retains current values during context replay. A tempo-replay
regression led to deriving the primary edge's actual source time instead of
reinterpreting a saved global beat under a new tempo. Ghosts, live values and held
feedback consequently continue to refer to the same feasible edge.

Validation coverage and pending native/platform cases are documented in the linked
report; approval to implement is not final visual acceptance or permission to commit.

## 9. Maintainer follow-up: clip headers cover ruler feedback

Status: approved and implemented; maintainer visual acceptance pending.
Approval: “ja, ich dachte, es wird ohnehin in einem overlayer oder im paintOverChildren gezeichnet.”
Repository/issue state refreshed for this follow-up.

The Piano Roll creates its timeline before `TimelineOverlayComponent`. The latter
is a later-painted sibling covering the union of ruler and note canvas. Its opaque
clip headers occupy the bottom third of the ruler, including the ruler diamond at
`height - 4`. Before the correction, `TimeLineComponent::paint()` painted ruler feedback before
those headers. The overlay's own feedback draws only for the overlay owner, so it
does not restore note/Draw/Knife-owned ruler cues. The existing isolated-guide raster
tests did not cover this actual component layering.

Implemented minimal correction:

- Extract the existing ruler-feedback rendering from the timeline background pass.
  Keep ordinary arrangement ownership unchanged; explicitly defer the Piano Roll
  ruler pass to `PianoRollEditor::paintOverChildren()` after clip headers and borders.
- Translate/clip to the exact ruler bounds, reuse float X, existing physical-size
  marker and snap state. Do not move components to front or alter hit testing.
- Paint ruler feedback exactly once; clip the overlay's body guide below the ruler
  to avoid duplicate translucent ruler strokes during clip-overlay interactions.
- Ensure feedback updates/clear request the final foreground repaint, including
  stationary Shift and Escape. Do not change profiles, timing or live-value colors.
- Add overlapping-header paint-order/raster regression coverage (held and cleared,
  fractional X and four scales); perform a focused native note/Draw/overlay check
  against headers, then build/test and refresh the shared executable.

The new JUCE sibling/foreground raster regression verifies the shared production
layer selector and ruler helper at four scales/fractional phases; its explicit old
paint-order comparison differs only when held. This is isolated component coverage,
not the complete Piano Roll class. Focused native before/after checks verify the
actual editor route and stationary Shift/cancel. See the validation report for
evidence, the refreshed shared artifact and remaining coverage limits.

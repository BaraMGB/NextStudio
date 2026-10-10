# Soft timeline snapping and Piano Roll draw lengths — implementation plan

- Type: historical record
- Audience: contributors
- Status: implemented; #77 closed in `54bb3a0`
- Current reference: [timeline snapping](../../components/timeline-snapping.md)

The original checkboxes, future-tense instructions and intermediate approval/pending statements below are preserved as history, not open work. Final acceptance is recorded in the companion validation report and closed issue. Local evidence/delivery paths are not current prerequisites. The mandatory workflow now lives in [Contributing](../../development/contributing.md).

## Status and scope

- Issue: [#77](https://github.com/BaraMGB/NextStudio/issues/77).
- Source baseline: `main`, `44f805e` (completed timeline/grid work for #75).
- Status: approved by the maintainer's **“Implementiere das.”** The shared implementation and regressions are present; maintainer testing remains mandatory before completing #77.
- Initial policy: magnetic plateaus of **6 physical pixels on either side** of each snap target, capped to **20% of the adjacent interval**. After maintainer testing found this too weak, the policy was strengthened to **12 physical pixels / 30%**. Subsequent editor-specific feedback retains that accepted MIDI profile and strengthens only the Song Editor to **24 physical pixels / 40%**. The latest explicit request sets **both radii to 18 physical pixels**, keeping MIDI/Song interval caps at **30%/40%**; the original design equations below are retained as history.
- Delivery: a coherent shared mouse-snapping change, with #77 as the first consumer, followed by the other consumers listed below. No automatic commit, push, or issue closure.

This is larger than removing a clamp in `DrawTool`. We will separate deterministic musical quantization from magnetic mouse positioning, and migrate the approved interaction paths explicitly. The shared rendering/view transform introduced for #75 remains unchanged.

## Implementation record

The sections below retain the approved design and source findings against the original baseline, not a claim that those defects remain in current production code. The implementation now consists of `TimelineSoftSnap`, `TimelineSnapResolver`/`TimelineMouseGesture`, `MouseGestureInput`, `PianoRollDrawGesture`, and shared MIDI/clip/automation feasibility helpers. All listed mouse consumers have been migrated; hard/discrete command APIs remain separate. Existing-note Pencil hit resizing has **not** been added: Draw remains note creation with overlap handling, while Pointer resizing uses the new mouse policy.

Off and Shift retain complete raw displacement; changing to a different grid or changing view/raster/musical context re-anchors without replaying the old displacement. Modifier changes update active previews without pointer movement. The adaptive adapter also corrects two verified engine boundary failures locally, without patching Tracktion or changing hard command behavior.

See [implementation contract](../../components/timeline-snapping.md) and [validation record](soft-timeline-snapping-validation.md) for current APIs, test coverage, runtime evidence, and remaining maintainer checks. No automatic commit, push, or issue closure has occurred.

## 1. Verified source problems

### 1.1 The #77 length lock

`App/src/DrawTool.cpp` currently:

1. obtains the selected insert length on mouse-down;
2. converts it to a rounded integer pixel width;
3. clamps every drag endpoint to at least that initial width;
4. enforces the insert length again in beats on mouse-up.

`MidiViewport::paint()` independently repeats the beat conversion, quantization, and minimum-length enforcement. `PianoRollNoteLengthTests::testDrawLengthCannotShrinkBelowSelection()` explicitly expects this lower bound.

Consequences:

- the selected default is incorrectly also the minimum drag length;
- a coarse snap can round the default endpoint back to the start, after which the insert-length clamp creates an off-grid end;
- pixel rounding, snapping, preview, and commit can each influence the duration independently;
- removing only one clamp does not resolve the whole interaction.

### 1.2 Hard snap and mouse positioning are mixed

`TimeLineComponent::getQuantisedBeat()` and `snapTime()` perform fixed or Tracktion adaptive hard rounding. Mouse callers independently choose nearest or downward rounding.

Ordinary hard rounding can advance to another grid point when the raw pointer continues moving. The permanently unovercomeable limit in #77 is specifically the insert-length clamp. Nevertheless, hard rounding makes all intermediate positions inaccessible, so it cannot implement the requested soft interaction.

Additional source findings that must be handled during migration:

- `TimelineOverlayComponent` snaps the mouse position and may snap a derived edge again.
- `SongEditorView::setSelectedTimeRange()` always hard-snaps its input; range update and mouse-up paths can therefore re-quantize an already calculated result.
- `AutomationLaneComponent` uses `TimeUtils::getSnappedTime()`, which obtains an adaptive snap type directly rather than the arrangement timeline's full Off/Fixed/Adaptive policy.
- Pointer previews and commits calculate timing separately. Some code converts a duration through an absolute time-to-beat conversion; variable-tempo and offset clips require endpoint-based checks.
- Clip commit helpers apply valid-position, minimum-length, and multi-selection collision limits. A soft preview must reflect those limits rather than displaying an uncommittable edge.
- Existing automation drags mutate the model during the gesture, unlike the provisional Draw/Pointer previews. This plan does not silently convert the automation editor to a new transaction model.

## 2. Binding behavior

### 2.1 Mouse positioning

- A manipulated note/clip edge, range endpoint, automation point, or knife preview locks exactly to a nearby snap target.
- Continued physical pointer movement pulls it out of the detent without holding a modifier.
- Between targets, the resulting position is continuous and monotonic, not restricted to grid positions.
- Reversing direction retraces the same mapping. There is no event-count accumulation, animation, velocity threshold, or hidden history-dependent lock.
- Large pointer movements may skip targets; slow movement through each detent visibly lands on it.
- The **object or preview** snaps. The OS pointer remains freely movable; no cursor warping or global mouse capture is introduced.
- Snap Off and `Shift` bypass magnetic positioning. A note still cannot have a zero/negative duration.

### 2.2 Operations that remain discrete

Do not change the existing meaning of:

- keyboard nudging and provisional-paste arrow steps;
- explicit note quantization commands;
- exact numeric property edits and their wheel/scrub step sizes;
- menu/command placement that requests exact grid rounding;
- click/double-click note and MIDI-clip insertion defaults;
- playhead positioning by a single click, including ruler clicks.

Knife hovering/clicking is intentionally different from ordinary insertion: it selects a pointer-driven split position and must commit exactly what its preview shows, including between grid points.

No new preference or persistent snap mode is added. Existing Off/Fixed/Adaptive settings remain independent for Song Editor and Piano Roll. No project-state migration is required.

### 2.3 Draw default and drag behavior

For a new note:

1. Determine the clicked clip and pitch, and record the pointer position.
2. Resolve the insert-length mode once for this gesture.
3. If snapping is enabled, hard-align the initial start using the existing downward insertion convention.
4. Compute `requestedEnd = start + insertLength` in beats, not from an integer pixel width.
5. With snapping enabled, choose the first valid snap target **at or after** that requested end. The end must also be strictly after the start and satisfy the one-tick floor.
6. Without snapping, use the selected insert length, subject only to the positive-duration floor.
7. Display this complete initial preview on mouse-down.
8. A real horizontal drag changes the initial end **relatively** by the pointer displacement. It does not reinterpret the original click position as the absolute note end.
9. Soft-snap that candidate endpoint once, then enforce the one-tick floor.
10. Commit the same calculated range shown by the preview.

Examples in 4/4, starting at beat 2:

| SNAP | INSERT LENGTH | Initial end | Initial duration |
|---|---|---|---|
| 1/4 = 1 beat | 1/16 = 0.25 beat | beat 3 | 1 beat |
| 1/16 = 0.25 beat | 1/4 = 1 beat | beat 3 | 1 beat |
| 1/16 = 0.25 beat | Last Inserted = 0.37 beat | beat 2.5 | 0.5 beat |
| Off | 1/16 = 0.25 beat | beat 2.25 | 0.25 beat |

The ceiling rule avoids shortening a click-insert default and guarantees next-point alignment when snap is coarser than insert length. It also specifies how a fractional Last Inserted length from a previous soft drag behaves on the next snapped click.

Dragging left after leaving the initial detent can make the note shorter than **both** the insert length and the snap interval. The note start and pitch remain fixed throughout this draw gesture. Dragging past the start clamps to one tick; it does not reverse the note or relocate its start.

Click, double-click, and pointer-tool insertion use the same default-range resolver, so they do not produce different durations for the same settings. Successful creation remembers the actual committed duration as Last Inserted. Resizing existing notes still does not update Last Inserted.

`DrawTool` currently always creates a note; it does not hit-test an existing note for an in-place resize. This plan retains creation plus existing overlap cleanup. Pointer-tool resizing of existing notes is migrated separately. Because #77 also mentions shortening an existing note with the pencil, acceptance review must explicitly distinguish shortening the in-progress draw from adding direct pencil-resize semantics. A new hit-note editing mode is not silently included in the shared snap migration or claimed as fixed without approval.

## 3. Shared mathematical policy

### 3.1 A pure, stateless kernel

Add proposed files:

- `App/include/TimelineSoftSnap.h`
- `App/src/TimelineSoftSnap.cpp`

The kernel accepts a raw physical-pixel position and the adjacent snap targets in the same coordinate space. It has no Tracktion, mouse-event, ValueTree, or rendering dependency.

For adjacent targets `L < R`:

```text
D  = R - L
rL = min(6 physical pixels, 0.20 * D)
rR = min(6 physical pixels, 0.20 * D)
A  = L + rL
B  = R - rR

F(x) = L                                  if L <= x <= A
F(x) = L + (x - A) * D / (B - A)          if A < x < B
F(x) = R                                  if B <= x <= R
```

Across a target, the plateau also extends into the preceding/following interval. Unequal adjacent interval widths may therefore produce different left and right plateau radii at the same target.

Properties:

- both ends of the free section meet the plateau exactly;
- `F` is continuous and non-decreasing;
- the free section is strictly increasing;
- the 20% cap leaves at least 60% of each interval available for continuous movement;
- free-section gain is at most `1 / 0.6`, avoiding arbitrarily sensitive motion on narrow grids;
- maximum displacement from the raw position is bounded by the plateau radius;
- no midpoint jump, wraparound, overlapping detents, or nearest-target ambiguity.

Use a piecewise-linear curve initially. Position is continuous, but velocity/slope changes at a detent boundary. A smooth cubic curve is not part of this batch: it would need separate input-feel approval and additional slope constraints.

### 3.2 Preserve the grabbed position

Applying `F` directly to an existing off-grid edge can move it as soon as dragging begins. Avoid this by supporting an inverse anchor:

- for an off-grid output position, use the unique inverse of the free section;
- for an exact snap target, choose that target's center as the inverse anchor;
- add the complete pointer displacement to that raw anchor;
- apply `F` to the resulting candidate.

Thus `F(inverseAnchor(originalEdge)) == originalEdge` at zero displacement. Movement begins from the grabbed edge, not from the center of a hit-test tolerance rectangle.

Keep the original musical edge, pointer anchor, and raw request separate from the soft result. Never feed the previous soft result back as the next raw request. **Soft snap is not generally idempotent**, so applying it twice would change free-section positions.

### 3.3 Bypass and context transitions

- Off/Shift uses `originalEdge + completePointerDisplacement` through the unrounded view mapping, without the inverse magnetic correction.
- Returning from Shift to enabled snapping deterministically recomputes from the same gesture origin. A bounded change on switching the modifier is permissible and expected; it must not accumulate offsets or discard pointer displacement.
- Snap settings are read for the active interaction; no old detent stays locked after Snap Off.
- On zoom/raster/layout changes during a drag, re-anchor the gesture at its currently displayed musical edge and current pointer position before evaluating further displacement in the new context. Do not apply the entire old displacement at the new scale.
- A settings/grid change similarly invalidates the old inverse anchor. Re-anchor existing drags; do not recreate a new Draw default range mid-gesture.
- Ordinary pointer hover has no inverse anchor; Knife uses the absolute pointer position through `F`.
- Cancellation, tool changes, track/editor replacement, and destruction discard gesture-only anchors.

The initial draw start remains fixed after mouse-down. Shift toggled later changes endpoint positioning, not the start or the originally chosen click default. Shift held at mouse-down bypasses both initial start and end alignment.

### 3.4 Numeric safety

- Use doubles for beats, requests, target coordinates, and the kernel calculation; convert to float only at the drawing/event boundary.
- Validate finite coordinates and positive interval/scale before dividing.
- Zero-width or unavailable viewport contexts must not generate an invalid model mutation. Invalid target data falls back to a validated raw candidate; invalid raw requests preserve the last valid preview.
- Use a scale-aware target-equality tolerance only to resolve floating-point boundaries, not as a musical quantization step.
- Handle exact target positions explicitly; strict next-target lookup must make progress.
- Do not round beat positions to ticks globally. One tick is a lower duration bound, not a new quantization rule for free motion.

## 4. Grid targets and coordinate adapter

Add an engine-aware resolver, provisionally `TimelineSnapResolver`, separate from the pure curve. `TimeLineComponent` supplies the appropriate settings and current viewport context and exposes distinctly named mouse-positioning methods. Final names follow local code conventions; the API separation is mandatory.

Proposed responsibilities:

```text
existing hard APIs:
  getQuantisedBeat / getQuantisedNoteBeat / snapTime

new explicit APIs:
  adjacentSnapTargets(globalBeat)
  snapBeatForMouse(rawGlobalBeat)
  snapTimeForMouse(rawTime)
  rawAnchorForMouse(displayedGlobalBeat)
  snapEndAtOrAfter(requestedGlobalBeat, minimumGlobalBeat)
```

### Fixed snap

- Targets are integer indices of `4 / denominator` beats in project coordinates.
- Use indexed floor/ceil generation, not repeated additions.
- Derive the strict next target explicitly when the input is already on a grid point.
- Support every existing denominator, including 1/1 and 1/2; do not route those through the fallback level in `getBestSnapType()`.

### Adaptive snap

- Preserve the existing Tracktion snap-type selection for musical snapping.
- Obtain actual lower/upper targets via `TimecodeSnapType::roundTimeDown/Up()` with the edit's TempoSequence.
- Do not approximate adaptive spacing with `getApproxIntervalTime()` at the transport position.
- Test tempo ramps, tempo changes, meter boundaries, and triplet state using actual engine rounding. If adjacent-target lookup at a boundary fails to progress, resolve that before integrating consumers.
- Keep this separate from the visual beat-grid resolver introduced for #75. A musical snap point may lie between visible grid lines.

### Physical pixel conversion

For the current linear beat view:

```text
physicalX = (globalBeat - viewStartBeat) / beatsPerLogicalPixel * rasterScale
```

Use `EditViewState::getTimelineViewport()` and the context already published by `TimeLineComponent`. The raster scale includes platform, application, and component scaling; do not multiply any of them a second time.

Run the curve between the projected targets and convert the result back through the same view transform. Do not interpolate in seconds at variable tempo when the view is linear in beats.

### Clip-relative notes

Use a single documented conversion for all migrated note paths:

```text
globalBeat = clipStartBeat + internalNoteBeat - clipOffsetBeat
internalNoteBeat = globalBeat - clipStartBeat + clipOffsetBeat
```

The current Draw path omits the offset when converting the click to internal note coordinates. Add an offset-clip regression and fix the affected creation/preview conversion coherently, without rewriting unrelated loop/take semantics. Explicitly document any engine loop limitation found by the test.

## 5. Draw state and preview/commit ownership

Introduce a small testable draw-gesture model, provisionally `PianoRollDrawGesture`, alongside `PianoRollNoteLength`. Keep mode resolution separate from interaction state.

Its state records:

- active/inactive and real-horizontal-drag status;
- fixed global start and pitch;
- selected default duration and initialized global end;
- pointer anchor and raw endpoint anchor;
- last valid resolved range and relevant context revision.

`DrawTool` owns this state plus safe clip identity. It converts events to requests and asks the resolver/model for the range; it does not clamp in integer pixels.

Implementation details:

1. Remove `m_intervalX` and the insert-length `jmax` from mouse dragging.
2. Replace default endpoint pixel rounding with beat-domain range initialization.
3. Use float event coordinates and retain the full displacement.
4. Distinguish a click from a horizontal drag using JUCE's drag detection plus actual horizontal movement. Tiny click jitter and vertical-only movement must not replace the click default.
5. Expose one resolved preview range to `MidiViewport::paint()`. Remove its duplicated snapping/minimum calculation.
6. Mouse-up processes the final position/modifiers through the same calculation before committing; never hard-snap afterward.
7. Revalidate the clip is still present before accessing it; a clip removed while drawing cancels safely.
8. Create through `MidiViewport::addNewNote()` so overlap handling, velocity, colour, named undo, selection, and Last Inserted remain integrated.
9. Guard failed insertion before selecting the returned pointer.
10. Reset state on completion and `toolDeactivated()`. Add a narrow active-draw cancellation hook for Escape without intercepting text entry or breaking pending-paste Escape behavior.
11. Cover the existing double-click dispatch (`MidiViewport::mouseDown()` forwards mouse-down and double-click separately) so the gesture cannot commit twice accidentally.

Pointer double-click calls the same click-default range resolver. It must not re-resolve the old raw insert length after a coarse-snap default has already been aligned.

## 6. Consumer migration inventory

No global replacement of every `snapTime()` call. Each call is classified and changed according to its interaction semantics.

| Interaction | Main files | Planned change |
|---|---|---|
| Draw note preview/creation | `DrawTool`, `MidiViewport`, `PianoRollNoteLength` | Default resolver + relative endpoint soft snap + shared range |
| Pointer note move/copy/resize | `PointerTool`, `MidiViewport` | Inverse edge anchor, one soft position, shared planned note states for preview and commit |
| Piano Roll Knife | `KnifeTool` | One pointer-position resolver used for both split preview and click |
| Arrangement Knife | `TrackLaneComponent` | Shared mouse resolver in `getKnifeSplitTime()` for both clip preview and cut |
| Arrangement clip move/copy/resize/stretch | `TrackLaneComponent`, `SongEditorView` | Snap the manipulated edge/start once; keep original snapshot and shared constrained effective delta |
| Piano Roll clip overlay | `TimelineOverlayComponent` | Remove intermediate/double snapping; anchor the selected edge/start and use one effective result |
| Arrangement time-range creation/move/resize | `SongEditorView` and its overlay | Soft pointer endpoints; separate raw range assignment from explicit hard-snapped command assignment |
| Loop draw/move/resize | `TimeLineComponent` | Hard initial creation anchor; soft moving end/edge/start; commit exactly the rendered range |
| Automation point time movement | `AutomationLaneComponent` | Use arrangement resolver instead of adaptive-only `TimeUtils` path; retain value axis, Ctrl time lock, and existing model/undo lifecycle |
| Browser audio drag/drop | `SongEditorView::itemDragMove/itemDropped` | Same soft drop-position resolver in ghost and final drop, with current Shift state |
| Ordinary lasso selection | `LassoTool`, `LassoSelectionTool`, MIDI/arrangement selection | Keep geometric hit-testing semantics; do not attract selection rectangles incidentally |
| Note/clip click insertion | Pointer/Draw and arrangement insertion paths | Retain discrete placement; only the specified note default-end rule changes |
| Playhead clicks, nudge, quantize, property edits | Existing command/click consumers | Retain hard/discrete behavior |

### 6.1 Preview and constraints

For each moving/resizing consumer:

```text
original state + raw pointer request
  -> one magnetic mapping
  -> existing feasibility constraints
  -> one effective planned result
  -> render and commit that same result
```

Constraints are not detents and cannot be overcome: beat zero, clip-source bounds, valid track destinations, positive durations, and selected-clip collision limits remain real limits.

Where `EngineHelpers::moveSelectedClips()`, `resizeSelectedClips()`, or `timeStretchSelectedClips()` currently clamps only at commit, extract just the necessary pure effective-delta calculation and reuse it in previews. Preserve the overwrite command, transaction names, copy modifiers, offsets, audio speed/length behavior, and automation-follows-clip semantics. Do not duplicate constraint rules in two implementations.

For Pointer notes, derive preview and committed states from the same original snapshot. Convert actual absolute start/end times through TempoSequence before deriving new beat positions/durations. Do not call an absolute time-to-beat API on a duration. Preserve the shared-selection time/edge-delta behavior and all non-timing note properties.

For time ranges, adding an unsnapped validated setter is necessary: mouse results must not pass through the current always-snapping `setSelectedTimeRange()` on mouse-up. Discrete callers keep their explicitly hard-snapped path.

Automation point ordering, value clamps, and Ctrl time lock remain intact. This migration is not approval for a general automation undo redesign; record any pre-existing limitation separately and do not claim it as newly validated.

## 7. Automated regression plan

Tests precede integration. Add targets explicitly in `App/CMakeLists.txt`; source registration follows the current source-layout rules.

### 7.1 `TimelineSoftSnapTests` — pure kernel

- Exact plateaus at both sides of every target.
- Boundary values and just-inside/just-outside samples.
- Continuity at all piecewise joins and across repeated intervals.
- Monotonic dense sweeps in both directions.
- Positive free-section slope and bounded displacement/gain.
- Narrow, wide, fractional, unequal, translated, and very long-coordinate intervals.
- Identical output for the same input independent of event count or direction history.
- Large jumps versus many small moves ending at the same raw position.
- Inverse-anchor identity for off-grid and exact-grid starting edges.
- Regression demonstrating that feeding the soft result back into the kernel is not a valid gesture update.
- Invalid/non-finite inputs and zero/negative spacings.

### 7.2 Resolver tests — production grid and view

- Fixed denominators 1, 2, 4, 8, 16, 32, 64, 128.
- Actual adaptive targets with Tracktion tempo/meter/triplet changes.
- Exact-grid input, strict-next lookup, and ceiling-default endpoint alignment.
- Song Editor and Piano Roll settings remain independent.
- Snap Off, Shift, modifier toggles, and settings changes.
- Physical attraction widths at 100%, 125%, 150%, and 200%, including component/application scaling.
- Panned/negative/offscreen requests and large fitted views.
- Zoom/layout/raster context invalidation and gesture re-anchoring.
- Hard snap, nudge, quantize, and view normalization remain unchanged.
- Reading/evaluating a snap result does not mutate view state, musical state, or undo history.

### 7.3 `PianoRollNoteLengthTests` and gesture tests

Replace the test that requires the insert length as a drag minimum. Preserve mode/denominator/fallback coverage and add:

- every combination of snap smaller/equal/larger than insert length;
- Adaptive, Last Inserted, and fixed defaults;
- fractional Last Inserted ceiling behavior;
- default preview on mouse-down and plain click/double-click commit;
- relative left/right movement from the initial aligned end;
- pulling out of a detent and moving through several subsequent detents;
- shortening below both insert and snap length;
- one-tick floor, past-start drags, and reversing after reaching the floor;
- click jitter/vertical-only movement;
- Shift at creation and toggled during drag;
- identical final preview and commit;
- context changes, cancellation, and stale/removed clip handling.

### 7.4 Real gesture/model coverage

Use production handlers or the production extracted operation planner with real Tracktion edits, clips, notes, and UndoManager. Pure arithmetic mocks alone cannot prove commit behavior.

- New note state, actual Last Inserted, exclusive selection, and overlap trim/split/removal.
- No model/undo mutation while provisional Draw/Pointer gestures are active.
- One undo step restores the pre-gesture note sequence; redo recreates the exact fractional result.
- Cancellation leaves musical state, Last Inserted, and undo unchanged.
- Offset clips and variable-tempo positions.
- Multi-note move/copy/resize preserving properties and group relationships.
- Clip/overlay previews and commits respecting the same effective limits.
- Range/loop endpoints are not re-quantized on release.
- Knife preview and actual split have equal positions.
- Automation uses Off/Fixed/Adaptive arrangement settings and preserves Ctrl time lock.
- Browser ghost/drop use the same position resolver.

If linking a GUI production handler into a standalone target is impractical, test its extracted production calculation/model integration and require focused runtime validation of the event dispatch. Document the boundary instead of counting a helper-only test as full UI coverage.

## 8. Implementation batches and approval gates

### Batch 0 — baseline and final policy approval

- [ ] Refresh GitHub issues/milestones, release/changelog state, current `main`, and relevant tests.
- [ ] Capture #77 before-state with coarse snap/fine insert, long/fractional Last Inserted, and attempted leftward shortening.
- [ ] Record equivalent existing gesture behavior in note, clip, range, loop, knife, automation, and browser-drop paths.
- [ ] Confirm this plan's curve/radius, default-end ceiling rule, relative Draw semantics, bypass behavior, and full migration inventory before production code changes. Reconcile #77's existing-note wording with the retained create/overlap path; direct pencil resizing would need an explicit extension to this plan.

### Batch 1 — shared kernel and resolver

- [ ] Add failing/new policy tests.
- [ ] Implement the pure curve and inverse anchor.
- [ ] Implement fixed/adaptive target resolution and physical-coordinate conversion.
- [ ] Expose separate hard and mouse APIs; existing callers initially remain unchanged.
- [ ] Validate complete sweeps and engine boundary cases.

### Batch 2 — #77 Draw and note insertion

- [ ] Add gesture/default/model regressions, including failing-before controls for the old clamp.
- [ ] Implement shared default resolution and provisional draw state.
- [ ] Integrate one preview/commit calculation and safe lifecycle.
- [ ] Align Pointer click insertion with the same note default policy.
- [ ] Update Piano Roll documentation and changelog, then build/test/runtime-check.
- [ ] Produce an intermediate shared artifact for maintainer review of the input feel. Do not close #77 yet.

### Batch 3 — Piano Roll interaction consistency

- [ ] Migrate Pointer note move/copy/resize and MIDI Knife.
- [ ] Migrate Piano Roll clip-overlay movement/resizing.
- [ ] Add production planner/model coverage and focused UI checks.
- [ ] Review coarse/fine raster input feel before expanding the migration.

### Batch 4 — arrangement and remaining mouse consumers

- [ ] Migrate TrackLane clip movement/resize/stretch using shared effective constraints.
- [ ] Separate raw and discrete time-range assignment; migrate range gestures.
- [ ] Migrate loop gestures while leaving ruler pan/zoom and click placement unchanged.
- [ ] Migrate arrangement Knife, automation time movement, and browser drop.
- [ ] Add the listed caller regressions and audit remaining hard-snap call sites. Every remaining call receives a documented discrete/geometric classification.

### Batch 5 — documentation, complete validation, test delivery

- [ ] Reconcile this plan with the implemented API and all acceptance criteria.
- [ ] Run the complete build/test/runtime sequence below.
- [ ] Deliver the final shared binary and gesture recordings.
- [ ] Receive maintainer validation and re-check #77 acceptance criteria.
- [ ] Mark implementation complete only after validation. Commit/push/closure only with explicit instructions.

Each relevant implementation batch follows the mandatory workflow in `Todo.md`: tests, documentation, build, full test run, focused runtime validation, shared artifact, and maintainer testing. Approval for a materially changed curve or expanded semantics must precede that change.

## 9. Documentation and release files

Update alongside implementation, not after testing is finished:

- `docs/components/timeline-snapping.md` (new): hard vs soft APIs, exact curve, target provider, gesture anchors, bypass/context rules, and caller classification.
- `docs/components/piano-roll-editor.md`: draw state, shared preview/commit, default range, offset handling, Pointer/Knife changes.
- `docs/components/note-properties-bar.md`: insert default no longer acts as a drag minimum; fractional Last Inserted behavior.
- `docs/components/clip-properties-bar.md`: arrangement mouse behavior versus discrete snap/step settings.
- `docs/components/timeline-view-transform.md`: reference snapping policy while preserving its rendering-only scope.
- `docs/user/piano-roll.md`: click defaults, coarse-snap alignment, relative drag, magnetic detents, one-tick floor, Shift.
- `docs/ui/song-editor.md` and `docs/user/getting-started.md`: soft clip/range/loop/knife/drop behavior and real constraints.
- `docs/development/testing.md`: new targets and the exact coverage/runtime limitations.
- `docs/README.md`: links to the plan and final component documentation.
- `CHANGELOG.md`: user-visible draw fix and shared magnetic snapping.
- `Todo.md`: actual batch status, artifacts, validation, and approvals.

## 10. Build and runtime validation

From the repository root:

```bash
BUILD_JOBS=12 ./build.sh rd
BUILD_JOBS=12 ./test.sh rd
./build_and_copy_shared.sh
```

Use an isolated debug-shell session and temporary X display/project, not the maintainer's active project/settings. Use the debug shell for deterministic track/clip/note setup, state dumps, save/reload checks, and screenshots; use actual mouse input for gestures it cannot drive directly.

Runtime matrix:

1. Snap 1/4, Insert 1/16: initial end at the next quarter-note snap; slow leftward pull below a quarter and below a sixteenth.
2. Snap 1/16, Insert 1/4: initial quarter duration; slow left/right movement across several detents.
3. Last Inserted fractional duration: next snapped click uses the documented ceiling; Off retains the fractional default.
4. Snap Off and Shift at mouse-down/mid-drag/release: deterministic bypass, no stale snap, fixed start.
5. Preview versus saved musical duration, selection, Last Inserted, undo, redo, and reload.
6. Tool switch/Escape/track switch/clip removal during drawing.
7. Pointer single/multi-note move/copy/resize, offset clips, and variable tempo.
8. Arrangement clips and Piano Roll clip overlays, including real resize/source/collision bounds.
9. Range/loop creation, movement, edges, and reversal; no mouse-up re-quantization.
10. MIDI/arrangement Knife preview versus cut position.
11. Automation Off/Fixed/Adaptive, multi-point timing, value-axis movement, and Ctrl lock.
12. Browser drag ghost and final drop near/away from a detent.
13. Keyboard nudge, quantize, numeric edits, ordinary lasso, ruler pan/zoom, and playhead clicks retain their existing semantics.
14. Slow screen-recorded gestures at 100% and 125% native raster scale; automated geometry covers 150% and 200%. Additional native/platform checks are documented explicitly rather than implied.

Save diagnostics under `/tmp/nextstudio-soft-snap-validation/`. Deliver the binary at `/home/ai/Gemeinsam/NextStudio` plus clearly named recordings/screenshots. Verify built/shared SHA-256 equality. After maintainer feedback, repeat affected tests, full validation where appropriate, and artifact delivery.

## 11. Acceptance checklist

- [ ] Initial note end aligns to the next valid snap point when snap exceeds insert length.
- [ ] Insert length is a click default, never a drag minimum.
- [ ] Relative Draw input can shorten and extend from its initial endpoint.
- [ ] Snap detents are exact and can be exited in both directions without Shift.
- [ ] Continuous between-target positions are available with snapping enabled.
- [ ] Physical detent widths are consistent under display/UI scaling.
- [ ] All approved mouse consumers share the same policy and are not hard-snapped again at commit.
- [ ] Hard/discrete consumers remain unchanged except the explicit note default-end rule.
- [ ] Preview and final musical state agree, including existing feasibility limits.
- [ ] Last Inserted, selection, note properties, overlap handling, undo, and persistence satisfy the listed regressions.
- [ ] Cancelled provisional draws do not change the model or undo history.
- [ ] #75's shared view geometry, grid rendering, zoom normalization, and fit lifecycle remain intact.
- [ ] No new persistent state, cursor warping, or production-code change outside the approved scope.
- [ ] Build, all tests, runtime checks, shared artifact, and maintainer validation are recorded.

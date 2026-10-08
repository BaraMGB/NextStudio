# Timeline snapping

## Ownership

Musical snapping is separate from [timeline view normalization](timeline-view-transform.md).
`TimeLineComponent` selects the Song Editor or Piano Roll settings and publishes a
`TimelineSnapResolver` using the current viewport context and edit TempoSequence.
No snap setting, view normalization, or project format is replaced.

There are deliberately two interfaces:

- **Hard/discrete:** `getQuantisedBeat()`, `getQuantisedNoteBeat()`, `snapTime()`,
  `getNudgeDeltaBeats()`. Commands, keyboard nudging, quantization, numeric property
  edits, and playhead clicks retain their existing exact rounding.
- **Mouse:** `getMouseSnapResolver()`, `snapBeatForMouse()`, `snapTimeForMouse()`.
  These allow every intermediate position while attracting nearby targets.
  `TimelineSnapResolver::startAtOrBefore()` hard-aligns initial mouse creation
  anchors using the same corrected fixed/adaptive targets, without changing
  discrete command APIs.

Fixed targets use indexed multiples of `4 / denominator` global beats, including
1/1 and 1/2. Adaptive targets come from actual Tracktion downward/upward rounding
at the requested edit position, not an approximate duration at transport tempo.

Two engine boundary failures are handled locally by the mouse resolver: bar rounding uses the actual `BarsAndBeats` numerator rather than the meter at the last tempo event, and a meter change inside a bar resets that segment's indexed grid using Tracktion's exact straight/triplet fractions. Meter/triplet changes are explicit shared cell boundaries. This avoids future-valued downward targets and discontinuous cells; the engine and existing hard command APIs are not modified.

## Magnetic curve

`TimelineSoftSnap` is pure and stateless, with an explicit attraction profile.
`TimeLineComponent` selects the profile with the same `usePianoRollSnapSettings`
flag that selects the editor's musical snap settings (not by timeline ID):

| Editor | Maximum radius per side | Adjacent-interval cap | Minimum free travel | Maximum free gain |
|---|---:|---:|---:|---:|
| MIDI / Piano Roll | 18 physical pixels | 30% | 40% | 2.5 |
| Song Editor | 18 physical pixels | 40% | 20% | 5 |

Both editors use an 18-physical-pixel radius at the maintainer's request. The
stronger Song Editor interval cap addresses its often tighter visible grid. Profiles are runtime
context, not a new persisted preference. Every consumer of that timeline uses
the same profile, including the inverse anchor.

For adjacent physical-pixel targets `L < R`, let `D = R - L` and
`r = min(profile.radiusPixels, profile.intervalFraction * D)`:

```text
F(x) = L                                     x <= L + r
F(x) = L + (x - L - r) * D / (D - 2*r)       L + r < x < R - r
F(x) = R                                     x >= R - r
```

Each target therefore has an exact plateau on either side. The cap prevents
plateau overlap and leaves the free travel stated above in every interval.
Position is continuous and non-decreasing; the free segment's gain is bounded
by the selected profile. Slope is piecewise constant, not continuously differentiable. There is no
velocity threshold, hysteresis, animation, or OS cursor warping.

Convert through the viewport's linear **beat** transform, using
`rasterScale / beatsPerPixel` physical pixels per beat. Translate by the lower
target before scaling to retain precision on long timelines. Do not interpolate
between target times in seconds when tempo changes.

The raster scale is the context already published by `TimeLineComponent`; it
includes native, global, and component scale. Do not multiply it again.

## Relative gestures and bypass

`TimelineMouseGesture` keeps displayed position and raw pointer intent separate.
At mouse-down it computes an inverse raw anchor for the grabbed edge:

- exact target: use the target center;
- between targets: invert the free segment.

Thus an existing off-grid edge does not jump on grab. Subsequent requests use
complete floating-point pointer displacement from the gesture anchor. Never add
movement to a previously snapped result: the curve is **not idempotent**.

`Shift` bypass uses the unrounded displayed origin plus raw displacement. Releasing
Shift deterministically returns to the magnetic mapping. A bounded position
change when toggling the modifier is expected; no movement is accumulated or lost.
Snap Off uses the same complete raw displacement, even if the UI clears the fixed
interval. It retains the last active grid signature: restoring that grid recomputes
from the original anchor; choosing a different grid re-anchors at the current edge.
`MouseGestureInput` re-evaluates modifier changes at the unchanged raw pointer
position, preserving floating-point/down coordinates and stylus information.

Changes in attraction profile, scale, raster, view revision, grid resolution, or tempo/meter
revision re-anchor at the last feasible displayed musical edge and current pointer
before accepting more movement. This prevents replaying old pixel displacement
at a new scale. Invalid view/raw data retains the last valid result.

## Consumer classification

| Consumer | Policy |
|---|---|
| Piano Roll Draw endpoint | Relative soft gesture from initialized default end |
| Pointer note move/copy/resize | Relative inverse edge anchor; common constrained time delta |
| MIDI and arrangement Knife | Absolute pointer soft position; same resolver for hover/click |
| TrackLane clip move/copy/resize/stretch | Relative edge/start anchor; shared `ClipGestureLimits` |
| Piano Roll clip overlay | Relative edge/start anchor; no intermediate/double snap |
| Song Editor range creation | Fixed initial hard anchor; soft moving endpoint |
| Range move/resize | Relative edge/start anchor; raw validated range setter |
| Loop creation/move/resize | Hard creation start; soft moving end/edge/start; resolved range cached |
| Automation point time movement | Arrangement resolver; constrained common delta; Ctrl locks time |
| Browser audio ghost/drop | Same absolute soft resolver and current Shift state |
| Ordinary lasso | Geometric hit testing unchanged |
| Mouse click note/clip creation | Corrected downward creation target; note default-end ceiling |
| Playhead clicks, keyboard, quantize, property edits | Existing discrete policy unchanged |

`SongEditorView::setSelectedTimeRangeRaw()` validates already resolved mouse
ranges without rounding again. The discrete setter still rounds explicitly.
Automation retains its existing live mutation and undo lifecycle; there is no
new automation-specific undo system.

Knife preview X remains floating-point through rendering, with the same stroke
width as the grid. Arrangement Knife hover is not throttled: dropping the final
1–2 px pointer movement can otherwise leave a stale preview outside a detent.

## Preview geometry

Existing loop move/resize and loop creation paint the cached `m_newLoopRange`
while the interaction is active, not the old transport range. Release commits
that same resolved range; a nonzero legacy delta is not a rendering condition.

`TimeUtils::timeRangeToX()` projects both actual time endpoints through the edit
TempoSequence and the beat-linear view. Range ghosts and browser audio previews
must not use an average seconds-per-pixel value or retain the original pixel
width when their time range moves across tempo changes/ramps. Track previews
use the same sliced clip position/source offset as the range commit without
mutating the clip. Automation preview points receive the same time delta and
are projected in the destination rectangle, preserving its full origin/width
before clipping the paint.

## Real constraints and model coordinates

Only raw gesture intent is snapped. Feasibility constraints are applied afterward
and shared between preview and commit:

- `ClipGestureLimits`: beat-zero time bound, source offset, positive clip length,
  and collisions among selected clips; time stretching filters non-wave clips.
- `MidiNoteGesture`: internal/edit start bounds, one-tick note duration, shared
  edge/time delta, and endpoint-based conversion through the real TempoSequence.
- `AutomationGestureLimits`: unselected neighbours, edit bounds, and preserved
  selected-point spacing. The lane moves outward points first so Tracktion's
  per-point neighbour limits do not collapse the selection.

These constraints are not magnetic detents and cannot be pulled through.
Invalid track destinations still reject the whole clip command.

For non-looping MIDI sequences:

```text
globalBeat = clipStartBeat + internalBeat - clipOffsetBeat
internalBeat = globalBeat - clipStartBeat + clipOffsetBeat
```

Draw, Pointer timing, Knife, and note quantization use this offset-aware mapping.
Loop/take reinterpretation is not redesigned by this change.

## Note defaults and creation

`PianoRollDrawGesture` stores a fixed global start and one provisional endpoint.
At creation, the selected insert duration is resolved once. Enabled snapping
aligns the start downward and the default end to the first target at or after
`start + duration`, subject to the one-tick floor. This immediately advances to
the next target when snap is coarser than insert length. Fractional Last Inserted
values use the same ceiling rule. Off/Shift at mouse-down retains the raw default.

Dragging changes the initial end relatively; leftward movement can shorten below
both snap and insert length. The only duration floor is `1/960` beat. Click jitter
and vertical-only movement retain the default. Shift toggled later does not move
the fixed start. Preview and mouse-up read the same resolved range.

`MidiNoteCreation` is the production model path extracted from `MidiViewport`:
one `Add MIDI Note` transaction includes overlap cleanup and insertion, preserves
all properties on retained split pieces, and remembers the actual successful
inserted duration. The viewport owns selection and UI notification.

Draw remains a creation tool with overlap cleanup, not a new hit-note resize mode.
Existing-note edge resizing remains Pointer behavior. Escape/tool changes cancel
provisional Draw state; a removed clip cancels safely without dereferencing a
stale note pointer. Double-click dispatch does not insert twice for one gesture.

## Validation

`TimelineSoftSnapTests` covers dense physical-position sweeps and inverse anchors.
`TimelineSnappingTests` exercises the production engine resolver, Draw state,
creation/overlap transaction, persistence, note timing and group limits, clip
limits, automation limits, corrected creation anchors and production time-range
projection at multiple scales/pans across tempo changes/ramps.
`PianoRollNoteLengthTests` covers mode resolution
and the tick-only floor.

GUI event dispatch, selection feedback, previews, modifier keys, and native
scaling require the focused runtime checks recorded in `Todo.md` and the
[implementation plan](../changes/soft-timeline-snapping-plan.md) and
[validation record](../changes/soft-timeline-snapping-validation.md). Engine/helper
coverage is not presented as full GUI coverage.

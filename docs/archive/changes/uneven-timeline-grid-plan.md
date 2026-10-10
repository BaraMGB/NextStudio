# Issue #75 — implementation proposal

- Type: historical record
- Audience: contributors
- Status: implemented; #75 closed in `44f805e`
- Current reference: [timeline view transform](../../components/timeline-view-transform.md)

This is the original design/validation snapshot, not current instructions. Local evidence paths are historical, not durable downloads. Exact raster comparisons describe that tested renderer; later native-renderer test corrections are in `04f47ef` and `4106810` (#91).

Status: implemented and validated by the maintainer, including the clip-frame follow-up and code-review fit hardening. Commit/push explicitly approved; issue #75 closed as completed. See [the technical implementation](../../components/timeline-view-transform.md).

## Objective and constraints

Remove the unequal raster coverage of equal-rank vertical grid lines without
rounding each grid line independently. Keep grid, clips, notes, automation,
selection, loop ranges, and playhead on one linear horizontal view transform.
Do not change musical positions, clip/note lengths, quantization commands, DSP,
or undo history. Zoom normalization is a deliberate change to view behavior,
not a musical quantization operation.

The annotated original issue screenshot is available at
`/home/ai/Gemeinsam/NextStudio-issue75-before.png`. This is evidence from the
issue, not a new capture of the running current build. Before implementation,
capture current-main screenshots and slow pan/zoom sequences with snapped and
unsnapped clips/notes.

## Source findings

- `EditViewState::beatsToX()` and `timeToX()` use the visible beat range as a
  linear transform; view nodes store `viewX` and `beatsPerPixel`.
- Three setters write the scale: `setNewStartAndZoom()`, `setNewBeatRange()`,
  and `setNewTimeRange()`. All scale changes need one normalization entry point.
- Wheel zoom is duplicated in `EditComponent::mouseWheelMove()` and
  `MidiViewport::mouseWheelMove()`; both currently use factors 0.9/1.1.
- `TimeLineComponent::updateViewRange()` combines incremental 3% vertical zoom
  with horizontal panning. Normalizing each increment independently could make
  small gestures stick unless unnormalized zoom intent is accumulated.
- `GUIHelpers::centerMidiEditorToClip()` requests an 80%-width fit.
- Horizontal scrolling, follow-playhead, and `PianoRollEditor::scrollBarMoved()`
  change start position without requesting a new scale.
- `GUIHelpers::drawBarsAndBeatLines()` selects an adaptive visual grid even when
  tool snapping is fixed or disabled. It uses `max(3, getBestSnapType().level)`
  and the existing beat-interval table.
- `EditViewState::getBestSnapType()` depends on visible time duration and tempo
  at the transport position. It is also used by musical snapping and must not
  be changed globally as part of a rendering fix.
- `TimeLineComponent::beatsToX()` / `timeToX()`, the integer overload of
  `MidiViewport::getNoteRect()`, and `TimelineOverlayComponent::timeToX()` already
  truncate floating positions. A shared scale alone does not remove these
  existing differences. Some note/clip draw bounds intentionally add 1 pixel
  at the right edge; this is a drawing convention, not a musical end position.
- The rectangle-based conversion overloads currently accept integer width even
  when callers pass floating-point draw bounds. Fractional preview/slice widths
  must not silently introduce a different scale from the full viewport.
- The application has configurable global UI scaling. Integer logical-pixel
  distances are insufficient at fractional display/UI scales.

## 1. Introduce a small shared geometry policy

Add `App/include/TimelineViewGeometry.h` and
`App/src/TimelineViewGeometry.cpp`, independent of application state and paint
side effects. Use double precision internally. Input consists of requested
beats-per-logical-pixel, start/anchor, viewport width, effective raster scale,
and the existing visual meter/interval policy. Output contains actual
beats-per-pixel, start beat, selected visual interval/level, and pixel spacing.

Extract/reuse the existing grid interval table instead of creating a second
mapping. Enumerate lines using integer indices (`beat = index * interval`) and
classify line hierarchy from those indices where practical. No per-line x
rounding is permitted.

Planned API: a `ViewportContext` containing drawable width and raster scale,
a `ZoomRequest` containing requested scale, anchor beat/x, and nearest-versus-fit
policy, and a `NormalizedView` result. `EditViewState` owns the single apply path;
`TimeLineComponent` supplies UI context/gesture intent. Keep a distinct pan-only
operation so scrollbar/follow requests cannot accidentally enter zoom policy.

Provide a beat-based **visual** interval resolver: choose the finest existing
rendered interval with more than 12 logical pixels of spacing, respecting the
current minimum rendered level 3. This matches current selection for ordinary
constant-tempo bar/beat views, but makes the visual choice independent of the
transport tempo and pan position. Share this resolver between zoom
normalization and grid rendering. Leave `getBestSnapType()` and every musical
snap method unchanged. The removal of visual tempo/transport dependence must
be documented and tested; it is not an accidental implementation detail.

Do not introduce fixed-grid or triplet display changes in this batch. The
current rendered hierarchy remains the scope of #75. Finer fixed snap settings
may place objects between visible lines; their coordinates must remain correct,
but their antialiasing need not match the visible grid's common phase.

## 2. Normalize the scale, not the objects

For requested scale `b`, visual interval `I`, and physical pixels per logical
pixel `s`, the desired physical grid distance is `p = I * s / b`.

An admissible scale is `bActual = I * s / n`, where `n` is a positive integer.
Select the closest admissible scale to the requested value. Evaluate neighboring
integer candidates and the neighboring interval levels, and accept only
candidates whose interval resolver selects the same level after normalization.
Use finite candidate enumeration rather than an unbounded select/round/reselect
loop. Resolve equal-distance choices deterministically toward the wider view.

Reject invalid widths/scales and non-finite input. Respect the existing visible
length limits (0.05 to 100240 beats for user zoom); never divide by zero or write
invalid view state. Constrain candidate selection before committing it. Verify
idempotence, monotonicity, and continuity of candidate choices at level changes.

Example at scale 1: a requested 218.5 px/beat and interval 1/16 beat has
13.65625 px/interval. An admissible result is 14 px/interval, or 224 px/beat,
provided that the visual interval resolver still selects that interval.

All equal-rank grid lines then have the same fractional pixel phase. Panning
can leave them antialiased across two columns, but every equivalent line has
the same coverage pattern; this is not a promise that every line is always
one physical pixel wide or that zoom becomes continuous.

## 3. Preserve anchors and gesture intent

For a zoom anchor at beat `a` and logical x-coordinate `x`, compute the final
start only after normalization: `startActual = a - x * bActual`.
Clamp it at beat zero as today; exact anchor preservation is impossible when
that clamp is reached and must be tested as an explicit boundary case.

Centralize wheel/ruler zoom application through `TimeLineComponent` and the
shared `EditViewState` update path. Cache unnormalized requested zoom per
active gesture so successive small increments eventually cross a raster step.
Do not repeatedly multiply the last normalized scale and discard the residual.
Reset gesture intent after another view request, an external state change, a
viewport-context change, or a new gesture. The residual is runtime-only and
must not become edit state or an undo action. Retain current wheel direction,
modifier handling, platform-specific axes, and zoom factors in this batch.

For ruler drag, normalize the zoom first, preserve the mouse-down beat, then
apply the horizontal drag using the actual new scale. Pure horizontal drag
must not renormalize the zoom.

For clip fitting, retain the requested centre and choose the wider admissible
view where needed so fit content is not cropped. For generic range requests,
use an explicit fit/anchor policy rather than silently preserving a guessed
anchor. Convert time ranges to beats once, then delegate to the same path.
Fit requests prioritize visibility over the interactive upper zoom limit; a
wider raster-aligned scale can exceed it. If the coarsest interval would be
subpixel, retain the exact requested fit as an explicit raster-policy exception.
Passive refresh/restore must preserve these large fitted views.

## 4. Integrate state, layout, and raster context

Route all scale-writing setters through the shared normalization function.
Position-only setters remain position-only; never normalize while painting or
inside coordinate getters. Existing ValueTree notifications should continue to
schedule complete editor/lane updates with a coherent final scale and start.
Audit callbacks for synchronous partial-state reads.

Pass the timeline's drawable width and effective display/UI raster scale from
the owning UI component. Validate the derived scale against the JUCE graphics
context during runtime tests. Reconfigure on initialization/project restore,
monitor/UI-scale changes, and width changes; coalesce repeated layout requests
and skip identical results to avoid resize/listener loops. Preserve the left
visible beat for passive resize/restore and use the explicit anchor for zoom.
Clip-fit requests retain their latest target until the next post-layout owner
context, including previously visited tracks with stale cached widths. Explicit
pan/zoom cancels them. Track-ID changes defer context refresh until after layout,
rather than consuming the pending fit synchronously with old bounds.
Persist only the actual `viewX`/`beatsPerPixel` using the existing property names;
no project schema migration and no edit undo transaction are required.

## 5. Audit object alignment using actual drawing paths

Keep `EditViewState` beat/time conversion linear and unrounded, including
floating-point widths for rectangle-based conversions. Clip/slice/preview
rectangles inherit the viewport transform; never normalize their widths or
zoom independently. Ensure grid
lines, clip/note starts and ends, automation, loop/selection geometry, and
playhead receive the same normalized scale and local origin. Audit normal
painting, drag previews, and hit testing, not only final object rendering.

Separate continuous geometry from APIs requiring integer rectangles. Where
current premature truncation visibly separates an object's time edge from its
grid position, retain float x-coordinates through the drawing path (not a new
independent rounding rule). Likely sites include `TimeLineComponent`,
`MidiViewport` note bounds, and `TimelineOverlayComponent`; determine the exact
minimal replacements from current-main baseline tests. Keep intentional
right-edge draw padding separate from the true end coordinate. Do not change
musical values or expand this into a cosmetic outline redesign.

## 6. Automated regression coverage before production integration

Add `App/tests/TimelineViewGeometryTests.cpp` and
`App/tests/TimelineViewStateTests.cpp`, register them in
`App/CMakeLists.txt`, and include tests for:

- Screenshot-derived scales 206.5, 218.5, 186, and 176.5 px/beat.
- Integer physical spacing, common fractional phase, idempotence, and absence
  of interval-generation drift over long ranges.
- Normalization around every rendered interval threshold; monotonic zoom and
  stable interval selection with no alternating level/scale cycle.
- Slow 3% gestures and smaller accumulated intents: no permanent stuck zoom.
- Mouse anchors, combined ruler pan/zoom, beat-zero clamp, and conservative fit.
- Panning/scrollbar/follow updates leave the actual scale unchanged.
- Invalid/zero widths, min/max zoom, old state restore, resize, and view isolation.
- UI/display scales 1, 1.25, 1.5, and 2, including raster-context changes.
- Actual grid and object coordinate paths: snapped starts/ends, unsnapped
  positions, clip-relative notes, previews, selection/loop bounds, hit testing,
  and velocity/automation/playhead positions. Do not settle for comparing two
  copies of the helper formula.
- Constant/variable tempo and representative meters; no mutation of musical
  snap configuration, note/clip state, or the edit undo history.

Add a JUCE image-based raster test (separate target if needed) showing the
same coverage pattern for same-rank lines at several pan phases and scales.
Grid colors/hierarchy and band opacity must remain unchanged.

## 7. Runtime acceptance and delivery

Capture before/after views on current main and the candidate build with matching
viewport sizes, equivalent content, and requested zooms. Include clips and
notes starting/ending on visible grid lines plus unsnapped examples. Use short
slow-pan/slow-zoom sequences: a static screenshot cannot establish stability.

Acceptance requires uniform coverage within each line class, consistent object
alignment, no isolated grid-line jumping, no stuck/reversing zoom, stable anchors
apart from beat-zero clamping, and correct fit/restore/scroll behavior. Quantized
zoom steps and global antialias phase changes during pan are inherent tradeoffs
and must be assessed visually rather than promised away. If these fail, stop and
revise the design instead of adding independent object/grid rounding.

Update `docs/components/piano-roll-editor.md`, a shared timeline-transform
technical document, `docs/ui/song-editor.md`, the relevant Piano Roll user docs,
`docs/development/testing.md`, `CHANGELOG.md`, and `Todo.md` after implementation.

Validate with `BUILD_JOBS=12 ./build.sh rd` and `BUILD_JOBS=12 ./test.sh rd`,
perform focused debug-shell runtime checks, and deliver
`./build_and_copy_shared.sh` for maintainer testing. No commit/push or issue
closure without the required explicit approval and final validation.

## Approved clip-frame follow-up

Maintainer feedback confirmed improved beat/bar lines but exposed inconsistent
adjoining clip frames. The initial float audit missed `GUIHelpers::drawClip()`:
body/content used float rectangles while both final outlines called
`clipRect.toNearestInt()`. JUCE rounds position and width separately. A captured
fractional join at 481.20 logical pixels therefore had a left frame ending at
482 and the next frame beginning at 481, despite exactly adjoining clips.

The maintainer approved the proposed follow-up with “dann mach”. The final
outline operations now use the shared float rectangle through the small
`ClipFrameDrawing` helper, without changing view normalization, stroke style,
viewport clipping, content paint order, or musical data. The focused
`ClipFrameDrawingTests` failed with the original rounding and pass with the
float implementation. An independent float edge-band union reference covers
960 adjoining-clip cases across origins, widths, paint orders, selection states,
and 100/125/150/200% scaling, with zero channel difference in the tested
scanlines. Integer-coordinate appearance remains pixel-identical.

Complete-application before/after checks use matching musical content, view
geometry, and selections at 100% and 125%, including native desktop captures,
normal/left-selected/right-selected/both-selected pairs and a partly offscreen
first clip. Slow pan/zoom retains the scale for pan, accumulates zoom changes,
returns to the starting view after reversing zoom, and preserves clip/note
values. Detailed artifacts are under `/tmp/nextstudio-adjacent-validation/`.
All 27 tests pass. Fractional antialiasing and the existing two inside frame
strokes at an adjoining boundary remain intentional; a single-seam redesign
was not introduced. The maintainer subsequently validated the final build as fixed.

## Code-review fit hardening

Review exposed two missed boundaries: cached viewport widths on previously
visited tracks, and conservative-fit candidate exhaustion near the interactive
upper zoom limit. Every clip fit now retains its latest target until the next
post-layout owner context; changing timeline IDs only schedules that refresh.
Fits can exceed the interactive upper limit, and passive refresh/restore
preserves their wider scale. Beyond the interval table's physical-pixel
resolution, exact content fitting takes precedence over uniform raster spacing.

New regressions failed before the fixes and pass afterward. Both normalization
policies now have full-range sweeps; state tests additionally exercise reopening,
latest-fit replacement, cancellation, settled-fit resizing, large fits/restore,
and unchanged musical state/undo. All 27 tests and the debug-shell editing smoke
test pass. Isolated-X runtime checks reopen a visited track after reducing the
Piano Roll from 1360 to 760 px, for ordinary and 100000-beat clips. The prior
shared build leaves the large clip fit unchanged at 0.1 beats/pixel; the corrected
build fits it completely without changing clip/note values. Runtime artifacts
are under `/tmp/nextstudio-review-fit-validation/` and
`/tmp/nextstudio-review-large-fit-validation/`. The shared test binary has been
refreshed. The maintainer confirmed “Issue fixed” and explicitly approved
committing and pushing the complete #75 change set as one commit. Issue #75
has been closed as completed.

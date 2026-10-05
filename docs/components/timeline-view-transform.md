# Shared timeline view transform

## Ownership

`EditViewState` owns each timeline's persisted `viewX` (start beat) and
`beatsPerPixel`. `TimeLineComponent` supplies the current drawable width and
physical-pixel scale, owns wheel/ruler gesture intent, and observes layout,
view-state, meter, and native-scale changes. Updates are coalesced with an
`AsyncUpdater`; JUCE's `NativeScaleFactorNotifier` handles peer changes and
listener teardown.

`TimelineViewGeometry` is the shared, application-independent policy for zoom
normalization, visual interval selection, coordinate conversion, and indexed
grid generation. Painting never mutates view state.

## Normalized zoom

For interval `I`, raster scale `s`, and requested beats-per-logical-pixel `b`,
choose an integer physical spacing `n` and use `bActual = I * s / n`.
Candidate selection considers every supported visual level and neighboring
integer spacings and rejects candidates whose resulting visual level differs.
Interactive zoom enforces the existing view-length limits (0.05–100240 beats).
The nearest logarithmic scale wins; equal-distance ties favour the wider view.
Fit requests choose a scale no tighter than the requested fit, with no upper
view-length clamp: the next wider raster-aligned scale can exceed 100240 beats.
Passive context refresh/restore preserves these larger fitted views; the next
interactive zoom returns to the bounded policy. If even the coarsest supported
interval would have subpixel physical spacing, no integer spacing can preserve
the fit. Only in that extreme case, retain the requested linear scale rather
than ignoring the fit; uniform raster coverage is not guaranteed there.

The visual resolver uses the existing beat-interval table, minimum rendered
level 3, and more than 12 logical pixels per interval. This makes the visual
grid and bands independent of transport position and tempo. Musical
`EditViewState::getBestSnapType()`, fixed/off snapping, and Tracktion
quantization are not replaced. Finer fixed-snap objects can lie between visible
lines; this change does not add a fixed-grid/triplet display mode.

Equal-rank lines share a fractional physical-pixel phase. They can all be
antialiased across two columns while panning, but their coverage no longer
varies from line to line. Zoom is deliberately discrete. It is not valid to
claim perfectly continuous zoom or always-one-pixel-wide strokes.

## Requests and anchors

- `applyTimelineZoom()` applies normalized scale and start together via the
  existing view properties, without an edit undo transaction.
- `setNewStartAndZoom()` without a scale changes only position. Scrollbars and
  follow-playhead retain the actual scale.
- `setNewBeatRange()` / `setNewTimeRange()` conservatively fit the requested
  range, using its centre as anchor.
- `fitTimelineToClip()` reserves 80% of the target timeline's width. Every
  request retains a runtime-only pending fit until the owner supplies its next
  post-layout width/raster scale. This also covers reopened, previously visited
  tracks whose cached context is stale, not just an editor's first opening.
  Multiple requests before layout retain the latest target; explicit pan/zoom
  cancels the pending fit. Track-ID changes defer context refresh to the
  `AsyncUpdater`, so old bounds cannot consume the fit before the new layout.
- Once a pending fit is settled, passive resize, restore, and raster-context
  changes preserve the left visible beat and normalize the scale.

After scale normalization, `start = anchorBeat - anchorX * bActual`. The
existing beat-zero clamp takes precedence when preserving the anchor would
require a negative start.

Wheel gestures retain unnormalized intent for successive events (350 ms
inactivity starts a new gesture). Ruler drags retain it until mouse-up. External
view/context changes invalidate residual intent. Horizontal ruler movement is
applied using the actual new scale; horizontal movement alone does not quantize
positions or zoom.

Only actual start/scale are persisted. Context, revisions, pending fit, and
gesture residuals are runtime-only. Legacy projects need no schema migration.
Invalid/non-finite requests and zero-width contexts do not overwrite valid
view state; redundant normalization produces no ValueTree notification loop.

## Coordinates and consumers

Beat/time mapping remains linear and unrounded. Rectangle-based conversions
accept fractional widths, so slice/drag-preview bounds inherit the viewport's
scale. Grid lines are generated with integer interval indices, with
`beat = index * interval`, not accumulated additions.

Ruler conversions, note bounds, timeline clip overlays, playhead position, and
lasso painting retain float x-coordinates instead of truncating them before
painting. Inverse timeline conversions accept floats as well. Mouse hit tests
use the same geometry. Existing note/overlay right-edge padding remains a
drawing convention and is separate from musical end positions. Note/clip
values, lengths, automation, DSP, and edit undo history are unchanged.

`GUIHelpers::drawClip()` delegates the final normal/selected outlines to
`ClipFrameDrawing::draw()`, passing the same float rectangle as the clip body
and content. Do not call `toNearestInt()` on this rectangle: JUCE rounds x and
width independently, which can make a frame end differ by one logical pixel
from the next clip's frame start despite exactly adjoining musical positions.
Normal/selected colours, inside stroke widths (1/2 logical pixels), viewport
clipping, and content-before-frame paint order are retained. Adjoining clips
still have two inside frame strokes; this is not a single-seam style redesign.
Fractional origins/scales retain antialiasing, not guaranteed sharp pixel edges.

See [Piano Roll Editor](piano-roll-editor.md),
[Song Editor](../ui/song-editor.md), and
[Testing](../development/testing.md).

## Diagnostics and validation

Agent state dumps include `edit.timelines` with timeline ID, start beat,
beats-per-pixel, width, raster scale, and visual level. Inactive timelines may
retain their last layout context until reactivated.

`TimelineViewGeometryTests` covers geometry, threshold sweeps, accumulated
intent, fractional scales, and JUCE raster coverage. Both nearest and fit
policies are swept through the interactive zoom range; fit tests also cover
larger ranges and the extreme subpixel fallback. `TimelineViewStateTests`
exercises production `EditViewState` setters/conversions with real Tracktion
clips and notes, first/reopened/latest/cancelled fits, large-range fit/restore,
independent views, variable tempo, state preservation, and undo isolation. `ClipFrameDrawingTests` uses the production
outline helper and an independent float edge-band union reference. It covers
adjoining fractional widths/origins, partly offscreen clips, both paint orders,
all four pair selection states, and 100/125/150/200% scaling. A legacy-rounding
control exposes the regression; integer-coordinate appearance remains exactly
unchanged.

Runtime validation additionally checks the complete drawing/input paths and
slow zoom/pan sequences. At fractional UI scaling, capture the physical desktop
for raster analysis: the agent component snapshot is a logical-resolution
image, not necessarily the native display raster. Native monitor transitions
on other platforms still require platform testing.

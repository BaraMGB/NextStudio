# Separate Lasso and TimeRange selection

- Type: proposal
- Audience: contributors and maintainer
- Status: approved architecture; foundation and velocity source implemented, maintainer testing pending
- Related issue: [#84](https://github.com/BaraMGB/NextStudio/issues/84)
- Scope: replace the combined Lasso/Range mechanism before adding velocity-lane selection

## Decision

The maintainer approved retaining an application-owned, musically anchored lasso
and separating TimeRange into an independent gesture. This supersedes the earlier
local velocity-lane proposal. Do not use JUCE `LassoComponent`: its private pixel
anchor would require restart/rebase machinery and a second working selection set.
Reuse the architectural separation, not JUCE's implementation.

Lasso selects objects by their displayed geometry. TimeRange defines an interval
and lanes independently of object hits, including empty timeline space. They do
not share gesture state or an `isRangeTool` switch. Coordinate mapping and visual
primitives may be shared. Tracktion remains authoritative for object selection.

## Architecture

- `SelectionGestures.h`: separate `LassoGesture` and `TimeRangeGesture` value types,
  with a shared, snapshot-based replace/add/toggle selection combiner.
- `LassoSelectionComponent`: projection/rectangle drawing only, no model mutation,
  TimeRange flags, snapping or hidden selection ownership.
- MIDI source: beat/pitch anchors; live note rectangles provide hits;
  `MidiSelectionSnapshot` retains ValueTree identity, resolves current notes and
  applies the whole set through Tracktion's batch selection API.
- Arrangement source: beat and normalized track/lane coordinates; clips and
  automation provide their own hits. Automation snapshots identify curve-child
  states rather than retaining disposable selection proxies or point indices.
- TimeRange: dedicated begin/update/end/cancel APIs. Arrangement retains magnetic
  snapping and selected-track/automation semantics. MIDI retains its time/pitch
  interval selection and persistent Range mode, rather than inventing arrangement
  slice operations inside the MIDI editor.
- Context refresh reprojects the original musical anchor and re-evaluates the
  endpoint under the stationary pointer after view changes. Rebuilding/reordering
  the source view discards its active gesture before destroying children.

## Behavior corrections approved with the separation

- Normal lasso replaces object selection; Shift adds; Ctrl/Command toggles. Shift
  takes precedence. Modifier changes recompute from the gesture-start snapshot,
  and shrinking removes transient hits. Alt does not change lasso selection.
- Pointer empty-space selection retains the original mouse-down anchor and its
  current strategy; it does not replace/delete its tool inside `mouseDrag()`.
- Escape and tool changes cancel selection gestures and restore valid original
  membership. Late drag/release events do not reapply or clear the restored set.
- Dedicated MIDI/arrangement Lasso remains one-shot. MIDI Range stays active;
  arrangement Range returns to Pointer after completion as before.
- An ordinary arrangement lasso no longer creates a hidden TimeRange on release.
- MIDI TimeRange has its own translucent preview instead of a suppressed lasso.
- Selection-only changes do not mutate musical state, persistence or undo.

## Batches and acceptance

### 1. Foundation and existing users

Migrate existing MIDI, arrangement and automation consumers; remove the combined
`LassoSelectionTool`. Add production geometry/policy/model regression tests and a
maintained native routing procedure. Validate original anchors, reverse/shrinking
rectangles, modifiers, cancellation/late events, view changes, selection identity,
Range behavior and unchanged musical state/undo. Preserve cursor and snapping
contracts with the existing regressions.

Build/test with `BUILD_JOBS=12 ./build.sh rd` and `BUILD_JOBS=12 ./test.sh rd`.
Document scope and untested platforms/scales in the issue, produce the shared
artifact with `BUILD_JOBS=12 ./build_and_copy_shared.sh`, and obtain maintainer
validation before treating this batch as accepted.

Maintainer feedback reports clip and TimeRange move/copy snapping back. Normal edit completion is now separate from selection cancellation; canceled releases are scoped to their original mouse-down, and new input ends any pending old selection before editing. These lifecycle corrections preserve the original overwrite policy. The unrelated legacy-overlap policy proposal is withdrawn from this feature; it was not an established diagnosis of the maintainer's regression.

### 2. Velocity source (#84)

Following the maintainer's request to proceed with #84, the velocity lane is now
attached through the viewport's `LassoSource` adapter using the same gesture and
selection policies. Its hit test uses marker centres,
not stems, pitch or duration. Add persistent selected-marker feedback; keep direct
velocity dragging intact. Add source-specific regressions and repeat the
build/test/runtime/shared-artifact gates. #84 remains open until its own acceptance
criteria and maintainer testing are satisfied.

## Evidence and limits

The current [selection contract](../components/selection-gestures.md) owns implemented responsibilities and lifecycle. [Testing](../development/testing.md#selection-gesture-validation) owns reusable test procedures. Noteworthy
results and baseline #84 reproduction are recorded once in the issue. A passing
helper suite does not prove GUI dispatch or native-platform coverage. No commits,
pushes, issue closure or platform certification are implied by this approval.

# Piano Roll Splitter Resize Fix

## Summary

This change fixes a lower-range splitter regression in the MIDI Editor:

- enlarging the Piano Roll and releasing the mouse no longer traps the next drag in collapse-only behavior;
- the Piano Roll can now be shrunk continuously again;
- collapse still uses the existing snap controller and remains reversible within the same drag.

The implementation now mirrors the sidebar's split behavior more closely: resize first, then collapse only after the remaining transition distance is crossed.

## Bug

Previous behavior:

1. open **MIDI Editor**;
2. drag the lower-range splitter upward to enlarge the Piano Roll;
3. release the mouse;
4. drag the splitter downward again.

Result before this fix:

- the Piano Roll did not shrink on the new drag;
- the panel stayed at its enlarged height until the collapse threshold was crossed;
- from the user's perspective the next downward drag effectively only collapsed the lower range.

## Root cause

`LowerRangeComponent::handleSplitterDrag()` only applied Piano Roll height changes for upward drags:

```cpp
if (m_evs.getLowerRangeView() == LowerRangeView::midiEditor && dragDistance < 0)
```

That meant:

- negative drag distances enlarged the Piano Roll;
- positive drag distances never reduced `m_midiEditorHeight`;
- the shared `SplitterCollapseController` still evaluated collapse transitions, so the next downward drag could collapse without any intermediate resize feedback.

The sidebar does not have this problem because it uses a dedicated layout policy with continuous resize clamping before collapse.

## Implementation

## New helper: `App/include/LowerRangeLayout.h`

A new layout-policy helper centralizes the lower-range sizing rules:

- `collapsedHeight = 38`
- `defaultExpandedHeight = 350`
- `getPreferredExpandedHeight()` clamps invalid stored heights back to the standard expanded height
- `getMinimumEditorContainerHeight()` defines the minimum editor area that must remain visible above the lower range
- `getMaximumExpandedHeight()` caps Piano Roll growth at the bottom edge of the Song Editor timeline
- `clampExpandedHeight()` normalizes stored or requested heights into the allowed range
- `getTransitionDistance()` computes the collapse/open travel for the shared snap controller
- `getResizedHeight()` grows upward freely until the timeline boundary and clamps downward resizing at the standard expanded height

This makes the lower-range behavior explicit and unit-testable, similar to `SidebarLayout.h`.

## `App/include/LowerRangeComponent.h`

`LowerRangeComponent` now takes its public `collapsedHeight` and `defaultExpandedHeight` constants from `LowerRangeLayout` so the UI component and the tests use the same source of truth.

## `App/src/LowerRangeComponent.cpp`

### Mouse-down behavior

`handleSplitterMouseDown()` now:

- normalizes the remembered Piano Roll height through `LowerRangeLayout::getPreferredExpandedHeight()`;
- computes the snap-controller transition through `LowerRangeLayout::getTransitionDistance()`.

### Drag behavior

`handleSplitterDrag()` now resizes the Piano Roll for both drag directions whenever the MIDI Editor is the active lower-range view and the panel is still expanded.

New behavior during an expanded MIDI Editor drag:

- drag upward: height grows above 350 as before;
- drag downward: height shrinks continuously;
- once 350 is reached, further downward drag no longer reduces height;
- only the remaining transition distance can collapse the lower range.

The existing Y-scroll compensation is preserved for both directions so the visible note area remains stable relative to splitter movement.

### Maximum-height cap

The Piano Roll can no longer be enlarged beyond the bottom edge of the Song Editor timeline.

Implementation details:

- `LowerRangeComponent` derives the current maximum from the parent `MainComponent` height and the Song Editor timeline height stored in `EditViewState`;
- `MainComponent::resized()` clamps the rendered lower-range height to the same maximum so startup and window-resize layout stay consistent;
- upward drag distance and scroll compensation now stop increasing once that maximum height is reached.

### Follow-up correction

A second edge case appeared after the first fix: once the Piano Roll had shrunk back to its minimum expanded height, continuing to drag downward through the collapse-resistance zone still scrolled the keyboard and note grid.

Cause:

- the height was clamped at 350 pixels;
- but the scroll compensation still used the full raw drag distance.

Fix:

- `LowerRangeLayout` now exposes `getAppliedDragDistance()`;
- `LowerRangeComponent` uses that clamped resize distance for Y-scroll compensation;
- after the Piano Roll reaches its minimum expanded height, further drag in the resistance zone no longer scrolls the view before collapse.

## Tests

`App/tests/SplitterCollapseControllerTests.cpp` now covers the new lower-range layout policy.

Added checks:

- invalid stored Piano Roll heights normalize to 350;
- valid enlarged heights are preserved;
- the Song Editor timeline boundary produces the expected maximum Piano Roll height;
- upward drags continue to grow the Piano Roll until that maximum is reached;
- downward drags shrink it continuously after enlargement;
- shrinking clamps at 350 before collapse;
- applied resize drag stops increasing once the minimum expanded height is reached;
- applied upward drag also stops increasing once the maximum expanded height is reached;
- an enlarged Piano Roll stays expanded throughout the resize phase and only collapses after the full transition distance is crossed.

## Documentation updates

Updated:

- `docs/architecture/overview.md`
- `docs/user/getting-started.md`
- `docs/user/piano-roll.md`
- `docs/development/source-layout.md`

These documents now describe that the Piano Roll splitter:

- enlarges upward freely;
- shrinks downward continuously back to the standard expanded height;
- enters the collapse snap zone only after that resize phase.

## Validation

Validated with:

```bash
BUILD_JOBS=12 ./build.sh rd
cd autobuild/RelWithDebInfo && ctest --output-on-failure -R SplitterCollapseController
```

# Lower-range layout and editor activation

- Type: reference
- Audience: contributors
- Scope: current collapsed lower zone, Piano Roll sizing and arrangement activation

## Ownership and sources

`MainComponent` composes the lower zone and clamps its rendered height. `LowerRangeComponent` owns/reuses its Piano Roll and plugin-chain views and synchronizes the active lower-range track from selection or the stored track marker. Application state owns collapsed status; edit-local view state retains MIDI editor height/view/vertical scroll. View/layout changes do not create musical undo steps.

Sources: `App/include/LowerRangeLayout.h`, `App/include/LowerRangeComponent.h`, `App/src/LowerRangeComponent.cpp`, `App/src/MainComponent.cpp`, and shared activation helpers in `App/include/Utilities.h` / `App/src/Utilities.cpp`.

## Resize before collapse

`LowerRangeLayout` is the single pure sizing policy, including collapsed/default heights, available editor-container minimum, maximum expansion, resize clamp, applied distance and collapse/open transition. The current collapsed/default values are 38/350 logical pixels; callers and tests use the helper instead of duplicating constants.

Mouse-down captures the normalized, maximum-clamped MIDI height and pitch scroll, then initializes `SplitterCollapseController` for that starting expanded/collapsed state. In an already expanded MIDI Editor:

- upward drag enlarges continuously until the boundary below the Song Editor timeline;
- downward drag shrinks continuously back to the standard expanded height;
- only the remaining collapse-transition travel can then collapse the lower range;
- the controller retains reversible open/collapse behavior within the same drag.

Pitch-scroll compensation uses **applied resize distance**, not raw drag distance. Once minimum/maximum height clamps, the keyboard/grid stop drifting through the resistance zone. Both directions compensate using the original pitch and note scale. `MainComponent::resized()` uses the same maximum policy for startup and window changes; stale stored height cannot extend the panel past available layout.

A drag begun collapsed follows its opening controller semantics rather than the already-expanded continuous-resize path. Other lower-range views retain their existing sizing behavior; do not infer MIDI-specific resize from the Plugins tab.

## Arrangement activation

`TrackLaneComponent` evaluates `EngineHelpers::shouldOpenMidiEditorForArrangementClip()` with clip type, double-click state and whether the MIDI editor is already active. `openMidiEditorForTrack()` selects the track/view and optionally expands the lower range; double-clicking a MIDI clip requests expansion and centering even if collapsed. Prior single-click activation policy is retained rather than opening arbitrary clips into the Piano Roll.

Collapsed view buttons use `EngineHelpers::openLowerRangeView()` to select/expand through the same path. No one-off activation layer is needed. [Timeline fit ownership](timeline-view-transform.md#requests-and-anchors) handles clip centering with the actual post-layout viewport, not a width remembered before reopening.

Track teardown and child ownership belong in [Piano Roll](piano-roll-editor.md) and [PluginChainView](plugin-chain-view.md). Preserve defensive selection clearing; opening a view must not reselect a removed/previous track during teardown.

## Validation

`App/tests/SplitterCollapseControllerTests.cpp` exercises the production layout/controller: invalid/enlarged stored heights, maximum timeline boundary, growth/shrink clamps, applied-distance limits and collapse only after the resize phase. It does not instantiate the entire arrangement activation path.

Focused native checks should open a collapsed MIDI editor by clip double-click, enlarge/release/shrink on a new drag, traverse collapse resistance without pitch drift and resize the main window. Build/focused helper tests alone are not proof of every mouse/focus path. Original fix details remain in [splitter history](../archive/changes/piano-roll-splitter-resize-fix.md) and [activation history](../archive/changes/piano-roll-double-click-expand.md).

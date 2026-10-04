# NextStudio issue implementation plan

This document tracks the complete implementation process for the open GitHub issues reviewed with `gh`.

Status markers:

- `[x]` completed and validated
- `[~]` in progress
- `[ ]` pending
- `[!]` blocked by a product or platform decision

## Mandatory workflow for every implementation batch

The following sequence is binding and repeats for every batch. It is not a global completion checklist. Track actual progress with checkboxes under the affected issue; completed issues remain completed when a new batch starts.

1. Refresh the current open issues and milestones from GitHub with `gh`; review the repository state, release state, changelog, source layout, and existing tests. Confirm release priority and dependencies.
2. Reproduce or verify each issue against current `main` before changing code.
3. Document the root-cause analysis and present a concrete solution proposal before implementation.
4. Obtain the maintainer's approval for the proposed solution; discuss and revise it where necessary. Do not implement before approval.
5. Add or update automated regression coverage where the behavior can be isolated; otherwise document why focused runtime or visual validation is appropriate.
6. Implement only the approved, smallest coherent change with undo, persistence, and platform behavior considered where applicable.
7. Update technical documentation under `docs/components/`, `docs/architecture/`, or `docs/development/` and user documentation under `docs/user/` or `docs/ui/` as appropriate. Update `CHANGELOG.md` for user-visible changes.
8. Build with `BUILD_JOBS=12 ./build.sh rd`.
9. Run `./test.sh rd`.
10. Perform focused UI/runtime validation, using the debug shell where practical.
11. Produce the shared test artifact with `./build_and_copy_shared.sh` and let the maintainer test the software. Repeat the relevant validation and artifact steps after any resulting changes.
12. Receive maintainer validation and re-check the affected GitHub issue acceptance criteria before marking the item complete. Commit or push only when explicitly requested.

## Completed initial planning

- [x] Load the open issues and milestones from GitHub with `gh`.
- [x] Review the repository state, release state, changelog, source layout, and existing tests.
- [x] Group issues by release priority, subsystem, and implementation dependency.

## Phase 1 — Triage and v0.06 release blockers

- [x] **#88 — NoteEditor rendered incorrectly after transparent theme colors were removed**
  - [x] Confirm the opaque-theme normalization and affected timeline drawing path in the current source.
  - [x] Analyze the rendering failure and prepare a solution proposal without changing production code.
  - [x] Obtain approval for the renderer-owned timeline-band opacity; initially 20%, then adjusted to 30% after visual review.
  - [x] Implement the approved rendering fix in the shared grid path used by Song Editor, Piano Roll, velocity, and automation contexts.
  - [x] Add regression coverage for preservation of the RGB tint and application of renderer-owned opacity.
  - [x] Validate the rendering visually: Dark Song Editor validated through the debug shell; the final 30% intensity was accepted in user testing.
  - [x] Update technical documentation, user documentation, the change record, and the changelog.
  - [x] Build with `BUILD_JOBS=12 ./build.sh rd`; run all 19 tests successfully; create the shared artifact.
  - [x] Receive user validation and approval to commit the completed implementation.
- [x] **#64 — Theme hex fields block General Settings scrolling**
  - [x] Identify the existing wheel forwarding and the changelog entry on current `main`.
  - [x] Verify all acceptance criteria with the focused runtime regression test.
  - [x] Treat as already implemented and close after validation.
- [x] **#66 — EQ bands do not reset on double-click**
  - [x] Reproduce from the graph event path: `EqResponseGraphComponent` handles down/drag/up/move/wheel but has no double-click handler.
  - [x] Confirm the factory defaults from Tracktion's attached parameter values: Low `80 Hz / 0 dB / 0.5`, Mid 1 `3000 Hz / 0 dB / 0.5`, Mid 2 `5000 Hz / 0 dB / 0.5`, High `17000 Hz / 0 dB / 0.5`.
  - [x] Prepare the solution proposal: reset all three parameters of the hit band from `getDefaultValue()`, send synchronous notifications, and group the changes into one named undo transaction.
  - [x] Obtain maintainer approval for the whole-band reset semantics, extended with a right-click **reset values** menu action.
  - [x] Add focused regression coverage for all four bands, factory-default sourcing, immediate values, and single-step undo/redo.
  - [x] Implement the approved graph double-click and right-click reset actions; update EQ documentation and changelog.
  - [x] Build successfully, run all 20 tests, and create the shared artifact.
  - [x] Receive maintainer UI validation for double-click, the **reset values** menu, and undo/redo; close #66.
- [x] **#67 — Reverb header text overlaps at narrow widths**
  - [x] Confirm the rendering failure: the title and status are independently drawn into the same full-width header rectangle, so JUCE fits each string without reserving space for the other.
  - [x] Prepare the solution proposal and revise it after review: remove the redundant Wet/Dry/Freeze status from the header entirely and reserve the header for the title.
  - [x] Obtain maintainer approval for the title-only header behavior.
  - [x] Confirm that no focused unit test is warranted: the fix removes one paint-only text operation and introduces no layout logic; retain focused visual validation at the minimum supported width.
  - [x] Implement the approved title-only header and update the Reverb documentation, technical change record, documentation index, and changelog.
  - [x] Build successfully, run all 20 tests, and create the shared artifact.
  - [x] Receive maintainer approval, commit and push `e654677`, and close #67.
- [x] **#68 — Compressor and Delay controls are too small**
  - [x] Confirm the root cause: both editors request only width factor 2 and stack a large graph above dense control rows; fixed 20 px title and 15 px value labels leave as little as 14–16 px for rotary sliders, whose renderer then has almost no drawable radius.
  - [x] Prepare the solution proposal: request width factor 3, place each graph beside rather than above its controls, arrange Compressor knobs as two rows of three above a full-width sidechain footer, and arrange Delay controls as one choice row plus two knob rows.
  - [x] Obtain maintainer approval for the wider side-by-side editor layouts.
  - [x] Extract and test the responsive rectangle calculations at narrow, default, and wide supported sizes, including non-overlap and minimum control-cell dimensions.
  - [x] Implement both layouts without changing parameter, automation, MIDI-learn, or sidechain behavior; update plug-in documentation, technical documentation, test documentation, and changelog.
  - [x] Address the first visual review: provide the Sidechain Trigger button name required by the custom toggle renderer, reserve additional Delay control width, and weight the choice row toward Mode.
  - [x] Address the second visual review: suppress the generic toggle checkmark through a component ID and widen the Delay Sync choice while preserving the weighted Mode/Sync/Division row.
  - [x] Review the Delay screenshot and refine its visual balance: use a compact 64–68 px choice row, split the remaining height equally between knob rows, keep a 150 px graph where possible, use full-width choice boxes, and spell out Feedback.
  - [x] Rebuild successfully, run all 21 tests, and refresh the shared artifact after the screenshot-driven refinement.
  - [x] Receive maintainer visual revalidation for the refined Delay editor and close #68.
- [x] **#70 — Pitch Shifter layout uses excessive space** — completed; maintainer validated the compact editor and snapped graph input and approved commit, push, and issue closure.
  - [x] Refresh issue details and repository state; confirm priority and dependencies. #70 is open in v0.06 alpha; current branch is `main`, with no production-code changes in the working tree. No implementation dependency blocks this layout-only change.
  - [x] Verify and analyze the layout problem in the current source. `PitchShiftPlugin` exposes one automatable parameter, Semitones. `PluginChainItemView` has no dedicated Pitch Shifter branch, so it falls back to `VstPluginComponent`: width factor 3, a 30 px last-changed-parameter row, and a viewport containing another 30 px row for the same parameter. The remaining height is unused. This is source verification, not visual runtime validation.
  - [x] Document the root cause and present a concrete layout proposal, revised after the maintainer requested a more distinctive GUI. Review `DelayPluginComponent`, `ChorusPluginComponent`, `PhaserPluginComponent`, `FilterPluginComponent`, and `EffectEditorLayout`: dedicated `PluginViewComponent` editors combine reusable automatable controls with a separate parameter-driven graph, track-coloured panel headers, and explicit responsive bounds. Proposed Pitch Shifter editor: retain width factor 1, but place a compact, read-only **PITCH MAP** above a single standard Semitones knob with its existing value label. The map uses a vertical interval scale from -24 to +24 semitones, labelled at octave intervals; a subdued reference at 0 and a track-coloured output marker/connecting arrow show the shift's direction and magnitude, with fractional shifts positioned continuously. At zero the markers coincide. This represents the configured transposition, not measured input/output notes or an audio analyser. React to effective parameter changes, including automation; no independent animation timer or audio analysis is needed. Reuse `GUIHelpers::drawHeaderBox`, theme backgrounds, and `AutomatableParameterComponent`; keep the graph non-interactive to preserve the standard knob's input, MIDI-learn, and undo behavior. Bound the graph and knob sizes so neither grows disproportionately, with tested bounds and readable labels at narrow/default/wide sizes. Rack-item width becomes one third of its previous allocation at the same rack height; the shared rack height, existing parameter range/formatting, fractional precision, DSP, and automation remain unchanged. The maintainer approved this revised proposal.
  - [x] Obtain maintainer approval before implementation.
  - [x] Add regression coverage: extend `EffectEditorLayoutTests` with compact width policy, narrow/default/wide layout bounds, graph/knob size caps, centering, tiny-size safety, octave positions, zero alignment, clamping, and fractional shift precision. The standalone test run passes.
  - [x] Implement the initially approved layout change: dedicated `PitchShiftPluginComponent`, initially read-only parameter-listener-driven Pitch Map, and a single standard Semitones control; retain the existing engine and input behavior.
  - [x] Update technical and user documentation and `CHANGELOG.md`.
  - [x] Build with `BUILD_JOBS=12 ./build.sh rd`.
  - [x] Run `BUILD_JOBS=12 ./test.sh rd`; all 21 tests pass.
  - [x] Perform focused UI/runtime validation in an isolated debug-shell session on a temporary X display. Review the header, all five scale labels, the knob/value, and marker positions at 0, +12, -12, +0.5, and +24 semitones; verify fractional knob dragging through state dumps. Compare against the previous shared build to confirm removal of the duplicated control and width reduction. Layout tests cover narrow/default/wide and tiny sizes; runtime screenshots validate the normal rack allocation.
  - [x] Check the observed undo behavior against the previous shared build: in both headless sessions, the toolbar undo attempt after a pitch-knob drag left the native parameter at the changed value. This is not a newly introduced display behavior; successful pitch undo/redo is not claimed as validated by this batch.
  - [x] Create the shared artifact with `./build_and_copy_shared.sh`: `/home/ai/Gemeinsam/NextStudio`.
  - [x] Address maintainer feedback and revalidate: add whole-semitone input through the Pitch Map while keeping the knob and native automation parameter continuous.
    - [x] Analyze the request: the continuous native range must not be quantized globally; add a separate marker-only input path using the existing scale geometry and parameter notifications.
    - [x] Obtain approval: the maintainer explicitly requested vertically dragging the coloured point with whole-semitone snapping.
    - [x] Add regression coverage: test relative drag mapping and all 49 integer values; add `PitchShiftDragTests` for live/persisted values, notifications, balanced gestures, exact fractional undo/redo, no-op/cancel/destruction, and preservation of the continuous native range.
    - [x] Implement marker hit testing, hover/cursor/tooltip feedback, relative snapped dragging, clamping, Escape cancellation, and one graph-specific undo action per gesture. Suppress intermediate CachedValue undo recording only during graph updates; keep the parameter binding and standard knob unchanged.
    - [x] Update technical and user documentation, test documentation, and `CHANGELOG.md`.
    - [x] Build with `BUILD_JOBS=12 ./build.sh rd`; run `BUILD_JOBS=12 ./test.sh rd` successfully (22 tests).
    - [x] Validate in the debug shell: upward drag to +10, one-step undo to 0 and redo to +10, downward drag to -12, limits at +/-24, Escape restoring the fractional start, and continuous knob dragging to -11.232 confirmed by state dumps.
    - [x] Refresh the shared artifact with `./build_and_copy_shared.sh` for maintainer revalidation: `/home/ai/Gemeinsam/NextStudio`.
    - [x] Receive maintainer revalidation and explicit approval of the final implementation and issue closure. No further feedback remains; continuous knob/automation behavior is retained.
  - [x] Receive maintainer validation and re-check acceptance criteria: remove duplicate controls and excess width, preserve readable controls/values, match compact rack sizing, and retain the native parameter range and automation behavior. Maintainer approved commit, push, and marking #70 fixed.
- [ ] **#65 — Bypassed plugins are not fully decolorized**
- [ ] **#71 — Plugin selection menus are not sorted alphabetically**
  - [ ] Use one stable, case-insensitive sorting policy for filtered and unfiltered menus.
  - [ ] Add unit coverage for categories and entries.

## Phase 2 — Editing correctness and regression hardening

- [ ] **#75 — Uneven grid quantization at certain zoom levels**
  - [ ] Capture exact failing zoom, tempo, meter, and snap combinations.
  - [ ] Isolate grid interval/position calculations for regression tests.
- [ ] **#77 — Pencil tool cannot create or shorten notes below insert length**
  - [ ] Extend `PianoRollNoteLengthTests`.
  - [ ] Preserve snapping, minimum length, preview, commit, and undo behavior.
- [ ] **#84 — Velocity lollipops cannot be selected with a lasso**
  - [ ] Implement velocity-lane-local lasso semantics.
  - [ ] Support replace/add selection modifiers and selected-marker feedback.

## Phase 3 — Focused workflow improvements

- [ ] **#87 — Visible feedback after saving**
  - [ ] Cover Save and Save As without a modal interruption.
- [ ] **#73 — Right-click erases notes in MIDI Draw Mode**
  - [ ] Preserve undo and context-menu behavior outside note hits.
- [ ] **#54 — Hotkeys for Song Editor and MIDI Editor tools**
- [ ] **#81 — Shortcuts for track arm, mute, and solo**
  - [ ] Build on the shared application-command/key-mapping infrastructure from #54.
- [ ] **#85 — Add tracks directly below another track or into a folder**
- [ ] **#74 — Auto-scroll MIDI editor while dragging notes**
- [ ] **#86 — Auto-scroll track list while reordering tracks**
  - [ ] Share a tested edge-scroll policy with #74 where component boundaries allow it.

## Phase 4 — Larger editor features

- [ ] **#72 — Audition notes while drawing or inserting**
  - [ ] Add a persistent MIDI-toolbar toggle.
  - [ ] Guarantee note-off on mouse-up, cancellation, tool changes, and editor destruction.
- [ ] **#76 — Configurable playhead positioning on editor clicks**
  - [ ] Keep ruler clicks active and prevent insertion double-clicks from moving the playhead.
- [ ] **#83 — Separate follow-playhead state per timeline**
  - [ ] Store state on each timeline view node.
  - [ ] Migrate the legacy global state and default MIDI follow to off.
- [ ] **#82 — Join/glue selected MIDI clips**
  - [ ] Decide and document gap, overlap, loop/take, ordering, and selection semantics.
  - [ ] Implement as one atomic undoable command.
- [ ] **#56 — Two-dimensional middle-mouse panning**
- [ ] **#57 — Alt modifiers and wheel-based tool switching**
  - [ ] Implement after tool commands from #54 are centralized.

## Phase 5 — Platform, reliability, and plugin organization

- [ ] **#60 — Space toggles playback while a plugin window is focused**
  - [ ] Avoid stealing text-entry and plugin-specific key input.
  - [ ] Test native plugin windows on supported platforms.
- [!] **#59 — Plugin editor windows always on top**
  - [ ] Resolve the product/platform policy against the existing normal Linux stacking behavior.
- [ ] **#79 — Crash backtraces/minidumps and next-start reporting**
  - [ ] Split into POSIX capture, Windows minidumps, crash context, symbol artifacts, and startup-report UI.
  - [ ] Keep crash-handler code async-signal-safe and uploads strictly user-controlled.
- [ ] **#58 — Favorite plugins and custom categories**
  - [ ] Introduce persistent plugin metadata after the deterministic ordering work in #71.

## Deferred

- [ ] **#21 — Detachable/fullscreen Piano Roll window** — intentionally deferred to Post v1.0.

## Issue metadata cleanup

- [ ] Add appropriate feature/enhancement labels to #81, #82, and #83.
- [ ] Place #88 in the v0.06 milestone as a release blocker.
- [ ] Clarify #75 reproduction details and #82 merge semantics before implementation.
- [ ] Record the decision required for #59.

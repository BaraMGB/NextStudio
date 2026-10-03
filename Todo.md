# NextStudio issue implementation plan

This document tracks the complete implementation process for the open GitHub issues reviewed with `gh`.

Status markers:

- `[x]` completed and validated
- `[~]` in progress
- `[ ]` pending
- `[!]` blocked by a product or platform decision

## Process applied to every implementation batch

- [x] Load the current open issues and milestones from GitHub with `gh`.
- [x] Review the repository state, release state, changelog, source layout, and existing tests.
- [x] Group issues by release priority, subsystem, and implementation dependency.
- [~] Reproduce or verify each issue against current `main` before changing code.
- [ ] Document the root-cause analysis and present a concrete solution proposal before implementation.
- [ ] Obtain the maintainer's approval for the proposed solution; discuss and revise it where necessary.
- [ ] Add or update automated regression coverage where the behavior can be isolated.
- [ ] Implement only the approved, smallest coherent change with undo, persistence, and platform behavior considered where applicable.
- [ ] Update technical documentation under `docs/components/`, `docs/architecture/`, or `docs/development/` as appropriate.
- [ ] Update user documentation under `docs/user/` or `docs/ui/` as appropriate.
- [ ] Update `CHANGELOG.md` for user-visible changes.
- [ ] Build with `BUILD_JOBS=12 ./build.sh rd` after each relevant batch.
- [ ] Run `./test.sh rd` after each relevant batch.
- [ ] Perform focused UI/runtime validation, using the debug shell where practical.
- [ ] Produce the shared test artifact with `./build_and_copy_shared.sh` after a batch is ready for user testing.
- [ ] Re-check the affected GitHub issue acceptance criteria before marking the item complete.

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
- [ ] **#64 — Theme hex fields block General Settings scrolling**
  - [x] Identify the existing wheel forwarding and the changelog entry on current `main`.
  - [ ] Verify all acceptance criteria at runtime.
  - [ ] Treat as already implemented and close only after validation.
- [ ] **#66 — EQ bands do not reset on double-click**
- [ ] **#67 — Reverb header text overlaps at narrow widths**
- [ ] **#68 — Compressor and Delay controls are too small**
- [ ] **#70 — Pitch Shifter layout uses excessive space**
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

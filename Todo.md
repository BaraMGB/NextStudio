# NextStudio open-work queue

Synced from [GitHub issues](https://github.com/BaraMGB/NextStudio/issues) on 2026-11-07: **13 open issues**. Entries are grouped by milestone, not implementation priority.

GitHub issues are tracker entries for reported work and status only. Solution proposals, review, validation and acceptance take place directly in the active Pi Agent conversation. Refresh issue/milestone state before starting a batch. Follow the [mandatory contribution workflow](docs/development/contributing.md) and [documentation maintenance schema](docs/development/documentation-policy.md).

## v0.07 alpha

[Milestone](https://github.com/BaraMGB/NextStudio/milestone/7) — 11 open issues.

### Editing and arrangement

- [ ] [#97 — Autoscroll when group selecting events/notes](https://github.com/BaraMGB/NextStudio/issues/97) — allow selection of events and notes initially outside the viewport.
- [ ] [#100 — Auto-scroll Song Editor horizontally and vertically while moving clips](https://github.com/BaraMGB/NextStudio/issues/100) — edge scrolling for move/copy drags, including multiple clips; update preview and destination continuously, and stop cleanly on completion or cancellation. Resizing, lasso selection and other drag operations are outside this issue's scope.
- [ ] [#101 — Track duplication](https://github.com/BaraMGB/NextStudio/issues/101) — duplicate tracks for plugin variations and double tracking.

### Audio and routing

- [ ] [#96 — Effect sends / aux send tracks](https://github.com/BaraMGB/NextStudio/issues/96) — send audio from selected tracks to a shared effect instance without requiring a folder.

### Plugin workflows and content import

- [ ] [#107 — Instrument browser double-click and drag-and-drop hot-swap](https://github.com/BaraMGB/NextStudio/issues/107) — create a new instrument track on double-click; replace an existing instrument only through explicit drag-and-drop with a warning.
- [ ] [#108 — Drag audio files from the system file browser into NextStudio](https://github.com/BaraMGB/NextStudio/issues/108) — support external audio-file drops in the Song Editor and on DrumSampler pads using the existing internal drop rules.

### UI and application controls

- [ ] [#106 — Plugin cards in the bottom panel have insufficient contrast](https://github.com/BaraMGB/NextStudio/issues/106) — make individual plugin cards clearly distinguishable from the panel background.
- [ ] [#109 — Add an in-app Quit button and Ctrl/Cmd+Q shortcut](https://github.com/BaraMGB/NextStudio/issues/109) — route both controls through the existing project-aware quit workflow.

### Platform, reliability and plugin windows

- [ ] [#59 — Temporarily pin individual plugin editor windows](https://github.com/BaraMGB/NextStudio/issues/59) — add a non-persistent per-window title-bar toggle that keeps only selected plugin editors on top until they are unpinned or their host window is destroyed.
- [ ] [#60 — Space bar playback regardless of focused plugin window](https://github.com/BaraMGB/NextStudio/issues/60) — preserve text/plugin-specific key input; test supported native windows.
- [ ] [#79 — Capture crash backtraces/minidumps and offer reporting on next startup](https://github.com/BaraMGB/NextStudio/issues/79) — crash diagnostics, offline symbolization, archived release symbols and next-start report UI; async-signal-safe handlers and no automatic uploads.

## Post v1.0

[Milestone](https://github.com/BaraMGB/NextStudio/milestone/5) — 2 open issues.

- [ ] [#21 — Detachable / fullscreen Piano Roll](https://github.com/BaraMGB/NextStudio/issues/21) — independent MIDI editor window that can be maximized or moved to another monitor.
- [ ] [#105 — Built-in SFZ instrument player](https://github.com/BaraMGB/NextStudio/issues/105) — load and play `.sfz` instruments through a maintained engine, with relative sample paths and clear missing-file errors.

## History

- #103 (Windows Direct2D grid-emphasis test failure) was closed on 2026-11-05 by [PR #104](https://github.com/BaraMGB/NextStudio/pull/104).
- #95 (Clip/content representation on folder tracks) was completed on 2026-11-04 and is no longer part of the open-work queue.
- #102 (Per-clip volume adjustment for audio clips) was closed on GitHub on 2026-11-03 by commit [`566d762`](https://github.com/BaraMGB/NextStudio/commit/566d762).
- #57 (Alt modifiers and wheel-based tool switching) was closed on GitHub on 2026-10-24 and is no longer part of the open-work queue.
- Completed batch logs are preserved in [Git at the pre-migration revision](https://github.com/BaraMGB/NextStudio/blob/ad5912e/Todo.md) (`git show ad5912e:Todo.md` locally). [Historical documentation](docs/archive/README.md) retains design and validation records.
- This synchronization does not reopen, close or otherwise modify GitHub issues.

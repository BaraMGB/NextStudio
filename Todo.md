# NextStudio open-work queue

Synced from [GitHub issues](https://github.com/BaraMGB/NextStudio/issues) on 2026-11-03: **10 open issues**. Entries are grouped by milestone, not implementation priority.

GitHub issues are tracker entries for reported work and status only. Solution proposals, review, validation and acceptance take place directly in the active Pi Agent conversation. Refresh issue/milestone state before starting a batch. Follow the [mandatory contribution workflow](docs/development/contributing.md) and [documentation maintenance schema](docs/development/documentation-policy.md).

## v0.07 alpha

[Milestone](https://github.com/BaraMGB/NextStudio/milestone/7) — 9 open issues.

### Editing and arrangement

- [ ] [#95 — Clip/content representation on folder tracks](https://github.com/BaraMGB/NextStudio/issues/95) — show folder contents when collapsed to clarify project structure and simplify copying.
- [ ] [#97 — Autoscroll when group selecting events/notes](https://github.com/BaraMGB/NextStudio/issues/97) — allow selection of events and notes initially outside the viewport.
- [ ] [#100 — Auto-scroll Song Editor horizontally and vertically while moving clips](https://github.com/BaraMGB/NextStudio/issues/100) — edge scrolling for move/copy drags, including multiple clips; update preview and destination continuously, and stop cleanly on completion or cancellation. Resizing, lasso selection and other drag operations are outside this issue's scope.
- [ ] [#101 — Track duplication](https://github.com/BaraMGB/NextStudio/issues/101) — duplicate tracks for plugin variations and double tracking.

### Audio and routing

- [ ] [#96 — Effect sends / aux send tracks](https://github.com/BaraMGB/NextStudio/issues/96) — send audio from selected tracks to a shared effect instance without requiring a folder.
- [ ] [#102 — Per-clip volume adjustment for audio clips](https://github.com/BaraMGB/NextStudio/issues/102) — make consistent audio levels easier to achieve.

### Platform, reliability and plugin windows

- [ ] [#59 — Plugin editor windows always on top](https://github.com/BaraMGB/NextStudio/issues/59) — **product/platform decision needed** against normal Linux stacking behavior. Decide with the maintainer in the active Pi Agent conversation before implementation; record the durable outcome in the relevant repository references.
- [ ] [#60 — Space bar playback regardless of focused plugin window](https://github.com/BaraMGB/NextStudio/issues/60) — preserve text/plugin-specific key input; test supported native windows.
- [ ] [#79 — Capture crash backtraces/minidumps and offer reporting on next startup](https://github.com/BaraMGB/NextStudio/issues/79) — crash diagnostics, offline symbolization, archived release symbols and next-start report UI; async-signal-safe handlers and no automatic uploads.

## Post v1.0

[Milestone](https://github.com/BaraMGB/NextStudio/milestone/5) — 1 open issue.

- [ ] [#21 — Detachable / fullscreen Piano Roll](https://github.com/BaraMGB/NextStudio/issues/21) — independent MIDI editor window that can be maximized or moved to another monitor.

## History

- #57 (Alt modifiers and wheel-based tool switching) was closed on GitHub on 2026-10-24 and is no longer part of the open-work queue.
- Completed batch logs are preserved in [Git at the pre-migration revision](https://github.com/BaraMGB/NextStudio/blob/ad5912e/Todo.md) (`git show ad5912e:Todo.md` locally). [Historical documentation](docs/archive/README.md) retains design and validation records.
- This synchronization does not reopen, close or otherwise modify GitHub issues.

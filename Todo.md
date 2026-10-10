# NextStudio open-work queue

GitHub issues own status, discussion and acceptance. This queue preserves the agreed priorities and dependencies; refresh issue/milestone state before starting a batch. Follow the [mandatory contribution workflow](docs/development/contributing.md) and [documentation maintenance schema](docs/development/documentation-policy.md).

The 18 issues below target [v0.06 alpha](https://github.com/BaraMGB/NextStudio/milestone/6); #21 remains deferred to Post v1.0.

## Next — editing correctness

- [ ] [Separate Lasso and TimeRange](docs/plans/lasso-time-range-separation.md) — approved prerequisite for #84; foundation and velocity source implemented, awaiting maintainer testing. Overwrite policy is unchanged.
- [ ] [#84 — Velocity lollipops cannot be selected with a lasso](https://github.com/BaraMGB/NextStudio/issues/84) — velocity source and persistent selected-marker feedback implemented; awaiting maintainer testing.

## Focused workflow improvements

- [ ] [#87 — Visible feedback after saving](https://github.com/BaraMGB/NextStudio/issues/87) — Save and Save As without modal interruption.
- [ ] [#73 — Right-click erases notes in MIDI Draw Mode](https://github.com/BaraMGB/NextStudio/issues/73) — preserve undo/context-menu behavior outside note hits.
- [ ] [#54 — Hotkeys for Song Editor and MIDI Editor tools](https://github.com/BaraMGB/NextStudio/issues/54).
- [ ] [#81 — Shortcuts for track arm, mute and solo](https://github.com/BaraMGB/NextStudio/issues/81) — reuse command/key-mapping infrastructure from #54.
- [ ] [#85 — Add tracks below another track or into a folder](https://github.com/BaraMGB/NextStudio/issues/85).
- [ ] [#74 — Auto-scroll MIDI editor while dragging notes](https://github.com/BaraMGB/NextStudio/issues/74).
- [ ] [#86 — Auto-scroll track list while reordering tracks](https://github.com/BaraMGB/NextStudio/issues/86) — share a tested edge-scroll policy with #74 where boundaries allow.

## Larger editor features

- [ ] [#72 — Audition notes while drawing/inserting](https://github.com/BaraMGB/NextStudio/issues/72) — persistent MIDI-toolbar toggle; guarantee note-off on release, cancellation, tool changes and destruction.
- [ ] [#76 — Configurable playhead positioning on editor clicks](https://github.com/BaraMGB/NextStudio/issues/76) — keep ruler clicks active; insertion double-clicks must not move the playhead.
- [ ] [#83 — Separate follow-playhead state per timeline](https://github.com/BaraMGB/NextStudio/issues/83) — store per view; migrate global state; default MIDI follow off.
- [ ] [#82 — Join/glue selected MIDI clips](https://github.com/BaraMGB/NextStudio/issues/82) — decide gap/overlap/loop/take/order/selection semantics first; one atomic undoable command.
- [ ] [#56 — Two-dimensional middle-mouse panning](https://github.com/BaraMGB/NextStudio/issues/56).
- [ ] [#57 — Alt modifiers and wheel-based tool switching](https://github.com/BaraMGB/NextStudio/issues/57) — after tool commands from #54 are centralized.

## Platform, reliability and plugin organization

- [ ] [#60 — Space toggles playback with a plugin window focused](https://github.com/BaraMGB/NextStudio/issues/60) — preserve text/plugin-specific key input; test supported native windows.
- [!] [#59 — Plugin editor windows always on top](https://github.com/BaraMGB/NextStudio/issues/59) — product/platform decision needed against normal Linux stacking behavior.
- [ ] [#79 — Crash capture and next-start reporting](https://github.com/BaraMGB/NextStudio/issues/79) — POSIX capture, Windows minidumps, context, symbols and startup UI; async-signal-safe handlers and user-controlled uploads.
- [ ] [#58 — Favorite plugins and custom categories](https://github.com/BaraMGB/NextStudio/issues/58) — persistent metadata following completed deterministic ordering (#71).

## Deferred and metadata

- [ ] [#21 — Detachable/fullscreen Piano Roll](https://github.com/BaraMGB/NextStudio/issues/21) — Post v1.0.
- [ ] Record the #82 merge semantics and #59 platform decision in their issues before implementing.

## History

Completed batch logs are preserved in [Git at the pre-migration revision](https://github.com/BaraMGB/NextStudio/blob/ad5912e/Todo.md) (`git show ad5912e:Todo.md` locally). [Historical documentation](docs/archive/README.md) retains design and validation records. No software issue is reopened or closed by this queue cleanup.

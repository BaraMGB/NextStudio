# NextStudio open-work queue

GitHub issues are tracker entries for reported work and status only. Solution proposals, review, validation and acceptance take place directly in the active Pi Agent conversation. This queue preserves the agreed priorities and dependencies; refresh issue/milestone state before starting a batch. Follow the [mandatory contribution workflow](docs/development/contributing.md) and [documentation maintenance schema](docs/development/documentation-policy.md).

The 6 open issues below target [v0.06 alpha](https://github.com/BaraMGB/NextStudio/milestone/6); #21 remains deferred to Post v1.0.

## Larger editor features

- [ ] [#56 — Two-dimensional middle-mouse panning](https://github.com/BaraMGB/NextStudio/issues/56).
- [ ] [#57 — Alt modifiers and wheel-based tool switching](https://github.com/BaraMGB/NextStudio/issues/57) — build on the centralized tool commands from #54.

## Platform, reliability and plugin organization

- [ ] [#60 — Space toggles playback with a plugin window focused](https://github.com/BaraMGB/NextStudio/issues/60) — preserve text/plugin-specific key input; test supported native windows.
- [!] [#59 — Plugin editor windows always on top](https://github.com/BaraMGB/NextStudio/issues/59) — product/platform decision needed against normal Linux stacking behavior.
- [ ] [#79 — Crash capture and next-start reporting](https://github.com/BaraMGB/NextStudio/issues/79) — POSIX capture, Windows minidumps, context, symbols and startup UI; async-signal-safe handlers and user-controlled uploads.
- [ ] [#58 — Favorite plugins and custom categories](https://github.com/BaraMGB/NextStudio/issues/58) — persistent metadata following completed deterministic ordering (#71).

## Deferred and metadata

- [ ] [#21 — Detachable/fullscreen Piano Roll](https://github.com/BaraMGB/NextStudio/issues/21) — Post v1.0.
- [ ] Decide the #59 platform behavior with the maintainer in the active Pi Agent conversation before implementing; transfer the durable outcome into the relevant repository references.

## History

Completed batch logs are preserved in [Git at the pre-migration revision](https://github.com/BaraMGB/NextStudio/blob/ad5912e/Todo.md) (`git show ad5912e:Todo.md` locally). [Historical documentation](docs/archive/README.md) retains design and validation records. No software issue is reopened or closed by this queue cleanup.

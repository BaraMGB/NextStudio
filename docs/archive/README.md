# Historical documentation

- Type: historical index
- Audience: contributors
- Scope: completed change records and validation snapshots

These records preserve original designs, tradeoffs and results. Future-tense instructions, unchecked boxes and intermediate "pending" statements refer to the historical iteration, not today's queue. Use the [current index](../README.md) for supported behavior/procedures, [active plans](../plans/README.md) for unresolved proposals and [GitHub issues](https://github.com/BaraMGB/NextStudio/issues) for current software status.

All 18 original change records are archived after extracting current knowledge. Old `docs/changes/` paths are compatibility redirects, not parallel manuals. Original record bodies/language are retained apart from necessary relative-link adjustments and explicit status/current-reference headers.

## Timeline and interaction

| Record | Known outcome / revision | Current knowledge |
|---|---|---|
| [Uneven grid plan/follow-ups](changes/uneven-timeline-grid-plan.md) | #75, `44f805e` | [View transform](../components/timeline-view-transform.md) |
| [Soft snapping plan](changes/soft-timeline-snapping-plan.md) and [validation](changes/soft-timeline-snapping-validation.md) | #77, `54bb3a0` | [Timeline snapping](../components/timeline-snapping.md) |
| [Snap feedback/live values plan](changes/snap-feedback-and-live-header-plan.md) and [validation](changes/snap-feedback-and-live-header-validation.md) | `48e8f95` | [Feedback](../components/timeline-snapping.md#snap-feedback-and-live-values), [numeric input](../components/property-field-input.md) |
| [Tool cursor working areas](changes/tool-cursor-working-areas.md) | #90, `ad5912e` | [MIDI cursor contract](../components/piano-roll-editor.md#tool-cursor-working-areas), [Song cursor contract](../components/song-editor.md#cursor-working-areas-and-stationary-changes) |
| [Opaque-theme shading](changes/opaque-theme-timeline-shading.md) | #88 | [Timeline rendering](../components/timeline-view-transform.md#timeline-band-rendering) |
| [Note properties/position display](changes/note-properties-bar-and-position-display.md) | Initial implementation/review record | [NotePropertiesBar](../components/note-properties-bar.md), [shared input](../components/property-field-input.md) |
| [Piano Roll double-click expansion](changes/piano-roll-double-click-expand.md) | Activation/reopen record | [Lower-range activation](../components/lower-range.md#arrangement-activation) |
| [Piano Roll splitter resize](changes/piano-roll-splitter-resize-fix.md) | Resize and scroll-compensation follow-up | [Lower-range sizing](../components/lower-range.md#resize-before-collapse) |

Exact historical raster results describe the renderer/revision tested then. Later #91 fixes (`04f47ef`, `4106810`) allow native-renderer differences and finish drawing before readback; earlier Linux comparisons are not a cross-platform pixel-identity promise.

## MIDI input and project workflows

| Record | Historical context | Current knowledge |
|---|---|---|
| [Computer MIDI keyboard controller](changes/computer-midi-keyboard-controller.md) | #52 refactor and upstream Linux latency experiment | [Controller](../components/computer-midi-keyboard.md), [routing](../architecture/midi-input-routing.md) |
| [Piano Roll MIDI key lighting](changes/piano-roll-midi-key-lighting.md) | Dispatcher, audition and batching | [Live key lighting](../components/piano-roll-editor.md#live-midi-key-lighting) |
| [Embedded project browser](changes/embedded-project-file-browser.md) | Original German architecture/workflow record | [Directory browser](../components/directory-browser.md), [project workflow](../architecture/project-workflow.md), [lifecycle](../architecture/project-lifecycle.md) |

## Rack and built-in effects

| Record | Known outcome / rationale | Current knowledge |
|---|---|---|
| [Complete bypass presentation](changes/complete-bypass-presentation.md) | #65 accepted grayscale/badge treatment | [Rack bypass contract](../components/plugin-chain-view.md#whole-item-bypass-presentation) |
| [Compressor/Delay layouts](changes/compressor-delay-control-layouts.md) | Responsive readable controls | [Effect layouts](../components/built-in-effect-editors.md#dense-control-layouts) |
| [EQ whole-band reset](changes/eq-band-reset.md) | Native defaults and atomic notifications/undo | [EQ reset](../components/built-in-effect-editors.md#eq-whole-band-reset) |
| [Compact Pitch Shifter map](changes/pitch-shifter-compact-map.md) | #70 accepted; graph/knob validation differs | [Pitch Map](../components/built-in-effect-editors.md#compact-pitch-shifter-and-pitch-map) |
| [Reverb header simplification](changes/reverb-header-simplification.md) | Paint-only removal of duplicated status | [Reverb header](../components/built-in-effect-editors.md#reverb-header) |

## Platform validation

- [Wine/Bottles pre-migration snapshot](changes/wine-bottles-validation.md) — original Ubuntu 24.04/Bottles/Wine 11 observations and reported native Windows RDP result. Exact tested package revision/durable evidence are not established. Current procedure, import guard, repaint timer and limitations live in [Wine/Bottles compatibility](../development/wine-bottles.md).

## Documentation migration

- [Reorganization plan](changes/documentation-reorganization-plan.md) and [migration map](changes/documentation-migration-map.md) — completed, reviewed and accepted, including Markdown-checker review fixes. Current rules live in the [documentation maintenance schema](../development/documentation-policy.md) and [contribution workflow](../development/contributing.md).

## Evidence availability

`/tmp/`, shared-folder and contributor-home paths are historical locations, not durable downloads. Selected #90 working-area/transition scripts were available during migration and became [maintained native procedures](../development/testing.md#timeline-cursor-validation). Other action, pan, fractional-scale, grid, snapping, rack and Wine snapshots are not all checked-in reproductions or verified available archives. Their reported results remain intact; no bulk evidence upload or permanent artifact retention is claimed.

For future noteworthy runs, attach relevant evidence to the issue/release or retain a CI artifact with an explicit retention limit; ordinary CI logs/artifacts are not necessarily permanent. Record missing evidence as unavailable/unknown instead of claiming independent verification. The pre-migration task log remains in [Git at `ad5912e`](https://github.com/BaraMGB/NextStudio/blob/ad5912e/Todo.md), including original approvals and runtime details.

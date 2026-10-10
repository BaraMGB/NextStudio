# Documentation migration map

- Type: historical record
- Audience: contributors and maintainer
- Scope: migration inventory and pre-acceptance validation snapshots
- Status: implemented and accepted; maintainer authorized commit/push after review fixes.

The inventory and validation snapshots below preserve their pre-final-review context. The completion note records the final outcome.

## Inventory scope and verification

The starting inventory covered 65 Markdown pages under `docs/`, plus the two newly proposed maintenance/reorganization documents. Every starting page is classified below. Paths in the first column identify the original location; retain means no move is currently justified, not that every technical sentence has been exhaustively revalidated.

This is a migration inventory with source/test associations, not certification of all behavior. After maintainer pilot acceptance, all eleven remaining mixed records were read/extracted against corresponding property, MIDI, browser, lower-range and effect sources/tests. Current Wine prose was separated from its pre-migration observations and checked against the fallback implementation. Untouched references retain their previous scope; no exhaustive application/platform certification is inferred. #75, #77, #88, #90 and #91 are closed; #84 remains next, and the open queue retains all 19 open software issues. No issue/milestone/label was modified.

Internal incoming links were searched across README, repository instructions, workflows and documentation. GitHub issue bodies were inspected for documentation references, and #90's acceptance comment was checked. External comment/wiki/bookmark coverage is not exhaustive; all 18 moved change-record paths retain redirect stubs. Git history preserves the pre-migration `Todo.md` at `ad5912e`.

## Current references and procedures — starting pages

Source paths in this table are repository-relative associations for review. Where a page already has a detailed source/test map, that map remains authoritative instead of being reproduced here.

| Original page (`docs/`) | Type / audience | Authority and action | Source/test association; overlaps to review |
|---|---|---|---|
| `README.md` | Index / all readers | Rewrite task entries; keep current index, one history/plans entry | Source layout owns repository map; remove duplicate primary-source table |
| `agent-debug.md` | Procedure/reference / contributors | Retain; later audit mixed recommendations/current state | `tools/debug-shell-client.js`, `App/src/DebugAppController.cpp`, debug suites; testing guide links procedures |
| `logging.md` | Procedure / contributors | Retain | `App/include/Logging.h`; source layout/debug docs should link logging rules |
| `architecture/overview.md` | Reference / contributors | Retain ownership/layers | `App/src/Main.cpp`, `MainComponent`, `EditComponent`; source layout describes files, not a second ownership contract |
| `architecture/state-and-events.md` | Reference / contributors | Retain state/event boundaries | `ApplicationViewState`, `EditViewState`, selection/listeners; component-specific details link here |
| `architecture/clip-overwrite-command.md` | Reference / contributors | Retain placement/undo contract | `App/src/ClipOverwriteCommand.cpp`, `App/tests/ClipOverwriteCommandTests.cpp`; Song Editor links this |
| `architecture/midi-input-routing.md` | Reference / contributors | Retain routing authority; link extracted controller/lifetime reference | `App/src/MidiInputRouting.cpp`, `App/tests/MidiInputRoutingTests.cpp`; overlaps two MIDI change records |
| `architecture/playback-graph-reallocation.md` | Reference / contributors | Retain guard/lifetime contract | `ClipOverwriteCommand`, Tracktion `ReallocationInhibitor`; no duplication in gesture docs |
| `architecture/project-lifecycle.md` | Reference / contributors | Retain file/recovery authority; clarify exact save paths and single workflow decision | `ProjectLifecycle.cpp`, lifecycle tests; distinguish file validation from workflow controller |
| `architecture/project-workflow.md` | Reference / contributors | Retain typed intent/lock authority; extract browser navigation and post-lock clean check | `ProjectWorkflow.cpp`, workflow tests; overlaps browser record |
| `architecture/startup-wizard.md` | Reference / contributors | Retain | `SetupWizard.cpp`, `MainInteractionStateTests.cpp`; lifecycle links recovery ordering |
| `components/clip-properties-bar.md` | Reference / contributors | Consolidate clip planning/commit reference; shared formats/input link to new scoped reference | `ClipPropertiesBar.cpp`, `ClipPropertyEdit.h`; shared snap/lifecycle rules link timeline contract |
| `components/note-properties-bar.md` | Reference / contributors | Consolidate selection/planning/recreation/undo; shared formats/input extracted | `NotePropertiesBar.cpp`, `PositionDisplayHelpers.cpp`, position tests; overlaps initial change record |
| `components/piano-roll-editor.md` | Reference / contributors | Retain MIDI cursor/editor authority; incorporate dispatcher/audition/lighting lifecycle | `MidiViewport.cpp`, tools, MIDI operation tests; key lighting links current testing checklist |
| `components/timeline-snapping.md` | Reference / contributors | Pilot: authoritative profiles/curve/anchors/constraints/replay/feedback | `TimelineSoftSnap`, `TimelineSnapResolver`, `TimelineInteractionPreview`, snapping tests; extract surviving plan contracts |
| `components/timeline-view-transform.md` | Reference / contributors | Pilot: authoritative zoom/fit/float/band rendering | `TimelineViewGeometry`, `EditViewState`, geometry/state/frame tests; historical exact raster comparisons are environment-specific |
| `components/metronome-settings.md` | Reference / contributors | Retain | `MetronomeSampleManagerTests.cpp`; settings/source map in page |
| `components/theme-settings.md` | Reference / contributors | Retain; link shared timeline band policy where needed | `ThemeSettingsComponent.cpp`, `ThemePresetModelTests.cpp`; persisted colors differ from draw-time effects |
| `components/plugin-chain-view.md` | Reference / contributors | Retain rack/bypass ownership; link extracted effect editor contracts | `PluginChainItemView`, `EffectEditorLayout`, plugin layout/bypass/pitch tests; three change records |
| `development/building.md` | Procedure / contributors | Retain portable defaults | `build.sh`, `start.sh`, packaging; project-local jobs/delivery in contribution guide |
| `development/testing.md` | Procedure / contributors | Pilot: current reproducible native/doc checks and coverage boundaries | `App/CMakeLists.txt`, `App/tests/`, maintained scripts; historical delivery snapshots move out of current procedures |
| `development/source-layout.md` | Reference / contributors | Retain source registration; link schema for documentation layout | `App/CMakeLists.txt`, resources; avoid second directory-policy table |
| `development/wine-bottles.md` | Mixed procedure/history / contributors | Current procedure/fallback/limits retained; original observations archived with known coverage gaps | Wine runtime fallback sources indexed in page; retain rationale/limits before moving observations |
| `user/getting-started.md` | Workflow / users | Retain workflow; remove stale alternative modal-decision claim and clarify failed-save continuation | Main window/project/browser sources; links to screen and project workflow refs |
| `user/piano-roll.md` | Workflow / users | Retain workflow; correct snapping claim and document source-confirmed pitch-name limitation | MIDI editor/tools; user steps distinct from C++ contract |
| `user/peak-limiter.md` | Plugin workflow/reference / users | Retain path; no benefit established for moving | `PeakLimiterPlugin.cpp`, `PeakLimiterPluginComponent.cpp`; index explicitly lists it |
| `ui/header-bar.md` | UI reference / users | Retain | `HeaderComponent`, position display helpers/tests; task guide links controls |
| `ui/mixer.md` | UI reference / users | Retain | Mixer/channel strip sources; no change report dependency |
| `ui/shortcuts.md` | UI reference / users | Retain default user mapping; move settings implementation details into controller reference | Application commands, `KeyboardSettingsComponent.cpp`, keyboard layout tests |
| `ui/side-browser.md` | Mixed UI/reference / users | Retain controls; link current navigation/workflow rather than historical implementation | `DirectoryBrowser.cpp`, `ProjectsBrowser.cpp`; embedded-browser record |
| `ui/song-editor.md` | UI reference / users | Pilot: link arrangement technical contract; remove profile-number duplication | `SongEditorView.cpp`, `TrackLaneComponent.cpp`; component reference owns stationary cursor resolution |
| `ui/track-chain.md` | UI reference / users | Retain | Plugin chain/rack sources; bypass and ordering remain observable behavior |
| `plugins/arpeggiator.md` | Plugin reference / users | Retain | `App/src/ArpeggiatorPluginComponent.cpp` |
| `plugins/chorus.md` | Plugin reference / users | Retain | `App/src/ChorusPluginComponent.cpp` |
| `plugins/compressor.md` | Plugin reference / users | Retain; technical layout constraints elsewhere | `App/src/CompressorPluginComponent.cpp`, `EffectEditorLayoutTests.cpp` |
| `plugins/delay.md` | Plugin reference / users | Retain observable layout; remove duplicated numeric geometry policy | `App/src/DelayPluginComponent.cpp`, `EffectEditorLayoutTests.cpp` |
| `plugins/drum-sampler.md` | Plugin reference / users | Retain | Drum sampler editor/sound editor sources in `App/src/`; inspect when topic changes |
| `plugins/eq.md` | Plugin reference / users | Retain user reset behavior; parameter/undo rationale in effect reference | `App/src/EqPluginComponent.cpp`, `EqBandResetTests.cpp` |
| `plugins/filter.md` | Plugin reference / users | Retain | `App/src/FilterPluginComponent.cpp` |
| `plugins/phaser.md` | Plugin reference / users | Retain | `App/src/PhaserPluginComponent.cpp` |
| `plugins/pitch-shifter.md` | Plugin reference / users | Retain graph/knob user distinction; replace historical width comparison with current layout | `App/src/PitchShiftPluginComponent.cpp`, `PitchShiftDragTests.cpp` |
| `plugins/reverb.md` | Plugin reference / users | Retain title-only header behavior | `App/src/ReverbPluginComponent.cpp`; paint-only change record |
| `plugins/saturation.md` | Plugin reference / users | Retain | `App/src/SaturationPluginComponent.cpp` |
| `plugins/simple-synth.md` | Plugin reference / users | Retain | `App/src/SimpleSynthPluginComponent.cpp` |
| `plugins/soundfont-player.md` | Plugin reference / users | Retain | `App/src/SoundFontPluginComponent.cpp` |
| `plugins/spectrum-analyzer.md` | Plugin reference / users | Retain | `App/src/SpectrumAnalyzerPluginComponent.cpp` |
| `plugins/volume-pan.md` | Plugin reference / users | Retain | `App/src/VolumePluginComponent.cpp` |

## Change records — starting pages

All 18 starting records are historical or mixed historical/current explanations, not active software proposals. Seven moved during the pilot and eleven after pilot acceptance. Current knowledge is extracted; every original body/language is retained with status/current-reference headers and necessary relative-link adjustments. All old paths are redirects.

| Original page (`docs/changes/`) | Classification | Extraction / destination / review |
|---|---|---|
| `uneven-timeline-grid-plan.md` | Historical plan + follow-ups | Pilot archived; view transform owns normalization/float frames/latest deferred fit/extreme-fit tradeoff; #75 `44f805e` |
| `soft-timeline-snapping-plan.md` | Historical plan with stale open checkboxes | Pilot archived; shared snapping owns targets, profiles, inverse anchors, bypass and consumers; #77 `54bb3a0`; preserve original equations as history |
| `soft-timeline-snapping-validation.md` | Historical validation of several profiles | Pilot archived; current procedures in Testing; retain original 100/125% iteration limits separately |
| `snap-feedback-and-live-header-plan.md` | Historical plan + foreground follow-up | Pilot archived; shared feedback owns snapshot/cleanup/queued-field-edit contract; `48e8f95`; property bars retain specific integration |
| `snap-feedback-and-live-header-validation.md` | Historical validation | Pilot archived; preserve helper-versus-GUI, final follow-up and platform limits; no current procedure depends on local screenshots |
| `tool-cursor-working-areas.md` | Historical implementation/validation/delivery | Pilot archived; MIDI/Song references own current cursors; maintained native scripts derive working-area/transition checks; #90 `ad5912e` |
| `opaque-theme-timeline-shading.md` | Historical implementation/rationale | Pilot archived; view transform owns RGB tint/renderer alpha and ThemePresetModel test boundary; #88 |
| `complete-bypass-presentation.md` | Mixed historical/current | Archived; rack reference owns listener/filter lifetime, subtree alpha, badge and preserved interaction; `PluginBypassPresentationTests` |
| `compressor-delay-control-layouts.md` | Mixed historical/current | Archived; built-in effect reference owns responsive helper geometry and coverage boundaries; `EffectEditorLayoutTests` |
| `computer-midi-keyboard-controller.md` | Mixed historical/current | Archived; controller reference owns aliases/focus/lifetime/latency limits; routing links it; `ComputerMidiKeyboardLayoutTests` |
| `embedded-project-file-browser.md` | Mixed historical/current (German) | Archived original German text; directory browser and project architecture own reuse/filtering, dirty checks, Save-As/lock/error/rollback rules |
| `eq-band-reset.md` | Mixed historical/current | Archived; built-in effect reference owns parameter-default/notification/atomic-undo rationale; `EqBandResetTests` |
| `note-properties-bar-and-position-display.md` | Mixed historical/current | Archived; note bar/shared input own parsing, coordinate/recreation/selection safety and absolute/relative constraints |
| `piano-roll-double-click-expand.md` | Historical implementation | Archived; lower-range reference owns activation/reopen policy, with focused-test versus GUI distinction; splitter/layout tests |
| `piano-roll-midi-key-lighting.md` | Mixed historical/current | Archived; Piano Roll owns dispatcher/audition/batching/lifetime flow; Testing owns checklist; no velocity/playback-lighting claim |
| `piano-roll-splitter-resize-fix.md` | Mixed historical/current; formerly unindexed | Archived; lower-range reference owns normalized start-height, bidirectional resize, max cap and applied-distance anchoring; `SplitterCollapseControllerTests` |
| `pitch-shifter-compact-map.md` | Mixed historical/current | Archived; built-in effect reference owns listener, fractional knob versus quantized graph, cancellation/undo/lifetime; `PitchShiftDragTests`; retain unvalidated automation/knob undo boundary |
| `reverb-header-simplification.md` | Historical paint-only rationale | Archived; user plugin reference retains title-only behavior; effect reference explains paint-only rationale/test limits |

## Root and newly introduced documents

| Page | Action / authority |
|---|---|
| Root `README.md` | Retain project entry/portable quick start; link contribution workflow |
| Root `Todo.md` | Now open queue; all software work/dependencies retained, workflow extracted, history linked at pre-migration revision |
| Root `CHANGELOG.md` | Retain user-visible release history; documentation reorganization adds no product release entry |
| Root `AGENTS.md` (local ignored instructions) | Unchanged; preserve project-local build/jobs/delivery instructions |
| `development/documentation-policy.md` | Adopted maintenance schema; durable, current |
| `development/documentation-reorganization-plan.md` | Active plan moved to `plans/documentation-reorganization-plan.md`; compatibility redirect retained |
| `development/contributing.md` | New durable workflow; preserves original approval/build/test/runtime/artifact/maintainer/commit gates, with explicit docs/tool scope |
| `components/song-editor.md` | New current arrangement contract; no longer hidden in Piano Roll documentation |
| `archive/README.md` | Historical index, all extracted-record destinations and evidence availability |
| `plans/README.md` | Active work/proposal index |
| `components/property-field-input.md` | Shared numeric interaction/formats with explicit note/clip parser differences and source/test boundaries |
| `components/lower-range.md` | Resize/collapse/activation authority extracted from two records |
| `components/computer-midi-keyboard.md` | Controller/layout/focus/alias/virtual-state authority, separate from destination routing |
| `components/directory-browser.md` | Navigation/domain-callback authority, separate from project intent and file lifecycle |
| `components/built-in-effect-editors.md` | Shared layout and EQ/Pitch/Reverb special-input contracts, separate from rack composition |
| `archive/changes/wine-bottles-validation.md` | Original `ad5912e` Wine document retained as an explicitly historical environment/result snapshot |
| This map and active plan | Implementation complete; keep active until final acceptance, then archive rather than maintain another permanent source map |

## Pilot validation snapshot

- At the pilot checkpoint, local link/anchor/index checks passed across 84 Markdown pages; current pages are reachable within two links from `docs/README.md`. All 18 checker regression tests pass, covering missing targets/anchors, malformed URLs, reference links, fragments, encoded paths, duplicate/Unicode headings, code examples, history/redirect exemptions and repository boundaries. Workflow configuration is added; a hosted GitHub Actions run has not occurred because these changes are not pushed.
- Native checked-in suites pass **16 transition + 53 working-area comparisons** against the existing `87399ac5…` executable with unchanged musical model records. The retained `1965c028…` pre-correction executable fails **12 working-area checks** with unchanged model. That older binary passes the transition suite; it is not the historical review baseline with 13 transition failures, and no new failing-before transition run is claimed.
- The final maintained-tool replay again passes all 69 checks with unchanged models. Artifacts remain local under `/tmp/nextstudio-docs-native-final/` and `/tmp/nextstudio-docs-native-working-before/`; these are not durable downloads. The procedure is now checked in and accepts a new output directory on another machine.
- `BUILD_JOBS=12 ./build.sh rd` and `BUILD_JOBS=12 ./test.sh rd` succeed (29/29 CTest targets). The project-local shared build/copy procedure was rerun; the unchanged application artifact still matches the tested binary. Tool syntax checks and staged/unstaged whitespace checks pass.
- No application behavior, state format, cursor policy, or C++ test implementation changed. No additional native scale/platform/action/undo certification is inferred.
- Reader paths: index → Piano Roll workflow; index → Song Editor control/implementation; index → shared snapping ownership; index → Testing → native cursor procedure; index → history → #90 design/validation. These paths no longer require the old task transcript.

## Remaining-topic review and final gate

- Pilot accepted with “sieht gut aus”; the remaining migration followed that review rather than bypassing it.
- Numeric input is shared without falsely unifying planner semantics, duration-zero handling or integer parsers. The source-confirmed pitch-name/display octave mismatch is documented, not fixed. Property velocity `1..127` differs from lane `0..127`.
- Project references no longer rely on a completed browser record or an obsolete second modal decision. Testing's removed request-adapter claims are replaced with actual lifecycle/workflow coverage and full-component gaps.
- Effect contracts preserve role-independent bypass/source alpha, native defaults, exact fractional graph undo and unvalidated continuous-knob/automation limits. User references retain controls rather than internal sizing constants/history.
- Wine commands use configurable isolated setup; the pre-migration environment/result text is preserved below its historical header. Current import guard and conditional repaint timer are described from source; no new Wine run is claimed.
- Local artifact/hash/screenshot references are either intentional debug-sandbox examples or labeled historical snapshots. Original availability outside the selected cursor evidence is not newly verified, and no bulk upload is claimed.
- All 19 open software issues remain in the queue; #84 remains next. No application source, engine module, musical state format or changelog changed, and no issue/label/milestone was modified.
- Final local documentation checks pass across **101 Markdown pages**, with all **18 checker regression tests** passing. Python/Node syntax and staged/unstaged whitespace checks pass. Original-body comparison confirms all eleven newly moved records and the Wine snapshot preserve their bodies apart from required link adjustments.
- The remaining batch changes prose/placement only; prior pilot build/native/shared-artifact results above are retained snapshots, not fresh platform/property-bar validation. No application rebuild is required for these prose changes.
- **Final gate at this snapshot:** maintainer review of the complete migration, then archival of this plan/map. No documentation commit/push or hosted CI run was authorized by pilot acceptance.

## Completion

The final review found two Markdown-checker issues: list indentation was mistaken for code, and parentheses inside quoted link titles affected destination balancing. Both were corrected with regression coverage. All 23 checker tests and the repository link/index check pass. The independent review replay also passed all 69 native cursor comparisons against the existing binary with unchanged musical models. The maintainer authorized commit/push; this plan/map are now archived with compatibility redirects. Current maintenance rules remain in the [documentation schema](../../development/documentation-policy.md).

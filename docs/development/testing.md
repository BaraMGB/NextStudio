# Testing NextStudio

- Type: procedure
- Audience: contributors
- Scope: current console, native and documentation validation

## Overview

NextStudio currently has focused console test executables for logic that can be isolated from the full GUI and audio engine. CTest registers and runs these executables.

The current suites are:

| CTest name | Executable/production area | Test source |
|---|---|---|
| `PositionDisplayHelpers` | position parsing and formatting | `App/tests/PositionDisplayTests.cpp` |
| `PluginChainLayout` | rack scroll limits and reorder destination indices | `App/tests/PluginChainLayoutTests.cpp` |
| `SplitterCollapseController` | lower-range splitter/collapse layout boundaries | `App/tests/SplitterCollapseControllerTests.cpp` |
| `ComputerMidiKeyboardLayout` | computer-key aliases, mapping and key-state transitions | `App/tests/ComputerMidiKeyboardLayoutTests.cpp` |
| `ClipFrameDrawing` | production normal/selected clip frames against independent float edge-band raster reference; adjoining fractional widths/origins, offscreen clips, both paint orders, all selection pairs, 100/125/150/200% scaling, legacy-rounding control and unchanged integer-coordinate appearance | `App/tests/ClipFrameDrawingTests.cpp` |
| `TimelineViewGeometry` | normalized shared zoom, interval transitions, anchor/fit policy, long/panned ranges, fractional raster scales, accumulated intent, and JUCE line-coverage images | `App/tests/TimelineViewGeometryTests.cpp` |
| `TimelineViewState` | production view setters/conversions with real Tracktion clips/notes, coordinate coincidence, deferred fit, restore/resize, independent views, variable tempo, unchanged musical state and undo isolation | `App/tests/TimelineViewStateTests.cpp` |
| `PluginMenuOrdering` | recursive stable, case-insensitive natural ordering of plug-in categories and entries, filtered-menu equivalence, and sidebar format/name ordering | `App/tests/PluginMenuOrderingTests.cpp` |
| `PluginBypassPresentation` | whole-subtree grayscale, RGB/ARGB alpha and source preservation, initial/default bypass states, undo/redo colour restoration, teardown, unchanged control enablement/hit testing and state, and status badge bounds/contrast | `App/tests/PluginBypassPresentationTests.cpp` |
| `PitchShiftDrag` | whole-semitone graph edits, persistence, single-step undo/redo with exact fractional restoration, balanced gestures, no-op/cancel/destruction, and unchanged continuous native range | `App/tests/PitchShiftDragTests.cpp` |
| `EffectEditorLayout` | responsive Compressor, Delay, and Pitch Shifter graph/control rectangles, minimum sizes, non-overlap, compact pitch width/size caps, and interval-map positions including fractional shifts and whole-semitone drag mapping | `App/tests/EffectEditorLayoutTests.cpp` |
| `MainInteractionState` | setup/project lock composition and foreground priority | `App/tests/MainInteractionStateTests.cpp` |
| `DebugProtocol` | command/JSON Lines parsing and serialization | `App/tests/DebugProtocolTests.cpp` |
| `DebugSnapshotWriter` | PNG success, decode validation, and failure paths | `App/tests/DebugSnapshotWriterTests.cpp` |
| `DebugStateFilter` | binary-like state filtering and bounded strings | `App/tests/DebugStateFilterTests.cpp` |
| `DebugSettingsIsolation` | explicit sandbox settings persistence | `App/tests/DebugSettingsIsolationTests.cpp` |
| `DebugAppController` | fake-host validation, readiness, artifacts, errors, and quit | `App/tests/DebugAppControllerTests.cpp` |
| `ProjectLifecycle` | extensions, validation, direct-save targets, load inspection, and exact save rollback snapshots | `App/tests/ProjectLifecycleTests.cpp` |
| `ProjectWorkflow` | pending operations, save-error cleanup, recovery confirmation, deferred-execution guards, cancellation, errors, and interaction-lock states | `App/tests/ProjectWorkflowTests.cpp` |
| `MidiNoteOverlap` | Piano Roll overlap clearing | `App/tests/MidiNoteOverlapTests.cpp` |
| `MidiPendingPaste` | provisional MIDI paste state machine | `App/tests/MidiPendingPasteTests.cpp` |
| `SelectionGestures` | independent Lasso/TimeRange state, floating musical anchor projection, velocity head mapping/resize/stem exclusion, retained keyboard clip scope/identity/teardown, reverse/empty rectangles, inclusive marker boundaries, snapshot-based replace/add/toggle/shrink policy, real Tracktion note identity resolution and selection-only model/undo isolation | `App/tests/SelectionGestureTests.cpp` |
| `PianoRollNoteLength` | inserted-note length modes, note values, finite fallbacks, and tick-only draw floor | `App/tests/PianoRollNoteLengthTests.cpp` |
| `TimelineSoftSnap` | continuous/monotonic physical-pixel curve, MIDI/Song attraction profiles and default MIDI profile selection, exact detents and detailed plateau classification, dense sweeps, narrow/unequal/long intervals, inverse anchors, bounded slope/displacement, invalid profiles, and non-idempotence | `App/tests/TimelineSoftSnapTests.cpp` |
| `TimelineSnapping` | production fixed/adaptive resolver, tempo ramps/meter/triplet boundaries (including engine bar-rounding failures), 100/125/150/200% physical scaling, MIDI Knife preview coordinate-type and grid raster-coverage regression, corrected downward creation anchors, production time-range/drop endpoint projection through tempo changes/ramps at four scales and panned/off-screen views, raw bypass/context transitions, relative Draw state, real offset-clip creation/overlap, fractional persistence and one-step undo/redo, note/clip/automation group constraints, held/free/bypass/limit/invalid feedback and reset, stationary Draw Shift reporting, queued JUCE TextEditor focus-loss completion/rejection and idempotence, whole-group move destinations including invalid secondary lanes and extreme offsets, shared clip preview ranges, theme-derived font tint and fractional lane-clipped guide/diamond raster rendering and JUCE sibling/header versus foreground ruler-cue ordering at four scales | `App/tests/TimelineSnappingTests.cpp` |
| `ClipOverwriteCommand` | incoming-wins clip placement, trimming, identity, selection, and undo | `App/tests/ClipOverwriteCommandTests.cpp` |
| `MidiInputRouting` | automatic default focus, default-route deduplication, always-focused virtual PC keyboard, persistent manual targets, pinning, migration, and atomic routing undo/redo | `App/tests/MidiInputRoutingTests.cpp` |
| `MetronomeSampleManager` | WAV validation, settings-relative managed copies, source independence, and role-specific cleanup | `App/tests/MetronomeSampleManagerTests.cpp` |
| `ThemePresetModel` | preset validation/lifecycle, color conversion, scalable preset-browser filtering, and mouse-wheel forwarding from the inline hex editor | `App/tests/ThemePresetModelTests.cpp` |
| `EqBandReset` | all four EQ bands, parameter-owned factory defaults, synchronous updates, and atomic undo/redo | `App/tests/EqBandResetTests.cpp` |

## Run all tests

The simplest workflow is:

```bash
./test.sh rd
```

Accepted build types are:

```bash
./test.sh d
./test.sh r
./test.sh rd
```

The script first calls `build.sh` and then runs CTest with failure output enabled.

Parallel build jobs may be customized for the current machine:

```bash
BUILD_JOBS=8 ./test.sh rd
```

## Run CTest directly

After building:

```bash
./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

Other configurations use their corresponding directory:

```bash
ctest --test-dir autobuild/Debug --output-on-failure
ctest --test-dir autobuild/Release --output-on-failure
```

Useful CTest commands:

```bash
# List tests without running them
ctest --test-dir autobuild/RelWithDebInfo -N

# Run one suite by name
ctest --test-dir autobuild/RelWithDebInfo -R PositionDisplayHelpers --output-on-failure

# Verbose output
ctest --test-dir autobuild/RelWithDebInfo -V

# Repeat until failure when investigating intermittent behavior
ctest --test-dir autobuild/RelWithDebInfo --repeat until-fail:20 --output-on-failure
```

## Timeline grid tests

The two timeline suites exercise the shared [view transform](../components/timeline-view-transform.md). Geometry tests sweep both nearest and conservative-fit policies through every rendered interval transition and the full interactive zoom range at several widths, meters, and raster scales. They also cover fits beyond the interactive upper limit and the extreme subpixel fallback. State regressions cover stale cached contexts on reopening, latest-fit replacement, pan/zoom cancellation, passive resize after a settled fit, and large range/clip fits surviving asynchronous context refresh and restore. State tests compile the production `EditViewState`, not a second copy of its coordinate formulas, and create real Tracktion clips/notes. `ClipFrameDrawingTests` additionally exercises the production outline helper over 960 adjoining-clip cases, comparing interior scanlines against independently derived float edge bands using an explicit software renderer. Bands are rasterized as a union to avoid a separate rectangle fast path's different 8-bit coverage rounding. Native-renderer checks instead verify physical position, thickness and selection colour with bounded tolerances and negative controls; they do not require exact pixel equality across different drawing primitives/renderers. Graphics contexts finish before native image readback. Grid translated-coverage comparisons likewise allow the implemented small channel tolerance. Complete clip body/content/frame compositing, gesture routing, drag previews and hit-testing still need focused runtime validation.

For visual regression checks, capture Song Editor and Piano Roll with snapped starts/ends and unsnapped examples, and record slow pan/zoom sequences. Compare same-rank grid lines rather than differently emphasized beat/bar lines. At fractional UI/display scaling, use a native desktop capture for physical-pixel comparisons; logical-resolution agent screenshots can have a different raster phase. `state-dump` includes `edit.timelines` for checking actual view scales and anchors.

Include exactly adjoining fractional-length clips, not just whole-beat examples: independent rounding of x and width can otherwise escape visual checks. Compare unselected, left-selected, right-selected, and both-selected pairs in the complete application, including a partly offscreen first clip. Preserve selection/geometry across before/after captures and confirm musical clip/note data remain unchanged after slow pan/zoom and UI-scale changes. A float outline removes the separate frame-coordinate rounding; it does not promise constant raw RGB samples across different underlying content/bands or eliminate antialiasing.

## Timeline cursor validation

The working-area contracts belong in [Piano Roll](../components/piano-roll-editor.md#tool-cursor-working-areas) and [Song Editor](../components/song-editor.md#cursor-working-areas-and-stationary-changes). These direct GUI assignments require real OS cursor checks, not a helper reproducing the policy.

`tools/native-cursor-regression.py` contains two maintained Linux/X11 procedures derived from the #90 scripts:

- **Working areas:** 53 cursor comparisons for MIDI tool clip/gap/empty-row/re-entry behavior, Lasso/Range empty space, Song Knife clip/gap and Master. Full musical clip/note records must remain unchanged.
- **Transitions:** 16 stationary Song Range-to-Pointer/Time-Stretch checks over gaps, clip bodies/both edges, selected-range bodies/both edges, and external Master. Toolbar focus/Return changes mode without moving the pointer; native reference cursors are obtained by real hover first.

Prerequisites: Python 3.10+, Node.js, `xdotool`, `libX11.so.6`, `libXfixes.so.3`, and Xvfb (`xvfb-run` also requires `xauth`). Use a **private 1600×1000 X display without window-manager decorations**, with a fresh 100% application/cursor-scale debug sandbox and no other NextStudio instance. The tool fixes window size/position and creates temporary model fixtures; never run it on a normal working desktop/session. Its fixed UI coordinates are an explicit fixture contract. Layout/theme defaults changing may require fixture updates, not cursor-policy changes.

From the repository root, after building:

```bash
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-cursor-regression.py \
  --output "./artifacts/native-cursor-$(date +%Y%m%d-%H%M%S)"
```

`--binary` selects another build, `--suite transitions|working-areas|all` selects scope, `--display` selects an already private display, and `--delay` adjusts event settling on slower machines. The output directory must not exist. It retains results, binary hash, actual XFixes cursor PNG/JSON, model dumps, fixture screenshots and bridge diagnostics; keep relevant results with an issue/CI artifact when a durable validation snapshot is needed. No Pillow, Python Xlib module, HTTP server or fixed port is required. The private JSON-lines bridge reuses `debug-shell-client.js`, verifies sandbox settings, runs suites serially and cleans up its own application process.

A nonzero exit indicates failed cursor/model checks or invalid prerequisites/fixture. Reference images must be visible and distinguish tool/body/edge states, preventing an all-normal-cursor fixture from passing. Against the retained pre-correction binary the working-area procedure detects the original 12 failures; the current binary passes. This does not create new cross-platform or fractional-scale certification.

Additional focused manual/runtime checks still cover active drag cursors, release/cancellation, stationary MIDI tool changes and pan, real Draw/Knife/Eraser actions and undo/redo. These are **not** automated by the two maintained suites. Native higher scaling, monitor transitions, other platforms and exhaustive automation/overlay cases require separately scoped runs. The [historical #90 record](../archive/changes/tool-cursor-working-areas.md) preserves the original 100/125% evidence and follow-up limits.

## Selection gesture validation

The [selection contract](../components/selection-gestures.md) owns independent Lasso/TimeRange state, musical anchors, source hit tests, snapshots and cancellation. `SelectionGestureTests` exercises the production geometry/policy helpers and real Tracktion MIDI selection adapter, including deleted/recreated note identity and changed clip sets, plus mouse-down ownership, replay identity and cancellation not consuming a subsequent edit release. Shared-manager tests restore multiple MIDI owners, controller membership and external clips, retain automation proxies, reject deleted point/event-owner and detached clip identities, and check model/undo isolation. Indexed policy tests preserve distinct equal-content/propertyless identities, rebuild after property-storage changes, and print 2,000/20,000-identical-content-object update timings without machine-dependent pass thresholds. It is not a substitute for mouse dispatch or rendered selection checks.

After building, use the same private X11/debug-shell prerequisites and undecorated 1600×1000, 100%-scale fixture as the cursor procedure:

```bash
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-selection-regression.py \
  --output "./artifacts/selection-$(date +%Y%m%d-%H%M%S)"
```

The maintained procedure verifies MIDI Pointer down-anchor preservation, lasso expand/shrink, Shift addition, Ctrl toggling, Escape/restoration/late events, a new MIDI press without tool reset after cancellation, independent MIDI Range, stationary-pointer zoom, arrangement object/range separation, and automation selection with the same snapshot policies. Shift/Ctrl are pressed and released after drag initiation without pointer movement in all three sources. Cross-editor Lasso/Range cancellation compares the complete shared selection before/after, including automation proxy restoration after asynchronous cleanup. It creates its automation lane/points through the real control/menu and mouse handlers, not through a second selector. Musical note/clip records and automation-point summaries must remain unchanged during selection. State dumps include MIDI membership and automation selection count; screenshots cover rectangle/range appearance. Retain relevant output as issue/CI evidence; local output alone is not durable certification.

`--binary`, `--display` and `--delay` control the isolated fixture. The output directory must not exist. The procedure imports the existing cursor test's maintained session/bridge and inherits its dependency and safety checks. Fixed UI coordinates depend on default layout/theme; fixture changes must be reviewed, not treated as selection-policy changes.

Separately validate active tool/track changes, source deletion/reordering, multiple automation lanes, vertical pan and fractional UI/display scaling. Native tests do not inject superseded-source mouse events directly into the production viewport; helper tests and explicit press-ownership dispatch guards cover that rejection contract. Unit/model coverage checks some of these identities/projections but does not prove full native routing. Other native platforms and physical displays require scoped runs.

### Velocity-lane source (#84)

```bash
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-velocity-selection-regression.py \
  --output "./artifacts/velocity-selection-$(date +%Y%m%d-%H%M%S)"
```

The same isolated session supports `--binary`, `--display` and `--delay`. Checks cover lane expansion/shrinking, live Shift/Ctrl transitions, toggling an initially selected note, Escape/shared restoration and late events, a subsequent press, reverse Pointer selection, head-versus-stem hits, explicit Lasso on a head, stationary-pointer zoom and unchanged complete musical records during selection. Subsequent selected-group and unselected-single-marker drags compare actual model changes and selection membership. Note-field text typed without Enter must commit to the original selection before Pointer/Lasso lane presses; complete model comparisons cover delayed callback idempotence, one-step undo/redo, retention after lasso cancellation, invalid-focus rejection, text Escape and marker hit/drag origins after completion. The pre-correction binary fails at `property-pointer-focus-commit`. Marker click checks cover deferred release, replace/add/toggle (including selected/empty membership), unchanged right clicks and complete musical-model isolation; the pre-click-fix binary fails at `marker-click-replace`. Existing group/single-drag checks must continue to pass. Screenshots record the lane rectangle and persistent selected-head outline; these are visual evidence, not automated pixel assertions. Unit tests additionally cover velocity projection at small/zero and multiple lane heights. Physical/fractional displays, other platforms, active source deletion and multi-clip combinations still need scoped native runs.

### Piano-key selection after note selection

```bash
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-keyboard-selection-regression.py \
  --output "./artifacts/keyboard-selection-$(date +%Y%m%d-%H%M%S)"
```

The same isolated session checks exact note identities after grid/velocity lassos, Range, empty membership and an unmatched pitch; repeated piano-key clicks and Shift pitch addition/removal must work without reselecting arrangement clips. An unrelated sibling is excluded until explicitly selected. Single- and multi-clip targets update on real arrangement clicks and survive subsequent note selection. Complete musical records remain unchanged. The delivered pre-fix binary fails at `keyboard-after-grid-lasso`. `SelectionGestureTests` additionally validates retained clip scope, explicit replacement, duplicate filtering, deleted/recreated identity rejection, track-scope filtering, teardown and model/undo isolation. Native clip deletion/track teardown, audio audition, physical/fractional displays and other platforms are not certified by this procedure.

## Arrangement drag commit validation

Selection tests do not certify the subsequent move/copy commit. `tools/native-arrangement-drag-regression.py` uses the same private X11/debug-shell fixture and prerequisites as the cursor procedure. Run native sessions sequentially, with no competing NextStudio instance.

```bash
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-arrangement-drag-regression.py \
  --output "./artifacts/drag-clean-$(date +%Y%m%d-%H%M%S)"
xvfb-run -a --server-args='-screen 0 1600x1000x24' \
  python3 tools/native-arrangement-drag-regression.py --existing-overlaps \
  --output "./artifacts/drag-overlaps-$(date +%Y%m%d-%H%M%S)"
```

Both variants expect committed clip/range moves and copies, stable MIDI contents and untouched legacy material. Checks use actual post-release model dumps, not just visible previews. The default variant uses clean tracks. The default variant also cancels a lasso before clip editing and cancels a replacement range before editing the restored interval. The second variant is a separate legacy-data diagnostic: it seeds an unrelated overlapping pair through the engine fixture helper to represent existing/imported project state. It still returns failure under the unchanged [global overwrite contract](../architecture/clip-overwrite-command.md). That diagnostic is not an established reproduction of the maintainer's universal drag regression and does not authorize changing the overwrite policy. Do not treat it as passing acceptance.

`--binary`, `--display`, `--delay` and the non-existing output directory follow the cursor procedure. Fixed coordinates assume default 100% layout/theme. Audio, vertical/group dragging, physical displays, native fractional scaling and Windows/macOS require separate scoped coverage.

## Live MIDI input and key lighting

Current contracts: [Computer MIDI keyboard](../components/computer-midi-keyboard.md), [MIDI routing](../architecture/midi-input-routing.md), and [Piano Roll lighting](../components/piano-roll-editor.md#live-midi-key-lighting). Use an isolated project/settings environment and record platform, input/device, displayed track and mapping.

1. Route physical/virtual input to the displayed track; hold/release one and several notes and confirm the expected keys light/clear.
2. Route input to another track and confirm the displayed keyboard does not react.
3. Rapidly drag across the visible keyboard, then release; no released key should remain lit.
4. Verify computer primary keys, configured upper-C alias, key repeat and multi-key passages, including focus leaving main/plugin roots and track/edit changes.
5. Enter/leave the project/setup lock with notes held and verify note release/current-state rebinding, not an old device reference.
6. Change theme and confirm active white/black keys use the current PrimeColour variants.
7. Clear/switch track and close the application/plugin window while input/audition is active; check cleanup without stuck notes or stale callbacks.

Lighting is binary routed **live input**, not velocity brightness or arrangement-playback visualization. Dispatcher note-offs-last is a batching policy, not reconstructed original event order. Layout/routing unit tests do not replace this native/device checklist. These are recommended checks, not a newly completed runtime run for the documentation migration.

## Documentation checks

```bash
python3 -m unittest discover -s tools/tests -p 'test_check_docs.py'
python3 tools/check-docs.py
```

These standard-library checks need no application build. They validate local file links, supported heading anchors, reference links and current index coverage; they do not fetch external URLs or treat historical/redirect pages as current entries. Fenced/indented examples and inline code are excluded from link checks. See [Contributing](contributing.md#documentation-only-changes-and-development-tools) for prerequisites/CI and the [maintenance schema](documentation-policy.md#7-responsibility-and-review-checklist) for manual review.

## Debug-system tests

The focused debug tests compile the same protocol, controller, and PNG writer used by the application. They cover legacy and JSON request parsing, malformed requests, aliases, adversarial JSON response values, standard escaping, response recognition, fake-host controller validation and error paths, state-string filtering/truncation, invalid images, successful encode/decode with dimensions, unopenable output, explicit session settings paths, and quit dispatch.

Full process behavior is covered by `tools/debug-shell-client.js`. Its modes validate transport, errors, state artifacts, settings isolation, repeated Windows-compatible stdin commands, deterministic track/clip/note/plugin editing, and protocol desynchronisation. See [Agent Debug System](../agent-debug.md) for commands and CI distribution.

## Clip overwrite tests

`ClipOverwriteCommandTests` creates real Tracktion edits and verifies selective
victim splitting, move identity, copy-on-self, block and cross-track moves,
multiple removal masks, winner validation, selection, and atomic undo/redo.
It also covers audio fades/takes/clip plug-ins, horizontal and vertical
automation, resize/time-stretch placement finalisation, multi-track time ranges,
commit rollback, frozen/bounds/duplicate-source validation, grouped copies,
arrangement recording policy, and a 200-clip bulk regression.

## Position display tests

`NextStudioTests` compiles:

- `App/src/PositionDisplayHelpers.cpp`;
- `App/tests/PositionDisplayTests.cpp`.

It links only the JUCE core and Tracktion core dependencies required by those helpers.

Current coverage includes:

- strict integer parsing, including overflow and trailing characters;
- strict finite floating-point parsing;
- BPM formatting;
- time-signature formatting and parsing;
- time display formatting;
- bars/beats/ticks parsing and formatting;
- abbreviated bars/beats input;
- invalid bars/beats/ticks component counts and ranges;
- clock-time parsing variants and invalid ranges;
- negative-time parsing and clamping;
- denominator index helpers.

The suite uses a small local `REQUIRE`/`REQUIRE_EQ` harness and returns a non-zero process status when failures occur.

## Project lifecycle tests

`ProjectLifecycleTests` compiles:

- `App/src/ProjectLifecycle.cpp`;
- `App/tests/ProjectLifecycleTests.cpp`.

It links JUCE core and data structures.

Current coverage includes:

- `.tracktionedit` normalization, repeated extension handling and case-insensitive recognition;
- exact existing-path preservation for direct Save and canonical new targets;
- project name/target validation and directory/persistent-file browser filtering;
- distinction between persistent and recovery files;
- save-target selection for Save and Save As;
- exact property rollback, including missing properties, duplicate captures and successful dismissal;
- inspection of missing, unsupported, empty, corrupt, and wrong-root files;
- acceptance of XML and binary `EDIT` state;
- context-sensitive acceptance of `.nextTemp` recovery files.

Temporary test files are created in a unique child of JUCE's temporary directory and removed by RAII cleanup. Typed pending-operation, Save/Discard/Cancel, failed-write continuation cleanup and deferred-execution guards belong to `ProjectWorkflowTests`, not the removed project-request adapter. Neither suite instantiates the complete sidebar/editor replacement path.

## Numeric property-field validation

[Property-field input](../components/property-field-input.md) owns formats and shared interaction rules; [NotePropertiesBar](../components/note-properties-bar.md) and [ClipPropertiesBar](../components/clip-properties-bar.md) own distinct planners/commit routing.

`PositionDisplayTests` tests shared production conversion/parsing helpers. `TimelineSnappingTests` tests the shared queued JUCE TextEditor completion path. Neither directly exercises every local note/clip duration/pitch parser, full bar focus handler or owner-installed model commit. Source inspection found the documented pitch-name/display octave mismatch; it was not fixed or certified by a new full-component runtime test in this migration.

For numeric changes, cover absolute/relative and mixed selection, zero/invalid/overflow input, clip offsets and tempo changes, focus-loss/Tab order, preview without model mutation, all-group rejection, scrub no-op, overlap/recreation/selection and one-step undo/redo. Recommend tests against production code, not copied parser/formula implementations.

## MidiNoteOverlap tests

`MidiNoteOverlapTests` compiles:

- `App/src/MidiNoteOverlap.cpp`;
- `App/tests/MidiNoteOverlapTests.cpp`.

The helper is pure (no Tracktion or JUCE dependency), so the executable links only JUCE core.

Current coverage includes:

- no intersection and touching boundaries;
- clear range fully containing the note;
- exact equality between note and clear range;
- splitting a note into two pieces;
- trimming the note end and note start;
- regression coverage that an inserted range wins against an overlapping right-hand note;
- multiple clear ranges producing multiple pieces;
- overlapping and adjacent clear ranges;
- clear ranges outside the note;
- sub-epsilon pieces being dropped;
- empty note and empty clear inputs;
- full coverage by several clear ranges;
- unsorted clear ranges.

This is the planning layer behind `MidiViewport::cleanUnderNoteRanges()`. The mutation layer (applying the plan to real `MidiClip`/`MidiNote` objects) is not exercised by this suite.

## MidiPendingPaste tests

`MidiPendingPasteTests` compiles:

- `App/src/MidiPendingPaste.cpp`;
- `App/tests/MidiPendingPasteTests.cpp`.

Current coverage includes:

- inactive commands producing no resolution;
- deselect without movement cancelling the preview;
- deselect after movement committing accumulated offsets;
- `Enter` committing positively with zero offset;
- `Enter` committing accumulated beat/pitch movement;
- `Escape` cancelling after movement;
- zero nudges not marking the state as moved;
- repeated `begin()` resetting previous offsets;
- moving away and back to the origin still counting as a positive edit.

This suite exercises the pure pending-paste state machine. Clipboard capture, preview rendering, clip resolution, destination cleanup, note creation, command routing, and undo integration remain covered by application compilation and focused manual testing.

## What is not covered yet

The C++ suites and limited native procedures above do not fully certify:

- complete GUI layout and mouse/keyboard interaction;
- full application-level project replacement;
- asynchronous autosave worker timing;
- audio-device configuration;
- real-time DSP behavior;
- external plug-in scanning and native editor windows;
- arrangement tools and Piano Roll tool gestures;
- complete `NotePropertiesBar` / `ClipPropertiesBar` parsing, planning and owner-installed model application;
- platform packaging.

These areas currently depend on compilation, manual testing, and runtime assertions. This is a coverage gap, not an indication that the behavior is unimportant.

## Choosing what to test

Prefer extracting deterministic logic into a helper when it can be tested without a GUI, audio device, or full engine. Good candidates include:

- string parsing and formatting;
- range and coordinate calculations;
- validation matrices;
- file-type and request-state rules;
- conversion between persistent representations;
- ordering and filtering;
- transaction-planning logic that can be separated from mutation.

Avoid duplicating production behavior inside a test. The test should call the same helper used by the component.

## Adding a test suite

### 1. Create a test source

Place it under `App/tests/`, for example:

```text
App/tests/MyFeatureTests.cpp
```

A console test must return `0` on success and non-zero on failure.

### 2. Isolate production logic

If possible, put non-GUI logic in a small header/source pair under `App/include/` and `App/src/`. Keep dependencies minimal so the test executable does not have to link the complete application.

### 3. Register the executable in CMake

Test sources are listed explicitly in `App/CMakeLists.txt`. A typical pattern is:

```cmake
juce_add_console_app(MyFeatureTests
        PRODUCT_NAME "MyFeatureTests")
juce_generate_juce_header(MyFeatureTests)

target_sources(MyFeatureTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MyFeature.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/MyFeatureTests.cpp)

target_include_directories(MyFeatureTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(MyFeatureTests PRIVATE
        juce::juce_core)

add_test(NAME MyFeature COMMAND MyFeatureTests)
```

Add only the libraries actually required by the production helper.

### 4. Reconfigure and run

```bash
./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo -R MyFeature --output-on-failure
```

## Test design guidelines

1. Test valid, boundary, and invalid inputs.
2. Test defaults and abbreviated forms when the UI accepts them.
3. For file tests, use isolated temporary directories and deterministic cleanup.
4. Assert the externally meaningful result, not private implementation details.
5. Include regression cases for every fixed bug that can be represented without excessive infrastructure.
6. Avoid dependence on locale, wall-clock timing, installed plug-ins, or physical audio devices in unit tests.
7. Keep test output concise but include the failed expression and source line.
8. Ensure a failed assertion affects the executable's exit code.
9. Run the test in Debug at least once when Tracktion/JUCE assertions are relevant.
10. Keep tests compatible with Linux, Windows, and macOS CI environments.

## Manual validation checklist

For UI or engine changes without automated coverage, record and execute a focused checklist. Depending on the subsystem, include:

- new project, load, Save, Save As, and cancelled chooser behavior;
- undo/redo boundaries;
- selection changes and object deletion;
- narrow and large window layouts;
- dark and light themes;
- project switch while a child editor is open;
- playback/recording state transitions;
- clean shutdown and crash-recovery behavior;
- Debug build assertions;
- at least one relevant platform-specific path.

For the arrangement overwrite feature, additionally verify mouse-driven move,
Ctrl-copy, resize, time stretch, MIDI double-click creation, audio drag/drop,
time-range duplication, and audio/MIDI recording while playback is active.

Manual validation should supplement rather than replace extractable unit tests.

## CI

The independent documentation workflow runs link/index checks and their regression tests for documentation/checker changes. Build/release CI skips changes limited to Markdown and checker infrastructure, but release tags/manual builds retain the full pipeline. Mixed application changes still run build validation. The native cursor suites are currently a focused local Linux/X11 procedure, not automatically part of CI.

GitHub Actions builds and runs CTest on Linux, Windows, and macOS. Linux additionally runs the complete debug-shell smoke suite under Xvfb; hosted Linux runners have no audio device/clock and immediately stop playback, so CI disables the sustained-playing and clock-advance assertions with `NEXTSTUDIO_REQUIRE_AUDIO_CLOCK=0` while retaining command-acknowledgement and final stopped-state checks. Windows runs redirected-stdin startup/repeated-command/EOF/quit smoke tests, and macOS runs the transport-client protocol regression. Floating-point assertions must use a tolerance appropriate to the production value type. Packaging follows successful tests. Local validation should still use the repository build command, CTest, and the relevant smoke mode before pushing logic changes.

## Related documents

- [Building](building.md)
- [Source Layout](source-layout.md)
- [Project Lifecycle](../architecture/project-lifecycle.md)
- [State and Event Model](../architecture/state-and-events.md)
- [Agent Debug System](../agent-debug.md)

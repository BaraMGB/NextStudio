# Testing NextStudio

## Overview

NextStudio currently has focused console test executables for logic that can be isolated from the full GUI and audio engine. CTest registers and runs these executables.

The current suites are:

| CTest name | Executable/production area | Test source |
|---|---|---|
| `PositionDisplayHelpers` | position parsing and formatting | `App/tests/PositionDisplayTests.cpp` |
| `PluginChainLayout` | rack scroll limits and reorder destination indices | `App/tests/PluginChainLayoutTests.cpp` |
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

The two timeline suites exercise the shared [view transform](../components/timeline-view-transform.md). Geometry tests sweep both nearest and conservative-fit policies through every rendered interval transition and the full interactive zoom range at several widths, meters, and raster scales. They also cover fits beyond the interactive upper limit and the extreme subpixel fallback. State regressions cover stale cached contexts on reopening, latest-fit replacement, pan/zoom cancellation, passive resize after a settled fit, and large range/clip fits surviving asynchronous context refresh and restore. State tests compile the production `EditViewState`, not a second copy of its coordinate formulas, and create real Tracktion clips/notes. `ClipFrameDrawingTests` additionally exercises the production outline helper over 960 adjoining-clip cases, comparing interior scanlines against independently derived float edge bands. Bands are rasterized as a union to avoid a separate rectangle fast path's different 8-bit coverage rounding. Complete clip body/content/frame compositing, gesture routing, drag previews and hit-testing still need focused runtime validation.

For visual regression checks, capture Song Editor and Piano Roll with snapped starts/ends and unsnapped examples, and record slow pan/zoom sequences. Compare same-rank grid lines rather than differently emphasized beat/bar lines. At fractional UI/display scaling, use a native desktop capture for physical-pixel comparisons; logical-resolution agent screenshots can have a different raster phase. `state-dump` includes `edit.timelines` for checking actual view scales and anchors.

Include exactly adjoining fractional-length clips, not just whole-beat examples: independent rounding of x and width can otherwise escape visual checks. Compare unselected, left-selected, right-selected, and both-selected pairs in the complete application, including a partly offscreen first clip. Preserve selection/geometry across before/after captures and confirm musical clip/note data remain unchanged after slow pan/zoom and UI-scale changes. A float outline removes the separate frame-coordinate rounding; it does not promise constant raw RGB samples across different underlying content/bands or eliminate antialiasing.

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

- the save/discard/cancel decision matrix;
- `.tracktionedit` normalization and case-insensitive recognition;
- distinction between persistent and recovery files;
- save-target selection for Save and Save As;
- project request state, consumption, cancellation, and stale-request prevention;
- rejection of missing or unsupported load requests;
- inspection of missing, unsupported, empty, corrupt, and wrong-root files;
- acceptance of XML and binary `EDIT` state;
- context-sensitive acceptance of `.nextTemp` recovery files.

Temporary test files are created in a unique child of JUCE's temporary directory and removed by RAII cleanup.

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

The current tests do not directly exercise:

- GUI layout and mouse/keyboard interaction;
- full application-level project replacement;
- asynchronous autosave worker timing;
- audio-device configuration;
- real-time DSP behavior;
- external plug-in scanning and native editor windows;
- arrangement tools and Piano Roll tool gestures;
- `NotePropertiesBar` application against real `MidiClip`/`MidiNote` objects;
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

GitHub Actions builds and runs CTest on Linux, Windows, and macOS. Linux additionally runs the complete debug-shell smoke suite under Xvfb; hosted Linux runners have no audio device/clock and immediately stop playback, so CI disables the sustained-playing and clock-advance assertions with `NEXTSTUDIO_REQUIRE_AUDIO_CLOCK=0` while retaining command-acknowledgement and final stopped-state checks. Windows runs redirected-stdin startup/repeated-command/EOF/quit smoke tests, and macOS runs the transport-client protocol regression. Floating-point assertions must use a tolerance appropriate to the production value type. Packaging follows successful tests. Local validation should still use the repository build command, CTest, and the relevant smoke mode before pushing logic changes.

## Related documents

- [Building](building.md)
- [Source Layout](source-layout.md)
- [Project Lifecycle](../architecture/project-lifecycle.md)
- [State and Event Model](../architecture/state-and-events.md)
- [Agent Debug System](../agent-debug.md)

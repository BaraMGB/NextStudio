# Testing NextStudio

## Overview

NextStudio currently has focused console test executables for logic that can be isolated from the full GUI and audio engine. CTest registers and runs these executables.

The current suites are:

| CTest name | Executable/production area | Test source |
|---|---|---|
| `PositionDisplayHelpers` | position parsing and formatting | `App/tests/PositionDisplayTests.cpp` |
| `PluginChainLayout` | rack scroll limits and reorder destination indices | `App/tests/PluginChainLayoutTests.cpp` |
| `DebugProtocol` | command/JSON Lines parsing and serialization | `App/tests/DebugProtocolTests.cpp` |
| `DebugSnapshotWriter` | PNG success, decode validation, and failure paths | `App/tests/DebugSnapshotWriterTests.cpp` |
| `DebugStateFilter` | binary-like state filtering and bounded strings | `App/tests/DebugStateFilterTests.cpp` |
| `DebugSettingsIsolation` | explicit sandbox settings persistence | `App/tests/DebugSettingsIsolationTests.cpp` |
| `DebugAppController` | fake-host validation, readiness, artifacts, errors, and quit | `App/tests/DebugAppControllerTests.cpp` |
| `ProjectLifecycle` | project request, extension, validation, and unsaved-choice rules | `App/tests/ProjectLifecycleTests.cpp` |
| `MidiNoteOverlap` | Piano Roll overlap clearing | `App/tests/MidiNoteOverlapTests.cpp` |
| `MidiPendingPaste` | provisional MIDI paste state machine | `App/tests/MidiPendingPasteTests.cpp` |
| `PianoRollNoteLength` | inserted-note length modes, note values, fallbacks, and draw minimum | `App/tests/PianoRollNoteLengthTests.cpp` |
| `ClipOverwriteCommand` | incoming-wins clip placement, trimming, identity, selection, and undo | `App/tests/ClipOverwriteCommandTests.cpp` |

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

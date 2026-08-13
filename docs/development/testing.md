# Testing NextStudio

## Overview

NextStudio currently has focused console test executables for logic that can be isolated from the full GUI and audio engine. CTest registers and runs these executables.

The current suites are:

| CTest name | Executable/production area | Test source |
|---|---|---|
| `PositionDisplayHelpers` | position parsing and formatting | `App/tests/PositionDisplayTests.cpp` |
| `ProjectLifecycle` | project request, extension, validation, and unsaved-choice rules | `App/tests/ProjectLifecycleTests.cpp` |

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

## What is not covered yet

The current tests do not directly exercise:

- GUI layout and mouse/keyboard interaction;
- full Tracktion edit construction and project replacement;
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

Manual validation should supplement rather than replace extractable unit tests.

## CI

GitHub Actions builds and packages the application on Linux, Windows, and macOS. At the time of this document, the workflow's primary job is build/package validation; contributors should not assume that local CTest coverage is automatically equivalent to every CI job. Always run the test script before pushing logic changes.

## Related documents

- [Building](building.md)
- [Source Layout](source-layout.md)
- [Project Lifecycle](../architecture/project-lifecycle.md)
- [State and Event Model](../architecture/state-and-events.md)

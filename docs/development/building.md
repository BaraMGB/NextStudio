# Building NextStudio

## Overview

NextStudio uses CMake and builds as a JUCE GUI application linked with Tracktion Engine. The project requires a C++20 compiler. Commands in this document are run from the repository root.

The supported build configurations are:

| Script argument | CMake configuration | Intended use |
|---|---|---|
| `d` | `Debug` | assertions, debugging, fastest compile/least optimization |
| `r` | `Release` | optimized binaries and packaging |
| `rd` | `RelWithDebInfo` | optimized build with debug information |

## Clone and initialize dependencies

Clone the repository and initialize all submodules:

```bash
git clone https://github.com/BaraMGB/NextStudio.git
cd NextStudio
./fetch_submodules.sh
```

Equivalent Git commands are:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

The helper temporarily maps GitHub SSH URLs to HTTPS and restores the global Git configuration when it exits.

### Submodules

| Path | Purpose | Required behavior |
|---|---|---|
| `modules/tracktion_engine` | Tracktion Engine and JUCE | required |
| `modules/tinysoundfont` | SoundFont playback | required by include path and SoundFont feature |
| `modules/rubberband` | Rubber Band time stretching | optional at CMake feature-detection level |

When Rubber Band exists, CMake enables `TRACKTION_ENABLE_TIMESTRETCH_RUBBERBAND` and `TRACKTION_BUILD_RUBBERBAND`. Otherwise the project still enables SoundTouch time stretching.

## Linux prerequisites

For Debian/Ubuntu-based systems:

```bash
sudo apt update
sudo apt install build-essential cmake git \
  libasound2-dev libjack-dev libfreetype6-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libgl1-mesa-dev libxcomposite-dev libfontconfig1-dev \
  libcurl4-openssl-dev libwebkit2gtk-4.1-dev libgtk-3-dev \
  ladspa-sdk
```

The exact package names vary by distribution. The build enables ALSA and JACK on Linux and links `libatomic`.

## Preferred build script

Build with:

```bash
./build.sh rd
```

The script:

1. maps `d`, `r`, or `rd` to a CMake build type;
2. creates `autobuild/<Configuration>`;
3. configures CMake with compile-command export enabled;
4. builds the configured tree;
5. moves `compile_commands.json` to the repository root.

### Parallel jobs

The script defaults to two jobs for portability. Override this for the current machine with:

```bash
BUILD_JOBS=8 ./build.sh rd
```

`BUILD_JOBS` is a tuning parameter, not a project requirement. Choose a value appropriate for available CPU and memory.

### Clean CMake cache

To remove the configuration cache and `CMakeFiles` before rebuilding:

```bash
./build.sh rd -clean
```

This does not delete the entire build directory or every generated artifact; it forces CMake to reconfigure the important cached state.

## Build and run helper

`start.sh` wraps `build.sh` and optionally starts the resulting application:

```bash
./start.sh d
./start.sh r
./start.sh rd
```

Options:

| Option | Meaning |
|---|---|
| `-clean` | clean the CMake cache before building |
| `-build` | build only; do not launch |
| `-debug` | launch under `gdb` on non-macOS or `lldb` on macOS |
| `-h`, `--help` | print usage |

Examples:

```bash
./start.sh d -clean
./start.sh r -build
./start.sh d -debug
```

On macOS the helper opens the application bundle. On other supported desktop systems it executes the generated binary directly.

## Output locations

The build script writes configuration-specific trees under `autobuild/`.

Typical Linux executable paths:

```text
autobuild/Debug/App/NextStudio_artefacts/Debug/NextStudio
autobuild/Release/App/NextStudio_artefacts/Release/NextStudio
autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio
```

JUCE and CMake may use platform-specific bundle or multi-configuration layouts on macOS and Windows.

## Manual CMake build

The scripts are the documented local workflow, but a standard out-of-tree CMake build is also possible:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j 4
```

For multi-configuration generators:

```bash
cmake -S . -B build
cmake --build build --config Release -j 4
```

Tests are enabled by the root `include(CTest)` unless configured with `-DBUILD_TESTING=OFF`.

## Important CMake behavior

### Language standard

`App/CMakeLists.txt` requires C++20 with compiler extensions disabled.

### Source registration

Application implementation files are discovered with:

```cmake
file(GLOB NEXTSTUDIO_SOURCES src/*.cpp)
```

A new `.cpp` file under `App/src/` is therefore picked up when CMake reconfigures. Headers under `App/include/` are found through the target include directory.

Tests are different: test source files and their production helper sources are listed explicitly in `App/CMakeLists.txt`.

### Host formats

Compile definitions enable hosting for:

- VST3;
- LADSPA;
- LV2;
- Audio Unit.

Platform support and discovery still depend on the operating system, installed plug-ins, and available framework support.

### Audio backends

- Windows: WASAPI enabled; ASIO and JACK disabled in the current configuration.
- macOS: JACK disabled.
- Linux: ALSA and JACK enabled.

### Embedded resources

`App/resources/CMakeLists.txt` creates JUCE binary-data targets for icons, themes, and bundled drum-machine samples. Adding an icon requires listing it explicitly. Themes and sample folders are globbed at configure time.

## IDE and clangd setup

CMake exports `compile_commands.json`; `build.sh` places it at the repository root. This is sufficient for many editors and language servers.

`gen_clangd.sh` derives the compiler and system include paths from `compile_commands.json` and writes a `.clangd` file:

```bash
./build.sh rd
./gen_clangd.sh
```

The generated `.clangd` is machine-specific and should be reviewed before committing.

The repository also contains `.clang-format`. The code style is LLVM-derived with four-space indentation, no tabs, Allman braces, right-aligned pointer/reference symbols, and effectively unrestricted line length.

## Packaging

### CPack

The root CMake configuration installs the application and configures CPack generators:

- Linux: DEB, RPM, and TGZ;
- Windows: NSIS;
- macOS: DragNDrop.

After a Release build, packaging can be invoked from the build directory, for example:

```bash
cd autobuild/Release
cpack -C Release
```

Generator availability depends on platform tooling.

### Windows helper

`package_win.sh` configures `build_win_pkg`, builds Release, and invokes CPack. It assumes a Git Bash-compatible shell and a working Windows CMake generator. NSIS must be installed for the installer generator.

The helper currently hardcodes 12 build jobs; this is packaging-script behavior, not a general build requirement.

### Flatpak helper

`build_flatpak.sh` uses:

- `com.nextstudio.NextStudio.json`;
- `flatpak`;
- `flatpak-builder`;
- optionally `jq` for runtime discovery.

It creates a local repository and bundle below `dist/`, using `flatpak-build/` as a temporary build directory. It may reuse an already installed matching app ref.

### GitHub Actions

`.github/workflows/build.yml` builds Linux x64, Windows x64, and macOS arm64 on pushes, pull requests, tags, and manual dispatch. It packages platform artifacts and creates a GitHub release for `v*` tags.

## Common problems

### Missing Tracktion/JUCE CMake paths

Symptom: CMake cannot find `modules/tracktion_engine/modules/juce` or Tracktion targets.

Fix:

```bash
./fetch_submodules.sh
```

### Rubber Band not detected

CMake prints whether Rubber Band was found. Confirm the submodule exists:

```bash
ls modules/rubberband
```

Then clean/reconfigure:

```bash
./build.sh rd -clean
```

### Linux headers or libraries missing

Read the first CMake/linker error and install the corresponding development package. Audio and desktop integration require more than a minimal C++ toolchain.

### Stale generated configuration

Use:

```bash
./build.sh rd -clean
```

If necessary, remove only the affected configuration directory and rebuild:

```bash
rm -rf autobuild/RelWithDebInfo
./build.sh rd
```

### Build killed by the operating system

Reduce parallelism:

```bash
BUILD_JOBS=1 ./build.sh rd
```

## Validation after changes

At minimum:

```bash
./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

Or use the combined helper:

```bash
./test.sh rd
```

See [Testing](testing.md) for coverage and test-extension guidance.

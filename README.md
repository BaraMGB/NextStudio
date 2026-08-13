# NextStudio - Digital Audio Workstation (DAW)

NextStudio is a Digital Audio Workstation designed for music production, recording, editing, and mixing. Built with JUCE and Tracktion Engine.

## Community

Join the NextStudio community on Discord: [https://discord.gg/yyRzDUhbEB](https://discord.gg/yyRzDUhbEB)

- Get help with issues and bugs
- Share your music made with NextStudio
- Request features and discuss ideas
- Get notified about new releases (Alpha Tester role)

## Features

- **Multi-Track Recording:** Record multiple audio tracks simultaneously
- **Audio Editing:** Trim, split, merge, and rearrange recordings
- **Virtual Instruments:** Built-in synthesizers and samplers
- **Plugin Support:** VST3, AU, LV2, and LADSPA plugins
- **MIDI Support:** MIDI controller integration and recording
- **Audio Effects:** EQ, compression, reverb, delay, and more

## Download

Download installers from [GitHub Releases](https://github.com/BaraMGB/NextStudio/releases):

| Platform | Installer |
|----------|-----------|
| **Windows** | `.exe` (NSIS Installer) |
| **macOS** | `.dmg` or Homebrew |
| **Linux** | `.deb` or `.tar.gz` |

### macOS (Homebrew)

```bash
brew tap BaraMGB/tap
brew install --cask nextstudio
```

### macOS Installation

The DMG is not code-signed. On first launch, macOS will show a security warning.

**Workaround:**
1. Open the DMG and drag NextStudio to Applications
2. Right-click NextStudio in Applications → "Open"
3. Click "Open" in the dialog

Or via Terminal:
```bash
xattr -d com.apple.quarantine /Applications/NextStudio.app
```

## System Requirements

- **Linux:** x86_64, ALSA or JACK audio
- **Windows:** 10/11, x64
- **macOS:** 10.13 or later, Apple Silicon or Intel

## Building from Source

### Prerequisites

```bash
git clone https://github.com/BaraMGB/NextStudio
cd NextStudio
./fetch_submodules.sh
```

### Dependencies (Linux)

```bash
sudo apt install libasound2-dev libjack-dev libfreetype6-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libgl1-mesa-dev libxcomposite-dev libfontconfig1-dev \
  libcurl4-openssl-dev libwebkit2gtk-4.1-dev libgtk-3-dev ladspa-sdk
```

### Build

```bash
./build.sh r
```

The script defaults to a portable job count. Parallelism can be adjusted for the current machine:

```bash
BUILD_JOBS=8 ./build.sh r
```

Build and launch with `start.sh`:

```bash
./start.sh d        # Debug build and run
./start.sh r        # Release build and run
./start.sh rd       # RelWithDebInfo build and run
./start.sh d -clean # Clean CMake cache, build, and run
./start.sh d -debug # Build and run under a debugger
```

Run the test suites with:

```bash
./test.sh rd
```

See the detailed [building](docs/development/building.md) and [testing](docs/development/testing.md) guides.

## Documentation

The documentation index includes user guides, architecture, component documentation, and development workflows:

- [NextStudio documentation](docs/README.md)
- [Getting started](docs/user/getting-started.md)
- [Piano Roll](docs/user/piano-roll.md)
- [Architecture overview](docs/architecture/overview.md)

## Development

- **Issues:** [GitHub Issues](https://github.com/BaraMGB/NextStudio/issues)
- **Source layout:** [docs/development/source-layout.md](docs/development/source-layout.md)
- **License:** AGPL3

## Acknowledgements

Built with [JUCE](https://juce.com) and [Tracktion Engine](https://github.com/Tracktion/tracktion_engine).

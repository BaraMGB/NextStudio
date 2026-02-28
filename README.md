# NextStudio - Digital Audio Workstation (DAW)

NextStudio is a Digital Audio Workstation designed for music production, recording, editing, and mixing. Built with JUCE and Tracktion Engine.

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
| **macOS** | `.dmg` |
| **Linux** | `.deb` or `.tar.gz` |

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
# Quick start
./start.sh r

# Or manual build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j4
```

### Build Options

```bash
./start.sh d        # Debug build
./start.sh r        # Release build
./start.sh d -clean # Clean build
./start.sh d -debug # Run with debugger
```

## Development

- **Issues:** [GitHub Issues](https://github.com/BaraMGB/NextStudio/issues)
- **License:** AGPL3

## Acknowledgements

Built with [JUCE](https://juce.com) and [Tracktion Engine](https://github.com/Tracktion/tracktion_engine).

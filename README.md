# NextStudio - Digital Audio Workstation (DAW)

NextStudio is a powerful Digital Audio Workstation (DAW) designed for music production, recording, editing, and mixing. With a comprehensive set of features and a user-friendly interface, NextStudio provides musicians, producers, and audio engineers with the tools they need to create music.

## Features

- **Multi-Track Recording:** NextStudio allows you to record multiple audio tracks simultaneously, giving you the flexibility to capture performances from various instruments and sources.

- **Audio Editing:** With NextStudio, you can edit audio clips with precision. Trim, split, merge, and rearrange your recordings to achieve the desired composition.

- **Virtual Instruments:** NextStudio includes a wide range of virtual instruments, such as synthesizers, samplers, and drum machines. Also NextStudio supports VST3, AU, LV2 and LADSPA Plugins.

- **MIDI Support:** NextStudio seamlessly integrates with MIDI controllers, allowing you to control virtual instruments and record MIDI data.

- **Audio Effects and Plugins:** Enhance your audio with a variety of built-in audio effects, including EQ, compression, reverb, delay, and more. Additionally, NextStudio supports third-party plugins, expanding your creative possibilities.

## System Requirements

To run NextStudio, ensure that your system meets the following requirements:

- Operating System: Linux (compatible distributions), Windows 10/11, macOS 10.13 or later
- MIDI Controller (optional): For MIDI input and control

## Installation and Building

### Prerequisites

1. Clone the NextStudio repository from the official GitLab page:

```shell
git clone https://gitlab.com/BaraMGB/NextStudio

    Navigate to the cloned repository:

cd NextStudio

    Fetch Tracktion Engine and JUCE submodules:

# Linux/macOS:
chmod +x fetch_submodules.sh
./fetch_submodules.sh

# Windows:
fetch_submodules.bat
```
## Quick Start (Linux/macOS)

For the fastest and easiest build experience on Linux and macOS, use the provided start script:

### Make the script executable (first time only)
    chmod +x start.sh

### Build and run debug version
    ./start.sh d

### Build and run release version
    ./start.sh r

### Clean build and run
    ./start.sh d -clean

### Build only (don't run)
    ./start.sh r -build

### Build and run with debugger
    ./start.sh d -debug

### Start Script Options

    Build Types:
        d - Debug build
        r - Release build
        rd - RelWithDebInfo build

    Options:
        -clean - Clean CMake cache before building
        -build - Build only, don't run the application
        -debug - Run with debugger (gdb on Linux, lldb on macOS)

## Manual Build (All Platforms)

If you prefer manual control or are on Windows, follow these steps:

Create a build directory and navigate into it:

    mkdir build && cd build

## Generate the build files using CMake:

Debug build

    cmake -DCMAKE_BUILD_TYPE=Debug ..

Release build

    cmake -DCMAKE_BUILD_TYPE=Release ..

## Build NextStudio:

Linux/macOS

    cmake --build . -j4

Windows (Visual Studio)

    cmake --build . --config Release

The executable will be located at:

        Linux: build/App/NextStudio_artefacts/[BuildType]/NextStudio
        macOS: build/App/NextStudio_artefacts/[BuildType]/NextStudio.app
        Windows: build/App/NextStudio_artefacts/[BuildType]/NextStudio.exe

# Development Workflow

For developers working on NextStudio:

### Quick debug build and test
./start.sh d -clean

### Release build for testing performance
./start.sh r

### Debug with debugger attached
./start.sh d -debug

### Build only for CI/testing
./start.sh r -build

# Getting Started

After building, run NextStudio using the start script or execute the binary directly.
Configure your audio and MIDI settings in the preferences menu according to your system's hardware setup.
You're now ready to start using NextStudio for your music production needs!

Note: Make sure you have all the necessary dependencies installed and configured correctly before building NextStudio with CMake.
Support and Documentation

If you encounter any issues or have questions about NextStudio, please file an issue on GitLab.

# License

NextStudio is available under the AGPL3. Please review the license file included with the software for more information.
Acknowledgements

We would like to express our gratitude to the open-source community for their valuable contributions to the technologies and libraries used in NextStudio.

Thank you for choosing NextStudio as your Digital Audio Workstation! We hope it empowers you to unleash your creativity and produce incredible music. Happy composing!

# NextStudio Project

NextStudio is a C++ based Digital Audio Workstation (DAW).

## Important Commands

### Build
The project is built with CMake. The `build.sh` script can be used for automation:

```bash
# For a debug build
bash build.sh d

# For a release build
bash build.sh r

# For a release with debug info build
bash build.sh rd
```

The script also accepts a `-clean` argument to remove the CMake cache.

Alternatively, the manual CMake steps are:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo]
cmake --build build --config [Debug|Release|RelWithDebInfo] -j 12
```

### Code Formatting
The project uses `clang-format`. To format a file:
```bash
clang-format -i path/to/file.cpp
```

### Testing
There are currently no specific test commands defined in the `.gitlab-ci.yml`.

## Project Structure
The user-authored project code is contained entirely within the `App/` directory. The `modules/` directory contains external dependencies (like the Tracktion Engine and JUCE framework) which are treated as immutable libraries and should not be modified. They can be searched for context if needed.

- **App/**: Contains all the user-editable source code, resources, and project files.
- **App/Source/Main.cpp**: The main entry point of the application.
- **App/resources**: SVG icons and other UI assets.
- **modules/tracktion_engine**: The core engine as a Git submodule. This module recursively includes the JUCE framework. It is a dependency and should not be modified.
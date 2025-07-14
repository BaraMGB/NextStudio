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

## How AutomationLanes Work in NextStudio
Basic Architecture

AutomationLanes are visual components that control the AutomatableParameters of plugins/tracks over time. They are built using the Tracktion Engine Framework and use JUCE for the UI.
Core Components

1. AutomationLaneComponent

    Main Visualization: Draws the automation curve using points and lines
    Parameter Control: Manages a single te::AutomatableParameter
    Interaction: Allows adding, moving, and editing of automation points
    Caching: Uses CachedCurvePoint for efficient rendering

2. TrackLaneComponent

    Container: Holds multiple AutomationLanes for a track
    Layout: Positions AutomationLanes below the main track area
    Management: Creates AutomationLanes based on existing automation data

3. TrackHeightManager

    Height Management: Stores and manages the heights of all AutomationLanes
    State Persistence: Saves AutomationLane heights in the Edit-State
    Dynamic Adjustment: Adapts heights based on user interaction

Data Flow and Interaction

Creating an AutomationLane

    Trigger: Right-click on an AutomatableSlider → “Add automation lane”
    Parameter Detection: A te::AutomatableParameter is identified
    Lane Creation: TrackLaneComponent::buildAutomationLanes() creates a new AutomationLane
    Persistence: Height and visibility are stored in the TrackHeightManager

Visualization

    Timeline: X-axis represents time (beats/seconds)
    Value Range: Y-axis represents the parameter value (0.0–1.0 or a specific range)
    Curves: Draws smooth curves between points using Bezier handles
    Points: Rendered as circles that can be moved

Interaction Mechanisms

    Point Selection: Via SelectableAutomationPoint in the SelectionManager
    Mouse Handling: SongEditorView processes mouse events for automation
    Drag & Drop: Move points and time ranges
    Context Menus: Right-click menus for additional actions

Technical Details

Coordinate Transformation

// Time to X position
timeToX(timeInSeconds, width, startBeat, endBeat)

// Value to Y position
getYPos(parameterValue) // Maps parameter value to vertical position

Curve Calculation

    Bezier Curves: Uses quadratic Bezier curves for smooth transitions
    Control Points: Calculated based on curve parameter (curve value)
    Caching: Points are cached for performance

State Management

    Edit-State: Stores automation data in the Tracktion Edit
    View-State: Manages visible parameters and their heights
    Selection: Manages selected automation points

Integration with Tracktion Engine

AutomationLanes are deeply integrated into the Tracktion Engine:

    AutomatableParameter: The main object for automatable parameters
    AutomationCurve: Stores the actual point data
    Edit-State: Persists automation data within the project

Extensibility

The system is designed to automatically detect new parameters and create corresponding AutomationLanes as needed. The height management allows flexible adaptation of the UI layout.

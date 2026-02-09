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

## Tracktion Engine Audio System Components

### TransportControl and Playback Context Management

The TransportControl is the central component managing audio playback and recording in the Tracktion Engine. It handles the creation and management of the audio playback context.

#### ensureContextAllocated()

The `edit->getTransport().ensureContextAllocated()` method is a critical setup step that ensures the playback context is properly initialized before audio operations begin.

**Purpose:**
- Allocates the EditPlaybackContext if it doesn't exist
- Creates the audio node graph required for playback
- Must be called before starting playback or recording

**Implementation Details:**
```cpp
void TransportControl::ensureContextAllocated (bool alwaysReallocate)
{
    if (! edit.shouldPlay())
        return;

    const auto start = position.get();

    if (playbackContext == nullptr)
    {
        playbackContext = std::make_unique<EditPlaybackContext> (*this);
        playbackContext->createPlayAudioNodes (start);
        transportState->playbackContextAllocation = transportState->playbackContextAllocation + 1;
    }

    if (alwaysReallocate)
        playbackContext->createPlayAudioNodes (start);
    else
        playbackContext->createPlayAudioNodesIfNeeded (start);
}
```

**When to Use:**
- Before starting playback: `m_edit->getTransport().ensureContextAllocated(); m_edit->restartPlayback();`
- After structural changes to the Edit
- When configuring input/output devices
- Before enabling input monitoring

### InputDevice Monitor Modes

InputDevices in the Tracktion Engine support three monitor modes that control when live input is audible through the system.

#### MonitorMode Enum

```cpp
enum class MonitorMode
{
    off,        ///< Live input is never audible
    automatic,  ///< Live input is audible when record is enabled
    on          ///< Live input is always audible
};
```

#### MonitorMode::automatic Behavior

The `automatic` mode is the most commonly used and provides intelligent monitoring behavior:

**Logic:**
```cpp
switch (owner.getMonitorMode())
{
    case InputDevice::MonitorMode::on:          return true;
    case InputDevice::MonitorMode::automatic:   return isRecordingEnabled (t.itemID);
    case InputDevice::MonitorMode::off:         return false;
};
```

**Behavior:**
- **off**: Live input is completely muted - no monitoring regardless of record state
- **automatic**: Live input is only audible when the target track is "armed" (record-enabled)
- **on**: Live input is always audible regardless of record state

**Benefits of automatic mode:**
- Prevents unwanted feedback when not recording
- Optimizes CPU usage by enabling monitoring only when needed
- Provides intuitive workflow - you hear input only when you're ready to record
- Default mode for most input devices in examples and demos

#### Track Arming and Record Enabling

In NextStudio, tracks are "armed" for recording through the UI, which internally calls the record enabling mechanism:

**UI Implementation:**
```cpp
m_armButton.onClick = [this, audioTrack]
{
    EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
    m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), juce::dontSendNotification);
};
```

**Engine Implementation:**
```cpp
void EngineHelpers::armTrack (te::AudioTrack& t, bool arm, int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
    {
        if (te::isOnTargetTrack (*instance, t, position))
        {
            instance->setRecordingEnabled (t.itemID, arm);
        }
    }
}
```

**Visual Feedback:**
- Red "A" button in track header
- Component ID: "arm" with firebrick color
- Toggle state reflects current arm status

#### Typical Usage Pattern

```cpp
// Setup input devices for recording
for (auto& midiIn : engine.getDeviceManager().getMidiInDevices())
{
    midiIn->setMonitorMode (te::InputDevice::MonitorMode::automatic);
    midiIn->setEnabled (true);
}

// Ensure playback context is ready
edit->getTransport().ensureContextAllocated();

// Now when user arms a track, they will hear the input
// When they disarm the track, monitoring stops automatically
```

This system provides a professional DAW workflow where monitoring is intelligently managed based on the user's recording intentions.

## Tracktion Engine I/O System Architecture

### Two-Phase I/O Setup

The Tracktion Engine implements a sophisticated two-phase approach to audio I/O configuration:

1. **Programmatic Initialization**: When the application starts, code must initialize the `DeviceManager` with a default, functioning configuration. This is a non-interactive "bootstrap" to ensure the engine can make sound immediately.
2. **User-Driven Configuration**: After initialization, users can change audio settings (audio interface selection, buffer size, etc.) through a graphical interface.

### Phase 1: Programmatic Initialization with EngineBehaviour

The `te::Engine` owns the `te::DeviceManager`, which manages all audio and MIDI devices. By default, the engine constructor automatically initializes the `DeviceManager` with a basic 2-input, 2-output configuration.

For professional applications requiring manual control, you must provide a custom `te::EngineBehaviour` subclass:

```cpp
class MyEngineBehaviour : public te::EngineBehaviour
{
public:
    // Prevent automatic device initialization
    bool autoInitialiseDeviceManager() override { return false; }

    // You can override many other engine behaviors here...
};

// --- In your application's startup code ---
// Pass an instance of your custom behaviour to the engine.
te::Engine engine { "My App", std::make_unique<MyEngineBehaviour>() };

// Now you are responsible for initializing the device manager.
// This is the "bootstrap" for your app's audio system.
engine.getDeviceManager().initialise (2, 2); // e.g., open 2 ins and 2 outs
```

### Phase 2: User-Driven Configuration

For runtime device configuration, JUCE provides `juce::AudioDeviceSelectorComponent`, a pre-built GUI panel for device settings:

```cpp
// In your settings component's constructor:
// 1. Create the component
audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent> (
    engine.getDeviceManager(), // Give it the engine's DeviceManager
    0,     // min input channels
    256,   // max input channels
    0,     // min output channels
    256    // max output channels
);

// 2. Display it
addAndMakeVisible (*audioSetupComp);
```

### InputDevice vs. InputDeviceInstance Data Model

Understanding the distinction between these two concepts is crucial:

**InputDevice**: A *template* or *descriptor* for a physical or virtual input device. It represents a device that *can* be used, like "Focusrite Scarlett 2i2 - Input 1". The `DeviceManager` provides lists of available devices. They don't process audio/MIDI themselves.

**InputDeviceInstance**: An *active, real-time instance* of an `InputDevice` connected to a specific `Track`. When you assign an `InputDevice` to a `Track`, the engine creates an `InputDeviceInstance` behind the scenes. This instance handles the actual live audio/MIDI data reception.

**Key Point**: You select a passive `InputDevice` from a global list, and the `Track` creates its own active `InputDeviceInstance` to handle the data stream. A single `InputDevice` can be used by multiple tracks, each having its own unique `InputDeviceInstance`.

### Assigning Inputs to Tracks

Assigning a physical input to a track is managed by connecting an `InputDevice` to a specific `AudioTrack`. This is not done via a single `setInputDevice()` method on the track itself, as the `Track` base class doesn't have one. Instead, the connection is managed by manipulating the target of an `InputDeviceInstance`.

Each `InputDeviceInstance` represents a live, routable input in the engine and can be assigned to one or more tracks.

Here is the correct approach, as implemented in NextStudio:

1.  **Iterate through available `InputDeviceInstance`s**: Get the list of all active input instances from the `Edit`.
2.  **Modify the target**: Use `InputDeviceInstance::setTarget()` or `InputDeviceInstance::removeTarget()` to associate or disassociate a track.

```cpp
// Example from NextStudio's TrackHeadComponent::showPopupMenu
// 'at' is the target te::AudioTrack*
// 'instance' is an te::InputDeviceInstance* from edit.getAllInputDevices()

if (instance->getTargets().contains (at->itemID))
{
    // If the track is already a target, remove it
    instance->removeTarget (at->itemID, &at->edit.getUndoManager());
}
else
{
    // Before adding the new target, NextStudio's implementation clears
    // all other targets from the instance to ensure an exclusive connection.
    for (auto currentTargetID : instance->getTargets())
        instance->removeTarget (currentTargetID, &at->edit.getUndoManager());

    // Set the track as the new target for the input instance
    instance->setTarget (at->itemID, false, &at->edit.getUndoManager(), 0);
}
```

This mechanism directly controls the audio routing by telling the `InputDeviceInstance` which track(s) should receive its data.

### Real-Time Data Flow Architecture

The audio data flow centers around the `tracktion::graph`, a sophisticated processing graph managed by an `EditPlaybackContext`:

1. **Hardware Input**: The hardware audio callback provides raw audio data to the `DeviceManager`
2. **Graph Construction**: An `EditPlaybackContext` (created when an `Edit` is playing) builds a `tracktion::graph` of all necessary processing nodes (inputs, clips, plugins, etc.)
3. **Pull-Based Processing**: Input nodes corresponding to active `InputDeviceInstance`s **pull** required audio data from the `DeviceManager` (pull model, not push)
4. **Graph Processing**: Data flows through the graph—from inputs, through plugins, to summing busses
5. **Output Delivery**: The output node delivers processed audio back to the `DeviceManager`, which sends it to hardware

### HostedAudioDeviceInterface for Plugin Development

The `HostedAudioDeviceInterface` is essential for plugin development. When building VST, AU, or AAX plugins with Tracktion Engine internally, direct hardware access is impossible—the host DAW provides audio.

This interface creates a bridge, allowing you to implement audio buffer forwarding from the host's process block into your internal engine instance. This is the primary mechanism for integrating Tracktion Engine within plugins.

### Advanced EngineBehaviour Customization

Beyond device management, `EngineBehaviour` controls numerous engine aspects:

- **Plugin Management**: Blacklist plugins, create custom plugin formats
- **Playback and Processing**: Control CPU usage, define pre/post-fader send logic
- **Edit and Model Behavior**: Set limits on track/clip counts, define fade creation
- **Control Surfaces**: Enable/disable hardware controller support

This comprehensive I/O system provides the foundation for building professional audio applications with the Tracktion Engine.

## Gemini Added Memories

### MIDI Focus and Selection Optimization
Refactored `EngineHelpers::setMidiInputFocusToSelection` in `App/Source/Utilities.cpp` to improve performance and stability:
- **Early Exit:** Compares current MIDI targets with the desired selection. Returns immediately if they match, preventing unnecessary audio graph rebuilds (`ensureContextAllocated`).
- **Robust Update:** Implements a "Clear All & Set All" strategy for modifying targets. This fixes issues where MIDI routing could get stuck (e.g., both old and new tracks playing) when switching selection quickly or using hardware controllers.
- **Reliable Graph Update:** Uses `edit.restartPlayback()` after routing changes to ensure the audio graph correctly reflects the new input configuration.

### Clip Interaction Safety
Updated `TrackLaneComponent::mouseUp` to prevent accidental resource-heavy operations:
- Checks if the mouse actually moved (`e.mouseWasDraggedSinceMouseDown()` and `delta > epsilon`) before calling `EngineHelpers::moveSelectedClips`.
- This prevents simple clicks on clips from triggering the complex move logic, which includes aggressive plugin state optimization (stripping/restoring plugins) that causes graph rebuilds.

### MIDI Input Device Handling
- Validated that `InputDeviceInstance` in Tracktion Engine supports multiple targets (multicasting).
- NextStudio enforces "Exclusive MIDI Focus" by manually clearing old targets before adding the selected track.
- The default behavior of `InputDeviceInstance::setTarget(..., move=true)` would clear all targets, but NextStudio uses `move=false` and manages targets manually to support multi-selection scenarios.

### Piano Roll Keyboard Selection Behavior
- Keyboard clicks select notes by pitch across the currently selected MIDI clips on the active track.
- Shift-click toggles that pitch: if any notes of that pitch are selected, all of them are removed from selection.
- Selection logic lives in `PianoRollEditor` via a keyboard callback; `KeyboardView` emits events only and does not know about the `MidiViewport`.
- `MidiViewport::unselectAll()` clears only MIDI note selection and keeps track/clip selection intact.

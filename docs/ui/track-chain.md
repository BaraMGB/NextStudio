# Track Chain

## Overview

The Track Chain is displayed in the Lower Zone when the Plugins tab is active. It shows all effects, modifiers, and track controls for the currently selected track.

## Layout

| Section | Position | Description |
|---|---|---|
| Track Presets | Left | Save and load complete Track Chains including all modifiers and plugins. Quick access via dropdown. |
| Modifier Stack | Center-left | List of active modifiers |
| Plugin Chain | Center | Horizontally arranged plugins |
| Channel Strip | Right | Volume/pan controls and mute/solo/record buttons |

### Collapsible side panels

Track Presets and Modifiers are collapsed by default so the plug-in canvas receives most of the available width. Each collapsed panel remains available as a narrow labeled rail.

- Use the arrow at the left of an expanded panel header to collapse it.
- Click the arrow or the collapsed rail to expand the panel again.
- The controls use the same arrow symbols as track headers.
- Track Presets and Modifiers can be collapsed independently.
- Collapsing Modifiers also hides an open modifier detail panel. The selected modifier is retained and its detail returns when the panel is expanded.
- Both states are application-wide preferences and are restored in the next NextStudio session.

## Modifier Stack

Modifiers allow parameters to be controlled automatically. They are displayed in a stack on the left.

### Modifier Types

| Type | Description |
|---|---|
| LFO | Generates periodic modulation (tremolo, vibrato). Waveform (Sine, Square, etc.), frequency and depth adjustable. |
| Random | Generates random parameter values. Range and speed adjustable. |
| Step | Step sequencer for rhythmic modulation. Number of steps, values and timing adjustable. |

### Managing Modifiers

- **Add:** Click the "Add Modifier" button.
- **Show GUI:** Click the stack button to show the GUI to the right of the modifier stack. Click again to collapse. Connections are displayed in the modifier button.
- **Disable connection:** Click the eye symbol.
- **Delete connection:** Click the trash can.
- **Delete modifier:** Right-click → Delete Modifier.

### Using Modifiers

- **Connect parameter:** Drag the [+] from the modifier to a parameter slider anywhere in the chain. The parameter is now automatically controlled by the modifier.
  - A modifier **cannot** be applied to its own parameters.
- **Show connections:** All connections are displayed in the modifier button.
- **Remove connection:** Click the X next to the parameter name in the list.

### Automation Lanes

Right-click on any parameter slider to add an **Automation Lane** to the track. Automation data is recorded when the global Automation Write toggle is enabled in the Header Bar, and played back when Automation Read is enabled.

### MIDI Learn

Any automatable slider, knob, or toggle in the Track Chain can be controlled via MIDI CC messages.

- **Assign:** Right-click a parameter and select "MIDI Learn." Move a MIDI controller to assign it.
- **Clear:** Right-click the parameter and select "Clear MIDI Learn."
- **Supported controls:** Sliders, toggles, and combo boxes that are part of the automatable parameter system.

## Plugin Chain

The Plugin Chain organizes plugins into logical sections based on their role in the signal flow.

### Sections

For **MIDI tracks**, the chain is divided into three sections:

1. **MIDI Plugins** — Process MIDI data before the instrument (arpeggiators, chord generators, MIDI effects).
2. **Instrument** — A single slot for the sound source (only one instrument per track).
3. **Audio Effects** — Process the audio output after the instrument (reverbs, delays, filters, etc.).

**Audio tracks** show only the Audio Effects section.

### Plugin List

The left panel shows all plugins organized by section. Clicking a plugin in the list scrolls its GUI into view and selects it. The horizontal scrollbar remains visible and reaches the complete rack content, including the end of a wide final plug-in GUI on narrow windows.

### Managing Effects

- **Add:** Click the [+] button within a section. The menu shows only plugins that belong in that section. Or drag an effect from the Side Browser.
- **Bypass:** Click the eye symbol to deactivate a plugin.
- **Delete:** Click the trash can to remove a plugin.
- **Move:** Drag plugin title bar to a [+] in the desired section.

### Signal Flow

The section structure ensures correct signal flow — MIDI plugins always process before the instrument, and audio effects always process after. You cannot accidentally place a reverb before your instrument.

## Channel Strip

The Channel Strip on the right side provides quick access to the most important track controls:

- **Volume Fader:** Controls the track volume.
- **Pan Knob:** Controls the stereo position (left/right).
- **Mute (M):** Mutes the track.
- **Solo (S):** Mutes all other tracks.
- **Record (R):** Arms the track for recording.
- **Level Meters:** Show the current level.

## Related documents

- [Song Editor](song-editor.md)
- [Mixer](mixer.md)
- [Header Bar](header-bar.md)
- [Getting Started](../user/getting-started.md)
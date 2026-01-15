# SimpleSynthPlugin - Technical Documentation

This document describes the technical implementation of the `SimpleSynthPlugin`, a polyphonic subtractive synthesizer developed within the NextStudio application context.

## Overview

`SimpleSynthPlugin` is a lightweight, polyphonic subtractive synthesizer implemented as a `tracktion_engine::Plugin`. It is located in `App/Source/Plugins/SimpleSynth/` and features a dedicated custom GUI for improved usability.

## Key Features

*   **Polyphony:** 16 voices with dynamic assignment.
*   **Oscillators:** 5 waveform types (Sine, Triangle, Saw, Square, Noise). Default: **Saw**.
*   **Anti-Aliasing:** PolyBLEP (Polynomial Band-Limited Step) implementation for Saw and Square waves to reduce aliasing artifacts.
*   **Envelope:** Full ADSR (Attack, Decay, Sustain, Release) envelope for amplitude control. Defaults: Instant attack/decay/release, full sustain.
*   **Smoothing:** Parameter smoothing on the master level to prevent zipper noise.
*   **Tuning:** Independent **Coarse** (-24/+24 semitones) and **Fine** (-100/+100 cents) tuning.
*   **Unison:** Stack up to 5 voices per note with adjustable detune and stereo spread.
*   **Filter:** 24dB Low Pass Ladder Filter for a classic analog sound.

## Architecture

The plugin is structured as a `tracktion_engine::Plugin` managing a pool of internal `Voice` objects and a dedicated GUI component.

### Class Structure

*   **`SimpleSynthPlugin`**: The main plugin class handling DSP processing, MIDI handling, and parameter management.
*   **`SimpleSynthPlugin::Voice`**: A nested private struct representing a single synth voice. Manages its own oscillator phase, ADSR state, and Ladder Filter instance.
*   **`SimpleSynthPluginComponent`**: The custom GUI component, organized into modular sections:
    *   `SimpleSynthOscSection`: Manages waveform, tuning, and unison.
    *   `SimpleSynthFilterSection`: Manages cutoff and resonance.
    *   `SimpleSynthEnvSection`: Dedicated ADSR controls.

### Parameters

The plugin exposes the following automatable parameters:

| Parameter Name | ID | Range | Description |
| :--- | :--- | :--- | :--- |
| **Level** | `level` | -100.0 dB to 0.0 dB | Master output volume. |
| **Coarse Tune** | `coarseTune` | -24.0 st to +24.0 st | Coarse pitch offset in semitones. |
| **Fine Tune** | `fineTune` | -100.0 c to +100.0 c | Fine pitch offset in cents. |
| **Wave** | `wave` | 0 to 4 | Waveform (Sine, Tri, Saw, Square, Noise). |
| **Attack** | `attack` | 0.001s to 5.0s | ADSR Attack time (Default: 0.001s). |
| **Decay** | `decay` | 0.001s to 5.0s | ADSR Decay time (Default: 0.001s). |
| **Sustain** | `sustain` | 0.0 to 1.0 | ADSR Sustain level (Default: 1.0). |
| **Release** | `release` | 0.001s to 5.0s | ADSR Release time (Default: 0.001s). |
| **Unison Voices** | `unisonOrder` | 1 to 5 | Number of stacked voices per note. |
| **Unison Detune** | `unisonDetune` | 0.0 to 100.0 cents | Pitch spread for unison voices. |
| **Unison Spread** | `unisonSpread` | 0.0 to 100.0 % | Stereo spread for unison voices. |
| **Retrigger** | `retrigger` | Off (0) / On (1) | Toggle oscillator phase reset on Note On. |
| **Filter Cutoff** | `cutoff` | 20 Hz to 20 kHz | Low Pass Filter Cutoff Frequency. |
| **Filter Res** | `resonance` | 0.0 to 1.0 | Filter Resonance. |

## Signal Flow

The `applyToBuffer` method handles the core DSP loop in blocks:

1.  **Parameter Update:** Fetches current values from automatable parameters and updates smoothing targets.
2.  **MIDI Processing:**
    *   **Note On:** Triggers `unisonOrder` voices. Each voice gets a unique `unisonBias` for detuning and panning.
    *   **Note Off:** Triggers release phase for all voices matching the note number.
3.  **Audio Generation:**
    *   **Oscillator:** Generates samples based on `waveShape`. Saw and Square use PolyBLEP correction.
    *   **Filter:** Processes samples through the `juce::dsp::LadderFilter`.
    *   **Envelope:** Multiplies by the current `juce::ADSR` gain and velocity.
    *   **Mixing:** Pans the signal into the stereo buffer based on unison spread and applies master level smoothing.

## State Management

The plugin state is fully persistent within the Tracktion Edit:
*   Uses `juce::CachedValue` linked to a `juce::ValueTree` for robust parameter handling.
*   GUI components (`SimpleSynthPluginComponent`) listen to the `ValueTree` to ensure synchronization between the engine and the view.

## Integration with NextStudio

Like all internal instruments, `SimpleSynthPlugin` is fully compatible with NextStudio's **Modifier** system. Users can modulate parameters such as *Filter Cutoff* or *Pitch* using external LFOs, Envelope Followers, or Step Sequencers via the plugin rack.

## Future Improvements / Known Limitations

*   **Secondary Oscillator:** Mixing two oscillators for more complex timbres.
*   **Voice Stealing:** Implementing a more advanced stealing algorithm for better high-polyphony performance.
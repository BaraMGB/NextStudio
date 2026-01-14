# SimpleSynthPlugin - Technical Documentation

This document describes the technical implementation of the `SimpleSynthPlugin`, a basic polyphonic synthesizer developed within the NextStudio application context.

## Overview

`SimpleSynthPlugin` is a lightweight, polyphonic subtractive synthesizer implemented as a `tracktion_engine::Plugin`. It demonstrates how to create a custom instrument within the Tracktion Engine framework using JUCE components for DSP.

## Key Features

*   **Polyphony:** 8 voices.
*   **Oscillators:** 5 waveform types (Sine, Triangle, Saw, Square, Noise).
*   **Anti-Aliasing:** PolyBLEP (Polynomial Band-Limited Step) implementation for Saw and Square waves to reduce aliasing artifacts.
*   **Envelope:** Full ADSR (Attack, Decay, Sustain, Release) envelope for amplitude control.
*   **Smoothing:** Parameter smoothing on the master level to prevent zipper noise.
*   **Tuning:** Global coarse tuning.

## Architecture

The plugin inherits from `tracktion_engine::Plugin` and manages a fixed pool of voice objects.

### Class Structure

*   **`SimpleSynthPlugin`**: The main plugin class handling parameter management, state persistence, and audio block processing.
*   **`SimpleSynthPlugin::Voice`**: A nested private struct representing a single synthesizer voice. It holds the state for one note (phase, current note, velocity, ADSR state).

### Parameters

The plugin exposes the following automatable parameters:

| Parameter Name | ID | Range | Description |
| :--- | :--- | :--- | :--- |
| **Level** | `level` | -100.0 dB to 0.0 dB | Master output volume. |
| **Tune** | `tune` | -24.0 st to +24.0 st | Global pitch offset in semitones. |
| **Wave** | `wave` | 0 to 4 | Waveform selector (0=Sine, 1=Tri, 2=Saw, 3=Square, 4=Noise). |
| **Attack** | `attack` | 0.001s to 5.0s | ADSR Attack time. |
| **Decay** | `decay` | 0.001s to 5.0s | ADSR Decay time. |
| **Sustain** | `sustain` | 0.0 to 1.0 | ADSR Sustain level. |
| **Release** | `release` | 0.001s to 5.0s | ADSR Release time. |
| **Unison Voices** | `unisonOrder` | 1 to 5 | Number of stacked voices per note. |
| **Unison Detune** | `unisonDetune` | 0.0 to 100.0 cents | Pitch spread for unison voices. |
| **Unison Spread** | `unisonSpread` | 0.0 to 100.0 % | Stereo spread for unison voices. |
| **Retrigger** | `retrigger` | Off (0) / On (1) | Determines oscillator phase start behavior. |

## Signal Flow

The `applyToBuffer` method is the core DSP loop, processed in blocks:

1.  **Parameter Update:**
    *   The Master Level target is updated for the `LinearSmoothedValue`.
    *   ADSR parameters are captured from the `AutomatableParameter`s.
    *   Unison settings (Order, Detune, Spread, Retrigger) are read.

2.  **MIDI Processing:**
    *   The input MIDI buffer is iterated.
    *   **Note On:** Triggers `unisonOrder` number of voices.
        *   Each voice is assigned a `unisonBias` value (ranging from -1.0 to +1.0) which determines its relative position in the stereo and pitch field.
        *   **Phase Initialization:** If `Retrigger` is On, phase starts at 0.0. If Off, phase starts randomly (prevents phasing).
        *   Finds inactive voices in the pool and calls `start()`.
    *   **Note Off:** Finds *all* active voices matching the note number. If `isKeyDown` is true, it triggers release (`stop()`).
    *   **All Notes Off:** Stops all voices immediately.

3.  **Audio Generation:**
    *   The loop iterates through each sample in the block.
    *   **Parameter Update (Control Rate):** Before the sample loop, `pan` and `detuneMultiplier` are recalculated for every active voice based on its `unisonBias` and the *current* global `unisonSpread` and `unisonDetune` parameters. This allows for real-time modulation of unison width and thickness.
    *   **Level Smoothing:** The master gain is interpolated for each sample using `masterLevelSmoother`.
    *   **Voice Rendering:**
        *   Active voices calculate their waveform sample.
        *   **PolyBLEP:** Sawtooth and Square waves use PolyBLEP smoothing at discontinuities to reduce aliasing.
        *   **Phase:** Incremented by `phaseDelta`, wrapping around `2 * pi`. `phaseDelta` includes the unison detuning.
        *   **Envelope:** `juce::ADSR` is used to calculate the gain for each sample.
        *   The oscillator output is multiplied by the `adsrGain` and `velocity`.
        *   **Panning:** The voice output is distributed to Left/Right channels based on its calculated `currentPan` value.
        *   **Gain Compensation:** If Unison is active, the total gain is reduced by $1/\sqrt{N}$ to maintain consistent volume.
    *   **Voice Deactivation:** If the ADSR becomes inactive, `v.active` is set to `false`, freeing the voice.

## DSP Details

### Oscillator
*   **Sine:** `std::sin(phase)`
*   **Triangle:** Naive generation. Due to the rapid harmonic rolloff ($1/n^2$), aliasing is negligible for this use case.
*   **Saw:** Naive Sawtooth minus a PolyBLEP correction term at the phase reset point.
*   **Square:** Naive Square plus PolyBLEP correction terms at both the rising (0) and falling ($\pi$) edges.
*   **Noise:** `random.nextFloat() * 2.0 - 1.0` (White Noise)

## State Management

The plugin state is saved to and restored from a `juce::ValueTree`.
*   **Restoration:** `restorePluginStateFromValueTree` reads properties like `wave`, `level`, `attack`, etc., and updates the `CachedValue` objects.

## Future Improvements / Known Limitations

*   **Voice Stealing:** The current implementation simply searches for the first inactive voice. A smarter stealing algorithm (e.g., stealing the quietest voice in release phase) would be better for high polyphony.
*   **Filter:** Implementation of a low-pass filter (SVF or Ladder) is planned.

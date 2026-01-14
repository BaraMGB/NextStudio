# SimpleSynthPlugin - Technical Documentation

This document describes the technical implementation of the `SimpleSynthPlugin`, a basic polyphonic synthesizer developed within the NextStudio application context.

## Overview

`SimpleSynthPlugin` is a lightweight, polyphonic subtractive synthesizer implemented as a `tracktion_engine::Plugin`. It demonstrates how to create a custom instrument within the Tracktion Engine framework using JUCE components for DSP.

## Key Features

*   **Polyphony:** 8 voices.
*   **Oscillators:** 5 waveform types (Sine, Triangle, Saw, Square, Noise).
*   **Smoothing:** Parameter smoothing on the master level to prevent zipper noise.
*   **Tuning:** Global coarse tuning.
*   **Simple Release:** A basic exponential decay on note-off to prevent clicks.

## Architecture

The plugin inherits from `tracktion_engine::Plugin` and manages a fixed pool of voice objects.

### Class Structure

*   **`SimpleSynthPlugin`**: The main plugin class handling parameter management, state persistence, and audio block processing.
*   **`SimpleSynthPlugin::Voice`**: A nested private struct representing a single synthesizer voice. It holds the state for one note (phase, current note, decay level).

### Parameters

The plugin exposes the following automatable parameters:

| Parameter Name | ID | Range | Description |
| :--- | :--- | :--- | :--- |
| **Level** | `level` | -100.0 dB to 0.0 dB | Master output volume. |
| **Tune** | `tune` | -24.0 st to +24.0 st | Global pitch offset in semitones. |
| **Wave** | `wave` | 0 to 4 | Waveform selector (0=Sine, 1=Tri, 2=Saw, 3=Square, 4=Noise). |

## Signal Flow

The `applyToBuffer` method is the core DSP loop, processed in blocks:

1.  **Parameter Update:**
    *   The Master Level target is updated for the `LinearSmoothedValue`.

2.  **MIDI Processing:**
    *   The input MIDI buffer is iterated.
    *   **Note On:** Finds the first inactive voice and triggers it (`start()`).
    *   **Note Off:** Finds the active voice matching the note number and triggers release (`stop()`).
    *   **All Notes Off:** Stops all voices immediately.

3.  **Audio Generation:**
    *   The loop iterates through each sample in the block.
    *   **Level Smoothing:** The master gain is interpolated for each sample using `masterLevelSmoother`.
    *   **Voice Rendering:**
        *   Active voices calculate their waveform sample based on the current `phase`.
        *   **Phase:** Incremented by `phaseDelta`, wrapping around `2 * pi`.
        *   **Envelope:** A simple manual exponential decay is applied during the release phase.
        *   The oscillator output is multiplied by the current level.
        *   Voices are summed into the left and right output channels.
    *   **Voice Deactivation:** If the level falls below a threshold during release, `v.active` is set to `false`, freeing the voice.

## DSP Details

### Oscillator
The oscillators use naive mathematical generation (non-band-limited):
*   **Sine:** `std::sin(phase)`
*   **Triangle:** Calculated from phase to map `0..2pi` to a triangle wave.
*   **Saw:** `2.0 * (phase / 2pi) - 1.0`
*   **Square:** `phase < pi ? 1.0 : -1.0`
*   **Noise:** `random.nextFloat() * 2.0 - 1.0` (White Noise)

## State Management

The plugin state is saved to and restored from a `juce::ValueTree`.
*   **Restoration:** `restorePluginStateFromValueTree` reads properties like `wave`, `level`, etc., and updates the `CachedValue` objects.

## Future Improvements / Known Limitations

*   **Aliasing:** The naive oscillators will alias at high frequencies. Implementation of PolyBLEP or Wavetables would improve audio quality.
*   **Voice Stealing:** The current implementation simply searches for the first inactive voice.
*   **Velocity:** Velocity is currently received but not applied to the output gain.
*   **ADSR:** A full ADSR envelope implementation is planned for future iterations.
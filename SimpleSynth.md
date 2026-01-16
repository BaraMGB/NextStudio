# SimpleSynthPlugin - Technical Documentation

This document describes the technical implementation of the `SimpleSynthPlugin`, a polyphonic subtractive synthesizer developed within the NextStudio application context.

## Overview

`SimpleSynthPlugin` is a lightweight, polyphonic subtractive synthesizer implemented as a `tracktion_engine::Plugin`. It is located in `App/Source/Plugins/SimpleSynth/` and features a dedicated custom GUI for improved usability.

## Key Features

*   **Polyphony:** 16 voices with dynamic assignment and LRU voice stealing.
*   **Oscillators:** 5 waveform types (Sine, Triangle, Saw, Square, Noise). Default: **Saw**.
*   **Anti-Aliasing:** PolyBLEP for Saw/Square and PolyBLAMP for Triangle waves to reduce aliasing artifacts.
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
    *   **Note On:** Triggers `unisonOrder` voices. Uses **Voice Stealing** (LRU) if the 16-voice limit is reached.
    *   **Note Off:** Triggers release phase for all voices matching the note number.
3.  **Audio Generation:**
    *   **Oscillator:** Generates samples based on `waveShape`. All waveforms use appropriate anti-aliasing (PolyBLEP/PolyBLAMP).
    *   **Filter:** Processes samples through the `juce::dsp::LadderFilter`.
    *   **Envelope:** Multiplies by the current `juce::ADSR` gain and velocity.
    *   **Mixing:** Pans the signal into the stereo buffer based on unison spread and applies master level smoothing.

## State Management

The plugin state is fully persistent within the Tracktion Edit:
*   Uses `juce::CachedValue` linked to a `juce::ValueTree` for robust parameter handling.
*   GUI components (`SimpleSynthPluginComponent`) listen to the `ValueTree` to ensure synchronization between the engine and the view.

## Integration with NextStudio

Like all internal instruments, `SimpleSynthPlugin` is fully compatible with NextStudio's **Modifier** system. Users can modulate parameters such as *Filter Cutoff* or *Pitch* using external LFOs, Envelope Followers, or Step Sequencers via the plugin rack.

## Code Review & Quality Assessment

### Overall Assessment: B+

The SimpleSynth plugin demonstrates solid audio engineering fundamentals with good architecture, but has several areas needing improvement for production readiness.

### Critical Issues (Resolved)

1.  **Thread Safety Deficiencies:** Parameters are now accessed via `std::atomic<float>` in the audio thread.
2.  **No Error Handling:** Robust validation and clamping implemented for all parameters and sample rates.
3.  **Performance Issues:** Monolithic `applyToBuffer` refactored; control-rate updates moved outside the sample loop.
4.  **Inconsistent Anti-Aliasing:** PolyBLAMP implemented for Triangle waves.
5.  **Fixed Voice Allocation:** LRU Voice Stealing algorithm implemented.

### Pending Action Items

The following issues from the code review remain open:

*   **Standard Features:**
    *   Plugin bypass logic is missing.
    *   Latency reporting is not implemented.
*   **Refactoring:**
    *   Parameter management code is still repetitive (boilerplate).
    *   UI Layout relies on hardcoded magic numbers.
    *   Improve filter modulation to use logarithmic scaling (semitones) instead of linear Hz.

## Refactoring Log - Jan 2026

### Completed Improvements

1.  **Thread Safety Implemented**
    *   Introduced `std::atomic<float>` mirrors for all automatable parameters within a dedicated `AudioParams` struct.
    *   Added `juce::ValueTree::Listener` implementation to synchronize atomic values from the Message Thread.
    *   Refactored audio processing to read exclusively from these atomic variables, eliminating race conditions between UI and Audio threads.

2.  **Audio Quality Enhancements**
    *   Implemented **PolyBLAMP** (Polynomial Band-Limited Ramp) anti-aliasing for the Triangle waveform.
    *   This eliminates the high-frequency aliasing artifacts previously present at slope discontinuities (peaks) of the triangle wave.

3.  **Code Restructuring**
    *   Refactored the monolithic `applyToBuffer` method into specialized methods: `processMidiMessages()`, `updateVoiceParameters()`, and `renderAudio()`.
    *   Snapshotting of parameters at the start of each block ensures consistent processing state.

4.  **Error Handling & Validation**
    *   Implemented robust input validation in `applyToBuffer` (checking buffer/sampleRate).
    *   Added explicit clamping of all DSP parameters (unisonOrder, MIDI range, filter coeffs) to safe ranges.

5.  **Voice Stealing (LRU Algorithm)**
    *   Implemented a **Least Recently Used (LRU)** voice stealing algorithm.
    *   The system now tracks the "age" of each note via a global `noteCounter`.
    *   When the 16-voice limit is reached, the system prioritizes stealing voices in the release phase (key up) or, if necessary, the oldest active voice.
    *   Verified functionality with a reduced-voice (3) test build.

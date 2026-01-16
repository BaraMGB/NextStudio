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

## Code Review & Quality Assessment

### Overall Assessment: B+

The SimpleSynth plugin demonstrates solid audio engineering fundamentals with good architecture, but has several areas needing improvement for production readiness.

### Critical Issues

#### 1. Thread Safety Deficiencies
- **SimpleSynthPlugin.cpp:164-173**: Parameters accessed without atomic operations in audio thread
- **SimpleSynthPlugin.h:90-92**: Voice state flags accessed from both MIDI and audio threads without synchronization
- **Risk**: Race conditions, audio glitches, crashes

#### 2. No Error Handling
- Complete absence of validation and error checking
- **SimpleSynthPlugin.cpp:159**: Only basic sample rate check
- **Risk**: Undefined behavior, crashes with invalid parameters

#### 3. Performance Issues
- **SimpleSynthPlugin.cpp:304**: Filter cutoff updated every sample (should be control-rate)
- **SimpleSynthPlugin.cpp:198-214**: Linear voice search instead of free voice queue
- **SimpleSynthPlugin.cpp:270-272**: Unnecessary frequency recalculations

### Code Quality Issues

#### 1. Method Length Violations
- **SimpleSynthPlugin.cpp:151-380**: `applyToBuffer` is 229 lines - violates single responsibility
- Should be broken into: `processMidiMessages()`, `updateVoiceParameters()`, `renderAudioBlock()`

#### 2. Repetitive Code
- **SimpleSynthPlugin.cpp:105-123**: 19 identical parameter detach calls
- **SimpleSynthPlugin.h:44-82**: Repetitive parameter declarations suggest need for parameter registry pattern

#### 3. Magic Numbers
- **SimpleSynthPlugin.cpp:130**: `4096` block size should be configurable
- **SimpleSynthPlugin.cpp:288**: `std::sqrt((float)unisonOrder)` gain compensation needs documentation

### Audio Quality Concerns

#### 1. Inconsistent Anti-Aliasing
- **SimpleSynthPlugin.cpp:316-320**: Triangle wave has no anti-aliasing (unlike saw/square)
- **SimpleSynthPlugin.cpp:311-344**: Good PolyBLEP implementation for other waveforms

#### 2. Filter Envelope Scaling
- **SimpleSynthPlugin.cpp:300-301**: Linear frequency sweep (`18000.0f` limit) is less musical than logarithmic scaling

### Architecture Issues

#### 1. Fixed Voice Allocation
- **SimpleSynthPlugin.h:112-113**: Static 16-voice array without voice stealing
- **SimpleSynthPlugin.cpp:195**: No bounds checking for unison voice requests

#### 2. Missing Plugin Standards
- No bypass logic implementation
- No latency reporting
- No MIDI channel filtering

### UI/UX Issues

#### 1. Hardcoded Layout
- **SimpleSynthPluginComponent.cpp:275-297**: Magic numbers for layout calculations
- No responsive design considerations

#### 2. Missing Visual Feedback
- No parameter automation indication
- No modulation visualization

### Positive Aspects

#### 1. Clean Architecture
- Good separation between DSP, parameters, and UI
- Proper use of JUCE patterns (ValueTree, automatable parameters)
- Modular UI component design

#### 2. Audio Engineering Excellence
- PolyBLEP anti-aliasing implementation (SimpleSynthPlugin.cpp:8-26)
- Proper denormal handling (SimpleSynthPlugin.cpp:359)
- Filter stability protection (SimpleSynthPlugin.cpp:183)

#### 3. Memory Management
- RAII compliance
- No dynamic allocations in audio thread
- Proper cleanup in destructor

### Priority Recommendations

#### High Priority
1. **Add thread-safe parameter access** using `std::atomic<float>`
2. **Implement comprehensive error handling** and validation
3. **Break down `applyToBuffer`** into smaller, focused methods
4. **Add triangle wave anti-aliasing**

#### Medium Priority
1. **Implement voice stealing algorithm** for better voice management
2. **Optimize to control-rate processing** for filter parameters
3. **Add free voice queue** for efficient voice allocation
4. **Implement plugin bypass** and latency reporting

#### Low Priority
1. **Refactor parameter management** to reduce code duplication
2. **Add comprehensive unit tests**
3. **Improve UI responsiveness** with layout metrics
4. **Add preset management validation**

## Future Improvements / Known Limitations

*   **Secondary Oscillator:** Mixing two oscillators for more complex timbres.
*   **Voice Stealing:** Implementing a more advanced stealing algorithm for better high-polyphony performance.
*   **Thread Safety:** Add atomic operations for parameter access and voice state management.
*   **Error Handling:** Implement comprehensive validation and error checking throughout the codebase.
*   **Performance Optimization:** Control-rate filter processing and efficient voice allocation algorithms.

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
    *   Refactored the monolithic `applyToBuffer` method into three specialized, single-responsibility methods:
        *   `processMidiMessages()`: Handles note on/off and voice allocation.
        *   `updateVoiceParameters()`: Handles control-rate updates (pan, detune, resonance) once per block.
        *   `renderAudio()`: Handles the sample-accurate DSP loop (oscillators, filters, envelopes).
    *   Snapshotting of parameters at the start of each block ensures consistent processing state.

4.  **Error Handling & Validation**
    *   Implemented robust input validation in `applyToBuffer` (checking buffer/sampleRate).
    *   Added explicit clamping of all DSP parameters (unisonOrder, MIDI range, filter coeffs) to safe ranges.
    *   Secured loops against corrupted `unisonOrder` values to prevent potential hangs.

### Pending Action Items

The following issues from the code review remain open:

*   **Voice Management:**
    *   No **Voice Stealing** algorithm implementation yet. If more than 16 notes are played, new notes are dropped.
    *   Linear search for free voices is still used (O(N)), though less critical with only 16 voices.
*   **Standard Features:**
    *   Plugin bypass logic is missing.
    *   Latency reporting is not implemented.
*   **Refactoring:**
    *   Parameter management code is still repetitive (boilerplate).
    *   UI Layout relies on hardcoded magic numbers.
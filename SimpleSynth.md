# SimpleSynthPlugin - Technical Documentation

This document describes the technical implementation of the `SimpleSynthPlugin`, a polyphonic subtractive synthesizer developed within the NextStudio application context.

## Overview

`SimpleSynthPlugin` is a lightweight, polyphonic subtractive synthesizer implemented as a `tracktion_engine::Plugin`. It is located in `App/Source/Plugins/SimpleSynth/` and features a dedicated custom GUI for improved usability.

*   **Dual Oscillators:** Two independent oscillators with Sine, Triangle, Saw, Square, and Noise waveforms.
*   **Synthesis Modes:** Mix, Ring Modulation, Frequency Modulation (FM), and Hard Sync.
*   **Unison:** Stack up to 5 voices per note with adjustable detune and stereo spread.
*   **Filter:** Selectable filter types for different sonic characters:
    *   **Ladder:** 24dB Low Pass Ladder Filter for a classic analog sound (internally smoothed).
    *   **SVF:** 12dB State Variable TPT Filter for clean, snappy modulation without internal smoothing.

## Architecture
...
### Parameters

The plugin exposes the following automatable parameters:

| Parameter Name | ID | Range | Description |
| :--- | :--- | :--- | :--- |
| **Level** | `level` | -100.0 dB to 0.0 dB | Master output volume. |
| **Coarse Tune** | `coarseTune` | -24.0 st to +24.0 st | Coarse pitch offset in semitones. |
| **Fine Tune** | `fineTune` | -100.0 c to +100.0 c | Fine pitch offset in cents. |
| **Wave** | `wave` | 0 to 4 | Waveform (Sine, Tri, Saw, Square, Noise). |
| **Osc 2 On** | `osc2Enabled` | 0 / 1 | Enable/Disable Oscillator 2 to save CPU. |
| **Osc 2 Wave** | `osc2Wave` | 0 to 4 | Waveform for Oscillator 2. |
| **Osc 2 Coarse** | `osc2Coarse` | -24.0 st to +24.0 st | Coarse pitch offset for Osc 2 (Relative to Note). |
| **Osc 2 Fine** | `osc2Fine` | -100.0 c to +100.0 c | Fine pitch offset for Osc 2. |
| **Osc 2 Level** | `osc2Level` | 0.0 to 1.0 | Volume of Oscillator 2. |
| **Mix Mode** | `mixMode` | Mix/Ring/FM/Sync | Algorithm for combining Osc 1 and Osc 2. |
| **Cross Mod** | `crossModAmount` | 0.0 to 1.0 | Depth of RingMod blending or FM phase modulation. |
| **Attack** | `attack` | 0.001s to 5.0s | ADSR Attack time (Default: 0.001s). |
| **Decay** | `decay` | 0.001s to 5.0s | ADSR Decay time (Default: 0.001s). |
| **Sustain** | `sustain` | 0.0 to 1.0 | ADSR Sustain level (Default: 1.0). |
| **Release** | `release` | 0.001s to 5.0s | ADSR Release time (Default: 0.001s). |
| **Unison Voices** | `unisonOrder` | 1 to 5 | Number of stacked voices per note. |
| **Unison Detune** | `unisonDetune` | 0.0 to 100.0 cents | Pitch spread for unison voices. |
| **Unison Spread** | 0.0 to 100.0 % | Stereo spread for unison voices. |
| **Retrigger** | `retrigger` | Off (0) / On (1) | Toggle oscillator phase reset on Note On. |
| **Filter Type** | `filterType` | Ladder / SVF | Selects the filter algorithm. |
| **Filter Cutoff** | `cutoff` | 20 Hz to 20 kHz | Low Pass Filter Cutoff Frequency. |
| **Filter Res** | `resonance` | 0.0 to 1.0 | Filter Resonance. |
| **Filter Drive** | `drive` | 1.0 to 10.0 | Ladder Filter Input Drive / Saturation. (Ladder Only) |
...
## Refactoring Log - Jan 2026

### Completed Improvements
...
7.  **Logarithmic Filter Modulation**
    *   Changed filter envelope modulation from linear Hz mapping to logarithmic semitone mapping (+/- 60 semitones).
    *   This provides a much more musical response to envelope depth adjustments.

8.  **Selectable Filter Architecture (Snappy Modulation)**
    *   Integrated `juce::dsp::StateVariableTPTFilter` (SVF) as an alternative to the Ladder filter.
    *   Added `filterType` parameter and `AutomatableComboBox` UI control.
    *   SVF allows for instantaneous parameter updates, enabling extremely snappy envelopes (0ms attack) which were previously blurred by the Ladder filter's internal smoothing.

9.  **DSP Performance Optimization**
    *   Moved constant `sqrt(unisonOrder)` calculation out of the per-sample loop.
    *   Switched SVF filter processing to use `processSample` instead of constructing `AudioBlock` and `ProcessContext` for every sample, significantly reducing function call overhead.

10. **Real-time Envelope Updates**
    *   Updated `updateVoiceParameters` to refresh ADSR parameters (Attack, Decay, Sustain, Release) during active voice playback. This allows audible changes when adjusting Sustain or Release on held notes.

11. **Dynamic Unison Updates**
    *   Implemented intelligent re-triggering of held notes when the Unison Voice count is changed.
    *   Previously, changing unison voices required re-striking keys to hear the effect. Now, active voices are stopped and immediately re-triggered with the new unison count, providing instant audible feedback.

12. **Code Quality: Parameter Initialization Refactoring**
    *   Refactored the Plugin Constructor to use a helper lambda for parameter setup.
    *   Reduced boilerplate code by approximately 40 lines and improved readability by centralizing the `referTo`, `addParam`, and `attachToCurrentValue` logic.

13. **New Feature: Ladder Filter Drive**
    *   Added a `drive` parameter (1.0 - 10.0) exclusively for the Ladder Filter mode.
    *   Allows compensating for the inherent volume drop of the Ladder architecture and driving the filter into saturation.
    *   Added UI logic to disable (grey out) the Drive slider when SVF mode is selected.

14. **Dual Oscillator & Cross-Modulation System**
    *   **Architecture:** Extended `Voice` struct and parameter set to support two independent oscillators.
    *   **DSP Implementation:**
        *   Extracted `generateWaveSample` for efficient reuse.
        *   Implemented 4 mixing algorithms: **Mix** (Blend), **RingMod** (Multiplication), **FM** (Phase Modulation), and **Hard Sync** (Phase Reset).
        *   Decoupled Oscillator tuning logic. Osc 1 and Osc 2 are now tuned relative to the MIDI note independently, allowing for proper Hard Sync sweeps where the "Slave" (Osc 1) frequency changes while the "Master" (Osc 2) holds the pitch.
    *   **GUI:**
        *   Refactored `SimpleSynthOscSection` to be reusable for both oscillators.
        *   **Osc 2 Panel:** Integrated Mix Mode and Cross Modulation controls directly into the Osc 2 panel for better workflow. Added an On/Off toggle switch.
        *   **Layout Overhaul:** Switched to horizontal headers for all panels (Osc, Filter, Env). Added consistent vertical padding (10px). Optimized component alignment (Label top, Control centered below).
        *   **Look & Feel:** Implemented centered text rendering for ComboBoxes in `NextLookAndFeel`.

15. **Bugfix: Init Preset Generation**
    *   **Issue:** The "Init" preset was not correctly resetting parameters because `getFactoryDefaultState` returned an empty ValueTree, failing to overwrite existing parameter values.
    *   **Fix:** Updated `getFactoryDefaultState` to explicitly populate the returned ValueTree with the default values from all `CachedValue` instances in the plugin.
    *   **Robustness:** Improved XML parsing in `PresetManagerComponent` to use `juce::XmlDocument::parse` instead of brittle string checks, ensuring reliable preset loading even with formatting variations.







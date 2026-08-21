# Simple Synth

## Overview

| Property | Value |
|----------|-------|
| Type ID | `simple_synth` |
| Category | Instrument |
| Voices | 16 (with stealing) |
| Output | Stereo |
| Architecture | Two-oscillator subtractive |

A lightweight subtractive synthesizer with two oscillators, a filter, and dual ADSR envelopes.

## Voice Mode

| Mode | Description |
|------|-------------|
| Poly | Standard polyphonic playback |
| Mono | Monophonic; new notes retrigger the existing voice; held notes tracked in a priority stack |

### Glide (Mono only)

| Parameter | Description |
|-----------|-------------|
| Glide Mode | Off / Always (every note-on) / Legato (only when a key is held) |
| Glide Time | 0–2 seconds |

## Osc 1

| Parameter | Range | Description |
|-----------|-------|-------------|
| Waveform | Sine, Triangle, Saw, Square, Noise | Primary oscillator shape |
| Coarse Tune | Semitones | Pitch offset in semitones |
| Fine Tune | Cents | Pitch offset in cents |
| Unison Order | 1–5 | Number of detuned unison voices |
| Unison Detune | — | Detune amount between unison voices |
| Unison Spread | — | Stereo spread of unison voices |
| Retrigger | Toggle | Restarts oscillator phase on each note-on |

## Osc 2

| Parameter | Range | Description |
|-----------|-------|-------------|
| Enabled | Toggle | Activates second oscillator |
| Waveform | Sine, Triangle, Saw, Square, Noise | Oscillator shape |
| Coarse Tune | Semitones | Relative to base note (not Osc 1) |
| Fine Tune | Cents | Relative to base note (not Osc 1) |
| Level | — | Output level of Osc 2 |
| Mix Mode | Mix, Ring Mod, FM, Hard Sync | How Osc 2 combines with Osc 1 |
| Cross Mod Amount | — | Modulation depth for FM / Hard Sync modes |

## Filter

| Parameter | Range | Description |
|-----------|-------|-------------|
| Type | Ladder 24dB, SVF 12dB | Filter topology and slope |
| Cutoff | 20–20000 Hz | Filter cutoff frequency |
| Resonance | — | Filter resonance/emphasis |
| Drive | 1–10 | Input drive into the filter |
| Env Amount | −100 to +100 | Filter envelope modulation depth |

## Envelopes

| Envelope | Stages | Description |
|----------|--------|-------------|
| Amp | ADSR | Amplitude envelope |
| Filter | ADSR | Filter modulation envelope |

## Master

| Parameter | Range | Description |
|-----------|-------|-------------|
| Level | −100 to 0 dB | Final output level |
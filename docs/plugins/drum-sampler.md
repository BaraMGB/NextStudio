# Drum Sampler

## Overview

| Property | Value |
|----------|-------|
| Type ID | Tracktion Engine `sampler` |
| Category | Instrument |
| Pads | 16 |
| MIDI Note Range | 48 (C3) to 63 |

A 16-pad drum sampler with drag-and-drop sample loading and per-pad sound editing.

## Pads

| Action | Behavior |
|--------|----------|
| Click | Plays the pad and opens the sound editor |
| Drag & drop | Drop audio from the Side Browser onto a pad |
| Pad-to-pad drag | Moves or swaps samples between pads |
| Right-click | Context menu |
| MIDI input | Illuminates pads (velocity-dependent brightness) |

## Sound Editor

| Parameter | Range / Type | Description |
|-----------|-------------|-------------|
| Gain | dB | Per-pad gain |
| Pan | L/R | Per-pad pan position |
| Open Ended | Toggle | Active = plays sample to end; disabled = one-shot with start/end markers |
| Waveform | Display | Visual waveform with start/end markers (adjustable) |
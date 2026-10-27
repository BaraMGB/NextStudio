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
| Drag & drop | Drop audio from the Side Browser or a Wave clip from the Song Editor onto a pad |
| Pad-to-pad drag | Moves or swaps samples between pads |
| Right-click | Context menu |
| MIDI input | Illuminates pads (velocity-dependent brightness) |

## Loading Wave clips from the Song Editor

Drag the body of a Wave clip from the arrangement onto a pad. The visible clip range is rendered to a persistent WAV file in NextStudio's configured Samples directory and assigned to the destination pad; the source clip remains in place. Clip trimming, fades, stretching and the active render path are therefore reflected in the sample rather than loading the complete original source file unchanged.

A track header hovered during the drag becomes the target track and opens its Track Chain, even when the Mixer or MIDI Editor was visible. This allows a sampler on another track to be revealed before continuing onto one of its pads. The additional drag image is hidden over arrangement track lanes and shown after leaving them.

After a successful drop, the sampler's track and destination pad are selected. The pad label and Sound Editor refresh immediately. Track selection and Lower Zone switching are owned by the Song Editor/track-header drag flow; the sampler only accepts the sample and reports whether the pointer is over a valid pad.

## Sound Editor

| Parameter | Range / Type | Description |
|-----------|-------------|-------------|
| Gain | dB | Per-pad gain |
| Pan | L/R | Per-pad pan position |
| Open Ended | Toggle | Active = plays sample to end; disabled = one-shot with start/end markers |
| Waveform | Display | Visual waveform with start/end markers (adjustable) |
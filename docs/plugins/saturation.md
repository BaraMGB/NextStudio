# Saturation

## Overview

| Property | Value |
|---|---|
| Type ID | `next_saturation` |
| Category | Effect |
| Channels | Mono, Stereo |
| Oversampling | 1×, 2×, 4× |

## Controls

| Parameter | Range | Description |
|---|---|---|
| Input | −24 to +24 dB | Input gain before saturation |
| Drive | 0 to +36 dB | Additional drive pushing the saturation harder |
| Tone | 0–1.0 | Highpass filter applied before saturation |
| Bias | −1.0 to +1.0 | Asymmetric distortion — generates odd harmonics |
| Mix | 0–100% | Dry/wet ratio |
| Output | −24 to +24 dB | Output gain after saturation |
| Mode | Soft Clip / Smooth / Hard Clip | Saturation algorithm |
| Quality | 1× / 2× / 4× | Oversampling factor — higher = better quality, more CPU |

### Modes

| Mode | Description |
|---|---|
| Soft Clip | Warm, gentle clipper |
| Smooth | Soft saturation curve |
| Hard Clip | Hard limiter |

## Meters

Live input and output level meters.

## Graph

Live transfer curve showing the distortion behaviour of the currently selected mode.
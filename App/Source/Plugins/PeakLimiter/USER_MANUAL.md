# Peak Limiter - User Manual

## Overview

The **Peak Limiter** is a transparent brickwall-style limiter for mono and stereo signals with fixed look-ahead.
It controls short peaks quickly and reliably while preserving the signal character.

- **Plugin name in app:** `Peak Limiter`
- **Type ID:** `peak_limiter`
- **Channels:** Mono and Stereo
- **Look-ahead:** fixed at **3.0 ms**

## Typical Use Cases

- final dynamics stage on a track
- peak control before a bus or master
- transient overshoot protection

## Controls

## 1) Input

Boosts or attenuates signal level **before** limiter gain control.

- Range: **-24.0 dB to +24.0 dB**
- Default: **0.0 dB**

Higher Input usually results in stronger gain reduction.

## 2) Ceiling

Sets the maximum target output level.

- Range: **-12.00 dB to -0.01 dB**
- Default: **-0.30 dB**

Lower ceiling values provide more headroom and generally reduce clipping risk.

## 3) Release

Controls how quickly the limiter recovers after attenuation.

- Range: **5.0 ms to 500.0 ms**
- Default: **80.0 ms**

Short release times feel more aggressive but may pump.
Longer release times sound smoother but can soften transients.

## 4) Stereo Link

Links gain reduction behavior between channels.

- **On** (default): both channels share the same reduction
- **Off:** each channel is processed independently

For stable stereo imaging, **On** is usually the better choice.

## Metering

The plugin exposes three live values:

- **Input:** peak level at limiter input
- **Output:** peak level at limiter output
- **Reduction:** maximum gain reduction in the current block

The meter displays the same values as IN / OUT / GR.

## Suggested Starting Points

### Transparent Peak Control

- Input: **0 dB**
- Ceiling: **-0.3 dB**
- Release: **80-120 ms**
- Stereo Link: **On**

### More Loudness on a Single Track

- Input: **+2 to +6 dB**
- Ceiling: **-0.5 to -1.0 dB**
- Release: **40-100 ms**
- Stereo Link: **On** (for stereo material)

Typical target: around **1-4 dB** reduction on peaks, with only brief higher hits.

## Notes and Limitations

- The limiter uses **fixed look-ahead (3 ms)**.
- There is **no hard-clip safety stage**.
- Ceiling is sample-peak based; for strict true-peak delivery, verify in final export/mastering.

## Troubleshooting

- **Pumping:** increase Release or reduce Input.
- **Not enough limiting:** increase Input or raise Ceiling (less negative).
- **Unstable stereo image:** set Stereo Link to **On**.

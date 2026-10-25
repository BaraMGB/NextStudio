# Peak Limiter

## Overview

**Peak Limiter** is NextStudio's built-in transparent brickwall-style limiter for mono and stereo signals. It uses fixed look-ahead to react to short peaks while preserving the overall signal character.

| Property | Value |
|---|---|
| Name in NextStudio | `Peak Limiter` |
| Persistent type ID | `peak_limiter` |
| Channel modes | mono and stereo |
| Look-ahead | fixed at 3.0 ms |

## Typical uses

- final peak control on an individual track;
- transient overshoot protection before a bus or master stage;
- increasing average level while constraining sample peaks;
- protecting later processors from unexpectedly high peaks.

A limiter is not a substitute for healthy gain staging. Excessive gain reduction can create pumping, distortion, or softened transients.

## Signal flow

Conceptually:

```text
Input
→ input gain
→ look-ahead peak detection / gain control
→ ceiling-constrained output
→ output
```

The look-ahead introduces latency so the limiter can reduce gain before a detected peak reaches the output.

## Controls

### Input

Input gain boosts or attenuates the signal before limiter gain control.

- range: `-24.0 dB` to `+24.0 dB`;
- default: `0.0 dB`.

Increasing Input drives the signal further into the limiter and normally causes more gain reduction. Reducing it can restore transient shape when the limiter is working too hard.

### Ceiling

Ceiling sets the target maximum sample-peak output level.

- range: `-12.00 dB` to `-0.01 dB`;
- default: `-0.30 dB`.

A lower ceiling leaves more headroom. For material that will undergo encoding, sample-rate conversion, or additional processing, leave sufficient margin and verify the final result with appropriate metering.

### Release

Release controls how quickly gain returns after attenuation.

- range: `5.0 ms` to `500.0 ms`;
- default: `80.0 ms`.

Short values recover quickly and can sound aggressive or produce pumping on low-frequency material. Longer values are smoother but can keep subsequent signal attenuated and soften transient contrast.

### Stereo Link

Stereo Link controls whether stereo channels share gain reduction.

- **On** by default: both channels use linked reduction, preserving stereo position during peaks;
- **Off**: channels are processed independently, which can reduce unnecessary attenuation on the quieter channel but may shift the stereo image.

For buses, full mixes, and most stereo tracks, linked mode is the safer starting point.

## Metering

The plug-in exposes three live measurements:

- **IN** — peak level at limiter input;
- **OUT** — peak level at limiter output;
- **GR** — maximum gain reduction in the current processing block.

Use gain reduction as a diagnostic rather than a target by itself. The audible result depends on source dynamics, release, frequency content, and how often reduction occurs.

## Starting points

### Transparent peak control

| Control | Starting value |
|---|---|
| Input | `0 dB` |
| Ceiling | `-0.3 dB` |
| Release | `80–120 ms` |
| Stereo Link | On |

Reduce Input if gain reduction is frequent or sustained.

### Increased loudness on one track

| Control | Starting value |
|---|---|
| Input | `+2` to `+6 dB` |
| Ceiling | `-0.5` to `-1.0 dB` |
| Release | `40–100 ms` |
| Stereo Link | On for stereo material |

A reasonable initial aim is brief `1–4 dB` reduction on peaks, then adjust by ear in the mix.

### Percussive material

Start with moderate input drive and a release long enough to avoid chatter between closely spaced transients. If attacks lose impact, reduce Input or lengthen Release.

### Bass-heavy material

Low frequencies can hold the detector above threshold for longer. Increase Release if the level audibly modulates each cycle; reduce drive if the limiter removes too much body.

## Workflow

1. Insert Peak Limiter late in the track or bus chain.
2. Set Ceiling for the desired headroom.
3. Start with Input at `0 dB` and Release around `80 ms`.
4. Play the loudest section.
5. Increase Input only until the intended amount of peak control is reached.
6. Compare bypassed and processed signals at similar perceived loudness.
7. Listen for pumping, dull transients, stereo-image movement, or distortion.
8. Recheck the final render with suitable peak/true-peak tools for the delivery format.

## Limitations

- Look-ahead is fixed at 3 ms and is not user-adjustable.
- There is no additional hard-clip safety stage.
- Ceiling control is sample-peak based, not a guarantee of true-peak compliance.
- Inter-sample peaks may exceed what sample-peak meters indicate after conversion or lossy encoding.
- Heavy limiting can still sound distorted even when numerical peaks stay below the ceiling.

For strict broadcast or streaming delivery, verify the rendered file with dedicated loudness and true-peak metering.

## Troubleshooting

### Pumping or breathing

- increase Release;
- reduce Input;
- reduce low-frequency energy before limiting if appropriate;
- use less peak reduction.

### Transients sound dull

- reduce Input;
- try a longer Release;
- use the limiter only for occasional peaks;
- reconsider earlier compression or clipping in the chain.

### Not enough limiting

- increase Input;
- lower Ceiling if the target output must be quieter;
- verify the plug-in is enabled and receives the expected channel signal.

### Stereo image moves

Enable Stereo Link. Independent channel gain reduction can alter the apparent position of asymmetric peaks.

### Output exceeds a downstream meter target

- confirm whether the downstream meter reports true peak rather than sample peak;
- leave a lower ceiling;
- inspect processors placed after the limiter;
- verify the final encoded/rendered file, not only live playback.

## Related source files

- `App/include/PeakLimiterPlugin.h`
- `App/src/PeakLimiterPlugin.cpp`
- `App/include/PeakLimiterPluginComponent.h`
- `App/src/PeakLimiterPluginComponent.cpp`

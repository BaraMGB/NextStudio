# Reverb

## Overview

| Property | Value |
|---|---|
| Type ID | Tracktion Engine Standard Reverb |
| Category | Effect |
| Channels | Stereo only |
| Engine | JUCE reverb engine |

## Controls

| Parameter | Description |
|---|---|
| Room Size | Simulated space size |
| Damping | High-frequency damping — higher values dampen more |
| Wet | Amount of reverb signal in the output |
| Dry | Amount of original (unaffected) signal in the output |
| Width | Stereo width of the reverb tail |
| Mode | Selects different reverb algorithms |

## Chamber display

The chamber visualization reacts to the Reverb parameters. Its header shows only **REVERB CHAMBER** so the title remains readable at narrow Track Chain widths. Current Wet, Dry, and Freeze values are shown by their parameter controls and are not duplicated in the header.

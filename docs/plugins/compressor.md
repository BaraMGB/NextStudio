# Compressor

## Overview

| Property | Value |
|---|---|
| Type ID | Tracktion Engine Standard Compressor |
| Category | Effect |
| Channels | Mono, Stereo |
| Sidechain | Yes (any track selectable as source) |

## Controls

| Parameter | Description |
|---|---|
| Threshold | Level at which compression begins |
| Ratio | Compression ratio (e.g. 4:1) |
| Attack | Response time — how fast compression engages |
| Release | Time until compression recedes |
| Output | Make-up gain applied after compression |

## Sidechain

| Parameter | Description |
|---|---|
| Source | Dropdown to select any track as sidechain input |
| Trigger | Toggle to enable/disable sidechain processing |
| Sidechain Gain | Amplification of sidechain signal before compression |

## Layout

The transfer graph is placed to the left of two rows of parameter knobs. Sidechain source and trigger controls use a separate footer across the full editor width. This keeps parameter names, values, and knobs readable while the Track Chain is narrow or wide.

## Graph

Live transfer curve (input vs output). Threshold, ratio, and output gain are visually recognizable as the knee point, slope change, and vertical offset respectively.

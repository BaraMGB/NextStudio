# EQ

## Overview

| Property | Value |
|----------|-------|
| Type ID | Tracktion Engine Standard EQ |
| Category | Effect |
| Channels | Mono and stereo |
| Bands | 4 (Low, Mid 1, Mid 2, High) |

Four-band parametric equalizer with interactive frequency response graph.

## Controls

| Parameter | Range | Description |
|-----------|-------|-------------|
| Frequency | Hz | Center frequency of the band |
| Gain | dB | Boost or cut amount |
| Q | — | Bandwidth (higher = narrower) |

Each of the four bands (Low, Mid 1, Mid 2, High) exposes the same three parameters.

## Graph

| Interaction | Action |
|-------------|--------|
| Drag | Move frequency (horizontal) and gain (vertical) of nearest band |
| Mouse wheel | Adjust Q of nearest band |
| Double-click | Reset the nearest band's frequency, gain, and Q to factory defaults |
| Right-click | Open a menu containing **reset values** for the nearest band |

Both reset interactions update the graph and audio parameters immediately. The complete band reset is one undoable action. The interactive frequency response curve updates in real time as parameters change.
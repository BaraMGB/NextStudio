# EQ band reset interactions

## Problem

The four handles in `EqResponseGraphComponent` supported drag editing for frequency and gain and wheel editing for Q, but the component had no double-click handler or reset command. Consequently, none of the bands could be returned directly to its factory values from the graph.

## Implementation

`EqResponseGraphComponent` now offers the same whole-band reset through two interactions:

- double-click a band handle;
- right-click a band handle and choose **reset values**.

A reset applies frequency, gain, and Q together. `EqBandReset` reads each attached Tracktion parameter's `getDefaultValue()` instead of duplicating factory constants in the UI. It sends synchronous parameter notifications so the response graph and audio filters update immediately.

The three changes are grouped in one named undo transaction. One Undo restores all previous values for that band, and one Redo reapplies all three factory defaults.

## Factory defaults

| Band | Frequency | Gain | Q |
|---|---:|---:|---:|
| Low | 80 Hz | 0 dB | 0.5 |
| Mid 1 | 3000 Hz | 0 dB | 0.5 |
| Mid 2 | 5000 Hz | 0 dB | 0.5 |
| High | 17000 Hz | 0 dB | 0.5 |

## Regression coverage

`EqBandResetTests` creates Tracktion's actual 4 Band Equalizer and verifies all four bands, parameter-owned defaults, synchronous notifications, and single-step undo/redo.

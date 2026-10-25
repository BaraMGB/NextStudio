# Spectrum Analyzer

## Overview

| Property | Value |
|----------|-------|
| Type ID | `spectrum_analyzer` |
| Category | Effect |
| Channels | Mono and stereo |
| Signal | Pass-through (unchanged) |

Real-time frequency spectrum display. Signal passes through unmodified.

## Properties

| Property | Value |
|----------|-------|
| FFT Size | 4096 (Order 12) |
| Bins | 256 |
| Frequency Range | 10 Hz to Nyquist |
| Level Range | −96 to 0 dB |

## Display

| Property | Value |
|-----------|-------|
| Frequency Axis | Logarithmic |
| Attack | 60 ms |
| Release | 200 ms (peak-hold) |

Runs automatically when audio signal is present. No user controls — purely visual.
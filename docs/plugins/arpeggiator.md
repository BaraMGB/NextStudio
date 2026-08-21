# Arpeggiator

## Overview

| Property | Value |
|----------|-------|
| Type ID | `arpeggiator` |
| Category | MIDI Effect |
| Audio | No (MIDI only) |
| Placement | MIDI plugin section of Track Chain |

Turns held notes into rhythmic patterns. Incoming MIDI notes are arpeggiated according to the selected mode and rate.

## Controls

| Parameter | Range | Description |
|-----------|-------|-------------|
| Mode | Up, Down, Up/Down, Random | Direction pattern for arpeggiation |
| Rate | Musical note values (1/8, 1/16, etc.) | Speed of arpeggiation steps |
| Octave | Octave range | Number of octaves the pattern spans |
| Gate | 0–100% | Note length relative to rate (shorter = staccato, longer = legato) |
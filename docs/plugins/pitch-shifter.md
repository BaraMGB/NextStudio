# Pitch Shifter

## Overview

| Property | Value |
|---|---|
| Type ID | `pitchShifter` |
| Category | Effect |
| Pitch range | -24 to +24 semitones (two octaves down/up) |

## Controls

**Semitones** shifts the audio's pitch. Positive values raise it, negative values lower it, and zero preserves the original pitch. Fractional semitones remain supported.

The knob stays **continuous**, including fractional semitones, so automation can remain smooth. It uses the standard parameter controls, including its context menu for precise value entry, automation, and MIDI Learn.

## Pitch Map

The compact **PITCH MAP** above the knob shows the configured transposition:

- The scale runs from +24 at the top to -24 at the bottom, with octave marks at +12, 0, and -12.
- The subdued ring at zero represents the original pitch.
- The track-coloured point and connecting arrow show the shifted pitch relative to zero.
- At zero the reference and output markers coincide; fractional shifts move the point continuously.
- The display follows parameter changes, including automation.

### Whole-semitone input

Drag the coloured point **up or down** to adjust the pitch in **whole-semitone steps**. Only this graph input snaps; the knob and native automation parameter remain continuous. The point highlights on hover and shows a vertical-drag cursor.

- Dragging is limited to -24 through +24 semitones, even outside the graph.
- Clicking the point without moving it leaves a fractional value unchanged.
- Release the mouse to finish. Each graph drag is one undo/redo step, restoring the exact previous value, including any fraction.
- Press `Esc` while dragging to cancel and restore the starting value.

This is a schematic interval display, not an audio analyser or a detected-note display.

## Layout

The editor uses a compact rack width like Volume/Pan and Arpeggiator. The map and knob are stacked vertically, with no duplicate parameter row or internal scroll area.

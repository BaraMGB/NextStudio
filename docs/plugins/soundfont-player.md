# SoundFont Player

## Overview

| Property | Value |
|----------|-------|
| Type ID | `soundfont_player` |
| Category | Instrument |
| Output | Stereo |
| MIDI Channels | 16 |
| Max Voices | 256 |
| Engine | TinySoundFont |
| Format | `.sf2` only (no `.sf3`) |

Loads and plays SoundFont files using the TinySoundFont engine. All 16 MIDI channels share a single preset.

## Controls

| Parameter | Range / Type | Description |
|-----------|-------------|-------------|
| Load SF2 | File browser | Loads a `.sf2` file; path stored relative to project |
| Preset | Dropdown | Selects preset from the loaded SoundFont; changing it stops all playing notes |
| Gain | −60.0 to +12.0 dB (default 0.0) | Output gain |
| Panic | Trigger | Immediately stops all notes and sounds |

## MIDI Support

| Message | Behavior |
|---------|----------|
| Note On | Triggers a voice |
| Note Off | Releases a voice |
| All Notes Off | Stops all notes on the channel |
| All Sound Off | Immediately silences all sound |
| Pitch Wheel | Applies pitch bend |
| CC | Passthrough |

## Tips

- SoundFont quality varies widely between files — audition before committing.
- Reduce Gain if output distorts (clipping), especially with dense arrangements.
- Keep `.sf2` files near the project directory for reliable relative path resolution.
# Reverb header simplification

## Problem

The Reverb chamber graph drew `REVERB CHAMBER` and a Wet/Dry/Freeze status string independently into the same header rectangle. At narrow plug-in widths, the two strings overlapped and became unreadable.

## Decision

The status string duplicated values already shown by the Wet, Dry, and Freeze controls. The chamber header therefore displays only its title. Removing the redundant text avoids width-dependent abbreviations or visibility rules and keeps the title readable throughout the supported plug-in layout.

The parameter controls remain the authoritative visible source for the current Wet, Dry, and Freeze values. Reverb processing, automation, presets, and parameter notifications are unchanged.

## Validation

The change is covered by the full application build and test suite. Because it removes a paint-only text operation without adding layout logic, final coverage is a focused visual check of the Reverb at the narrowest available Track Chain width.

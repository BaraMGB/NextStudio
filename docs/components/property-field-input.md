# Property-field input contracts

- Type: reference
- Audience: contributors
- Scope: current shared note/clip numeric-field behavior and explicit parser differences

## Ownership

`NotePropertiesBar` and `ClipPropertiesBar` each implement their own field editor and duration parser. They share the following interaction conventions, **not a single interchangeable parser or mutation model**. Shared bars/beats/ticks conversion lives in `App/include/PositionDisplayHelpers.h` and `App/src/PositionDisplayHelpers.cpp`; model constraints belong in the individual [note](note-properties-bar.md) and [clip](clip-properties-bar.md) references.

## Field interaction

- Fields normally use read-only scrub mode with an up/down cursor. Double-click or focused Enter/F2 enters writable text mode, selects the text and uses an I-beam.
- Enter applies valid input and returns to read-only mode. Invalid Enter retains active red text for correction.
- Escape restores the last model-derived display and releases focus without applying text.
- Focus loss applies valid input; invalid input restores the model-derived display rather than leaving a red editor active.
- Tab/Shift+Tab commits valid text and wraps between fields. Invalid text stays in place; an untouched mixed-value em dash is a no-op.
- A read-only enabled field's wheel applies a discrete step. Vertical scrubbing starts after four pixels, with four pixels per step and upward movement increasing the value. Unbounded movement is enabled during scrubbing.
- Scrubbing retains a provisional plan without model/undo mutation; release applies one final operation. Returning to the original values produces no change. Text/wheel edits apply immediately.
- Empty selections disable fields. At rest, equal values display normally and mixed values display `—`; ordering/reference semantics are component-specific.
- Programmatic text/focus/mode changes use `m_handlingEditorCallback` guards to prevent recursive commit/cancel callbacks. Preserve invalid-state coloring when refreshing theme values.

Timing wheel/scrub steps use the relevant editor's Fixed or Adaptive interval, or one tick with Snap Off. Note pitch/velocity step by semitone/integer. These numeric steps are discrete, not the magnetic canvas mapping.

[Timeline feedback](timeline-snapping.md#snap-feedback-and-live-values) owns the common canvas-preview precedence/cleanup and queued-focus-loss completion contract. Property-bar pages describe their wiring rather than redefining that lifecycle.

## Focus-loss policy and canvas transitions

The rule for **note/clip property fields** is commit-on-leave, not Enter-only:
valid text is applied on Enter, Tab or focus loss; Escape discards it. Invalid
Enter/Tab keeps the editor active for correction, while invalid focus loss
restores the last model-derived value without mutation. This is the existing
policy in `NotePropertiesBar::configureField()` and
`ClipPropertiesBar::configureField()`, not a new global TextEditor default.

Before a user-initiated canvas gesture changes selection, performs a hit test or
captures an edit origin, its owner must synchronously finish writable property
text through the existing interaction-begin callback. In the Piano Roll this
includes both grid and velocity-lane Pointer/Lasso gestures and direct velocity
marker drags. Apply to the **old selection** first, then read the resulting model
and begin the gesture. `TimelineInteractionPreview::finishTextEdit()` uses the
normal focus-loss callback even when JUCE has already moved focus and queued its
notification. The field becomes read-only, so the delayed callback cannot apply
again or consume new selection/preview values. Canceling the subsequent gesture
does not undo an already completed property edit. External deletion/selection
refresh may still discard stale input as described by each bar's lifetime rules.

Other field types deliberately have different policies; do not change them to
match the property bars implicitly:

| Context | Existing behavior / source |
|---|---|
| Note/clip properties | Enter/Tab/focus loss applies valid text; Escape discards (`NotePropertiesBar.cpp`, `ClipPropertiesBar.cpp`) |
| Inline colour hex | Enter/focus loss attempts to apply; Escape restores the colour (`InlineColourEditor.cpp`) |
| Mouse cursor scale setting | Enter/focus loss invokes validation/update (`AudioMidiSettings.h`) |
| Transport/loop position display | Enter confirms; Escape/focus loss discards (`PositionDisplayComponent.cpp`, `beginEditing()`) |
| Project/theme Save As | Enter or the action button performs saving; focus loss alone does not save (`ProjectsBrowser.cpp`, `ThemeSettingsComponent.cpp`) |

## Positions and durations

| Input | Meaning / example |
|---|---|
| Absolute position | `bar.beat.tick`, e.g. `6.2.240`; `6` defaults to beat 1/tick 0, `6.2` to tick 0 |
| Relative Start/End | Signed fraction or ticks, e.g. `+1/16`, `-120 ticks`; not a bars/beats/ticks delta |
| Duration | Whole-note fraction or ticks; a leading sign makes it a delta |

Bars/beats are one-based; ticks are zero-based and less than `te::Edit::ticksPerQuarterNote` (960). Empty components, more than three components, nonpositive bars/beats and out-of-range ticks are rejected. Conversion uses the actual TempoSequence; no transport-tempo approximation or additional fixed-grid assumption is permitted. Formatting uses at least three tick digits and valid PPQ bounds.

Fractions use `beats = 4 × numerator / denominator` with positive integer numerator/denominator. Both components recognize binary durations `1/1`–`1/128` with an epsilon; other values display rounded ticks. Formatting is presentation, never a second model quantization pass.

**Parser difference:** NotePropertiesBar requires a positive tick magnitude, including before reapplying a relative sign. ClipPropertiesBar accepts a zero tick magnitude, though its final plan must still have positive lengths. Thus `+0 ticks` can be a clip no-op but is not valid note duration input. Neither component accepts zero/negative committed duration.

## Note pitch and velocity

Unsigned MIDI numbers `0..127` give an unambiguous absolute pitch. Signed semitone input requires `st`, e.g. `+1 st` or `-12 st`. Note letters `A..G`, one optional `#`/`b`/`B`, and a signed octave are also parsed.

**Known source mismatch:** `GUIHelpers::getMidiNoteName()` displays MIDI 60 as C4, but `NotePropertiesBar::parsePitch()` currently calculates `(octave + 2) × 12 + pitchClass`. Consequently typed `C4` means MIDI 72, not 60; typed `C3` means 60. The previous documentation incorrectly asserted those conventions matched. Use MIDI numbers for exact entry until a separately approved implementation correction aligns parsing/display. This documentation migration does not change the parser.

Velocity uses unsigned absolute or signed relative integers. Component-specific clamp rules and multi-note semantics are in NotePropertiesBar; do not infer the velocity-lane clamp from the property parser.

## Other shared position-display helpers

Transport/loop display consumers use the same bars/beats/ticks helpers. `PositionDisplayHelpers` also owns strict overflow-checked integers, finite doubles, BPM formatting, clock time and time signatures:

- Clock parsing accepts seconds, minutes:seconds or hours:minutes:seconds, comma/dot decimals and a leading minus. Colon-separated minutes/seconds obey their implemented ranges; a negative parsed time may then be clamped by the caller.
- Time-signature parsing accepts numerator 1–64 and denominator 1, 2, 4, 8, 16, 32 or 64. BPM displays two decimal places; value validation is caller-specific.
- The property components' local digit-validated `getIntValue()` parsers do **not** provide the shared integer helper's explicit overflow diagnostics. Do not describe all these parsers as identical.

## Validation and limitations

`App/tests/PositionDisplayTests.cpp` covers the production shared position-display helpers. `TimelineSnappingTests` covers the production queued TextEditor completion helper with a test callback, not every full property-bar parser/commit path. There is no dedicated NotePropertiesBar/ClipPropertiesBar end-to-end target.

`tools/native-velocity-selection-regression.py` additionally exercises real note-field focus loss into Pointer/Lasso selection and marker dragging, including invalid input, Escape, gesture cancellation and undo/redo. This is scoped Linux/X11 integration coverage, not a complete property-bar suite.

Focused validation should include mixed selections, offset clips, invalid input, no-op scrub, queued focus changes and undo/redo. Future isolated parser/planner tests should call production code, not copy these rules into a parallel implementation. Accessibility, extreme widths, integer overflow and the pitch-name mismatch remain limitations, not fixed by rewriting documentation.

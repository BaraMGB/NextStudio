# NotePropertiesBar

## Purpose

`NotePropertiesBar` is a compact editor for MIDI notes selected in the Piano Roll. It presents exact values for multiple selected notes without opening a separate inspector.

The component is declared and implemented in:

- `App/include/NotePropertiesBar.h`
- `App/src/NotePropertiesBar.cpp`

It is owned by `PianoRollEditor` and occupies a 30-pixel-high row between the Piano Roll tool header and timeline.

## Fields

The bar displays the selection count and five editable properties:

| Field | Meaning | Display/absolute input | Relative input |
|---|---|---|---|
| `START` | global project start | bars, beats, ticks; e.g. `6.2.240` | `+1/16`, `-120 ticks` |
| `END` | global project end | bars, beats, ticks | `+1/16`, `-120 ticks` |
| `DURATION` | note length | note fraction or ticks | `+1/16`, `-120 ticks` |
| `PITCH` | MIDI note number/name | `60`, `C3`, `G#4`, `Bb2` | `+1 st`, `-12 st` |
| `VELOCITY` | MIDI velocity | integer | `+5`, `-10` |

The left side displays `SELECTED NOTES:` and the number of valid selected note/clip pairs. Two combo boxes immediately after the Velocity field configure new-note behavior:

- `SNAP` selects Off, a fixed note value (`1/1`–`1/128`), or Adaptive zoom-dependent position snapping;
- `INSERT LENGHT` independently selects Adaptive, Last Inserted, or a fixed note value (`1/1`–`1/128`) for newly created notes.

Adaptive length uses the zoom-dependent interval independently of the current SNAP choice. Both modes persist in edit-local UI state, with Last Inserted as the default length mode.

## Public interface

```cpp
explicit NotePropertiesBar(EditViewState&);

void setSelectionProvider(SelectionProvider);
void refreshFromSelection(bool discardActiveEdit = false);
void clearSelection();
void updateColours();
```

### `SelectionProvider`

```cpp
using SelectionProvider =
    std::function<juce::Array<std::pair<te::MidiClip*, te::MidiNote*>>() >;
```

The bar does not own or discover selection globally by itself. Its owner injects a provider returning each selected note together with its owning clip. The clip is required for coordinate conversion and mutation.

### `refreshFromSelection()`

Reads the provider, removes null pairs, computes the display values, enables/disables fields, and repaints.

When `discardActiveEdit` is true, an active text edit is abandoned only if the actual ordered selection changed. This distinction is important because `MidiViewport` can broadcast tool changes and delayed notifications that do not represent a different set of notes.

### `clearSelection()`

Clears cached pairs, replaces all values with an em dash (`—`), returns editors to read-only scrub mode, disables labels/editors, clears invalid state, and repaints.

### `updateColours()`

Reads current theme colors from `ApplicationViewState`, applies label/text colors, preserves red invalid-state text, and repaints.

## Internal structure

### `Field`

Each property is represented by:

```cpp
struct Field
{
    Property property;
    juce::Label label;
    PropertyEditor editor;
    juce::String displayedText;
    juce::Rectangle<int> bounds;
    bool invalid;
};
```

`displayedText` is the last model-derived presentation value. It is used to restore text after cancellation or invalid focus loss.

### `PropertyEditor`

`PropertyEditor` derives from `juce::TextEditor` and adds semantic callbacks for:

- Tab/Shift+Tab navigation;
- mouse-wheel steps;
- focus gain;
- beginning text entry;
- beginning/updating/ending drag scrubbing.

The editor has two interaction modes:

1. **read-only scrub mode** — up/down cursor, wheel and vertical drag modify the property;
2. **text mode** — I-beam cursor and normal `TextEditor` editing behavior.

## Selection integration

`PianoRollEditor` installs a provider that:

1. obtains the viewport's current cached MIDI clips;
2. iterates selected note pointers;
3. searches each current clip sequence for the pointer;
4. returns only pairs still present in the model.

This is deliberately more defensive than blindly calling `SelectedMidiEvents::clipForEvent()`. A note-removal notification can be delivered after the model object has been removed, and raw Tracktion note pointers are no longer valid model members at that point.

The bar also removes null pairs as a final guard.

## Multiple selection

### Common values

For each property, `refreshFromSelection()` reads the first selected note and compares all remaining notes. Floating-point timing values use an epsilon of `1e-7`; integer values are converted for the same generic comparison helper.

If every value is equal, the common value is formatted. Otherwise the field displays `—`.

### Applying mixed values

Editing a mixed field applies the entered value to every selected note. Semantics depend on input:

- absolute value: set the same target value or endpoint;
- relative value: apply the same delta to each note's existing value.

For example, an absolute `END` gives all notes the same global endpoint and therefore potentially different lengths. Relative `END +1/16` adds the same duration to each note.

### Selection changes during editing

A model refresh does not overwrite a focused active editor unless the caller requests discard and the note selection actually changed. This prevents unrelated repaint/tool notifications from destroying text the user is typing.

When selection genuinely changes, focused editors return to read-only mode and release keyboard focus before new values are displayed.

## Interaction behavior

### Enter text mode

Text mode begins with either:

- double-click on a read-only field;
- `Enter` or `F2` while a read-only field has focus.

The editor becomes writable, uses the I-beam cursor, grabs keyboard focus, and selects all text. If the displayed value is mixed (`—`), the bar clears it before entry.

### Commit

`Enter` calls `commit()`:

1. validate and apply the input;
2. if invalid, keep editing and mark the field red;
3. if valid, return to read-only mode;
4. release keyboard focus;
5. refresh model-derived values.

### Cancel

`Escape` restores `displayedText`, clears invalid state, returns to read-only mode, and releases focus.

### Focus loss

If a writable editor loses focus:

- valid input is applied and values are refreshed;
- invalid input is silently restored to the previous model-derived display value;
- the editor returns to read-only mode.

This differs from pressing Enter on invalid input, which keeps the field active and red so it can be corrected.

### Tab navigation

- read-only field: move focus to adjacent property;
- writable valid field: commit, refresh, and move;
- writable invalid field: remain in place and show red;
- untouched mixed field: treat as no change and move.

Navigation wraps from the last field to the first and vice versa.

### Mouse wheel

A wheel event on a read-only enabled field applies one positive or negative step based on wheel direction. Each wheel change can begin its own undo transaction.

### Vertical drag scrubbing

A read-only field begins scrubbing after four vertical pixels. Every four pixels correspond to one step. Upward drag increases and downward drag decreases.

Once active, unbounded mouse movement is enabled so screen edges do not stop the gesture. Dragging only updates an in-memory edit plan, the displayed field values, and translucent note previews in `MidiViewport`; it does not mutate Tracktion note state or open an undo transaction. Releasing the mouse commits the final plan as one transaction. Returning to the original value before release produces no model or undo change.

## Scrub step sizes

| Property | Step input generated internally |
|---|---|
| Start | `±N × current SNAP interval` |
| End | `±N × current SNAP interval` |
| Duration | `±N × current SNAP interval` |
| Pitch | `±N st` |
| Velocity | signed integer delta |

The parser receives the generated relative string through the same `apply()` path as typed input. Fixed SNAP uses the selected note value, Adaptive uses the current zoom-dependent interval, and Off uses one tick. Pitch and Velocity retain semitone/integer stepping.

## Parsing and formatting

### Strict integers

The component-local integer parser:

- trims whitespace;
- accepts one optional leading `+` or `-`;
- requires at least one digit;
- rejects every non-digit after the sign;
- converts with `juce::String::getIntValue()`.

Position parsing uses the stricter shared parser in `PositionDisplayHelpers` indirectly.

### Bars, beats, ticks

Absolute start/end values use shared helpers:

```cpp
PositionDisplayHelpers::formatBarsBeatsTicks(...)
PositionDisplayHelpers::parseBarsBeatsTicks(...)
```

The format is:

```text
bar.beat.tick
```

Bar and beat are one-based. Tick is zero-based and must be less than `te::Edit::ticksPerQuarterNote` (currently 960).

Accepted examples:

- `6` → bar 6, beat 1, tick 0;
- `6.2` → bar 6, beat 2, tick 0;
- `6.2.240` → complete input.

Rejected examples include empty components, more than three components, bar/beat below one, negative ticks, and ticks at or above PPQ.

Conversion uses the edit's internal tempo sequence, so tempo and time-signature changes are respected.

### Durations

Duration input accepts either a fraction or positive ticks.

Fractions represent a portion/multiple of a whole note:

```text
beats = 4 × numerator / denominator
```

Examples:

| Input | Quarter-note beats |
|---|---:|
| `1/1` | 4.0 |
| `1/2` | 2.0 |
| `1/4` | 1.0 |
| `1/8` | 0.5 |
| `3/16` | 0.75 |

Tick form requires the suffix `ticks`, for example `120 ticks`. The parsed tick count must be greater than zero before a relative sign is reapplied by `apply()`.

Formatting recognizes binary values from `1/1` through `1/128` using epsilon comparison. Other lengths are rounded to the nearest tick and displayed as `<N> ticks`.

### Pitch

Absolute pitch accepts:

- MIDI number `0..127` without a sign;
- note letter `A..G`;
- optional sharp `#` or flat `b`/`B`;
- signed octave number after the note name.

The octave mapping is:

```text
MIDI note = (octave + 2) × 12 + pitch class
```

Therefore `C3` maps to MIDI note 60 in the current display convention.

Relative pitch requires a signed integer and the suffix `st`, for example `+1 st` or `-12 st`.

### Velocity

Velocity input is a strict integer. A leading sign makes it relative; unsigned input is absolute.

## Validation

`apply()` first resolves the current selection again. It parses once, then validates and plans the complete operation against every selected note before modifying any note.

This all-or-nothing validation prevents partial multi-note edits. Text commits and wheel steps apply the plan immediately; drag scrubbing retains the plan provisionally until mouse-up.

### Start

- absolute target or relative result must be at or after global beat zero;
- changing start preserves the note's existing duration.

### End

- absolute endpoint must be after each note's start;
- relative endpoint editing is implemented as a duration delta;
- every resulting duration must be greater than zero.

### Duration

Every resulting duration must be greater than zero.

### Pitch

- absolute pitch must be in `0..127`;
- relative results are clamped to `0..127` during application.

### Velocity

Absolute and relative results are clamped to `1..127` during application.

The separate `VelocityEditor` currently clamps drag editing to `0..127`, so velocity-zero behavior differs between the two components.

## Position coordinate conversion

Tracktion MIDI notes store their start in clip-content beat coordinates. The property bar displays global project beat coordinates.

### To global beat

```text
global start = clip start + note start - clip offset
```

Implemented by `getGlobalStart()`.

### Back to internal beat

```text
internal start = global start - clip start + clip offset
```

Implemented by `getInternalStart()`.

This conversion is essential for clips with non-zero content offsets and for editing notes from multiple clips at once.

## Applying properties

All mutations use the edit undo manager. Each plan captures the source note's complete state plus its destination start, length, pitch, and velocity.

Start, End, Duration, and Pitch can create same-pitch timing conflicts. Their commit path therefore uses the same destination-priority behavior as Piano Roll dragging:

1. remove all edited source notes;
2. clear existing notes under the grouped destination ranges with `MidiViewport::cleanUnderNoteRanges()`;
3. resolve conflicts between planned destinations in selection order;
4. recreate and select the resulting notes from full state copies.

This preserves custom note properties while ensuring committed notes do not overlap. Velocity cannot create timing conflicts and is applied directly with `setVelocity()`, clamped to `1..127`.

## Undo transactions

Property operations use semantic transaction names:

| Property | Transaction |
|---|---|
| Start | `Move MIDI Notes` |
| End | `Change MIDI Note Duration` |
| Duration | `Change MIDI Note Duration` |
| Pitch | `Change MIDI Note Pitch` |
| Velocity | `Change MIDI Note Velocity` |

Text commits and wheel changes begin transactions per application. Drag scrubbing begins its single transaction only on mouse-up; preview updates never touch the undo manager.

## Model synchronization

`PianoRollEditor` refreshes the bar in response to:

- `SelectedMidiEvents` changes forwarded by `MidiViewport`;
- global `SelectionManager` changes;
- NOTE property changes;
- note child removal;
- clip-set updates;
- active-track clearing/switching;
- theme changes.

NOTE property changes are coalesced with `FlaggedAsyncUpdater`, so a multi-note operation does not synchronously recompute the bar for every property notification.

When the active track is cleared, the old `SelectedMidiEvents` object is explicitly deselected without reselecting the old track, then the bar clears its cached selection.

## Layout

The component uses text measurement through `juce::GlyphArrangement`. Stable reference strings estimate useful widths:

- positions: `88.8.888`;
- duration: `888 ticks`;
- pitch: `G#10`;
- velocity: `888`;
- selection count: `8888`.

A responsive area for the SNAP and INSERT LENGHT controls is placed immediately after the property panel, so the controls follow the Velocity field instead of being anchored to the right edge. At preferred width, the property fields receive their measured content width plus padding. When space is constrained:

1. the two combo-box areas shrink to their compact allocation;
2. selection-count area is allocated first within the property panel, up to available width;
3. remaining field width excludes fixed inter-field gaps;
4. every property receives a proportional share based on preferred width;
5. the final field receives the rounding remainder.

This prevents rightmost fields such as Pitch and Velocity from collapsing to zero merely because earlier fields consumed all available space.

Within each field, padding is reduced safely for narrow widths, then label and editor split the remaining content.

## Theme and drawing

Colors come from `ApplicationViewState`:

- labels use button text color at 85% alpha;
- enabled values use full button text color;
- disabled selection count and fields use reduced opacity;
- invalid text and focus outline use red;
- active writable editor gets a rounded one-pixel focus outline;
- vertical border-colour separators distinguish the selection count, each note-property field, SNAP, and INSERT LENGHT.

Editor backgrounds, selection highlight, and built-in outlines are transparent so the Piano Roll row background remains visible.

## Callback recursion guard

`m_handlingEditorCallback` suppresses callbacks while the component programmatically changes editor text, focus, mode, or invalid state. Without this guard, `setText()`, focus changes, or selection transitions could recursively trigger commit/cancel behavior.

Any new programmatic editor mutation should consider whether it must be wrapped with this guard.

## Known limitations and follow-up opportunities

- Component parsing/application behavior does not yet have a dedicated test target; inserted-note length resolution is covered separately by `PianoRollNoteLengthTests`.
- The component holds raw model pointers between refreshes; current integration filters validity, but future asynchronous model changes must preserve that invariant.
- Integer conversion uses JUCE's `getIntValue()` after digit validation and does not explicitly report overflow.
- Velocity range differs from `VelocityEditor` at zero.
- Accessibility metadata and screen-reader-specific descriptions are not yet documented or tested.
- Extremely narrow widths keep fields present but cannot guarantee readable labels and values.
- Relative start/end notation accepts note fractions and ticks, not bars/beats/ticks deltas.

## Test recommendations

A future testable extraction should cover:

- duration parsing/formatting, including arbitrary fractions and tick rounding;
- pitch name parsing, accidentals, octave boundaries, and invalid spellings;
- absolute versus relative operation planning;
- all-or-nothing multi-note validation;
- clip-offset/global-position conversion;
- mixed-value detection epsilon;
- layout allocation at preferred, narrow, and extremely narrow widths;
- velocity-zero consistency decision.

## Related documents

- [Piano Roll](../user/piano-roll.md)
- [State and Event Model](../architecture/state-and-events.md)
- [Change document](../changes/note-properties-bar-and-position-display.md)

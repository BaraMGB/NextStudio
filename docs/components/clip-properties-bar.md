# ClipPropertiesBar

## Purpose

`ClipPropertiesBar` is the compact clip inspector above the arrangement timeline. It edits the position and length of the clips selected in the Song Editor and also owns the arrangement snapping and MIDI-clip insertion-length controls.

The implementation is located in:

- `App/include/ClipPropertiesBar.h`
- `App/src/ClipPropertiesBar.cpp`
- `App/include/ClipPropertyEdit.h`

`EditComponent` owns the bar and places it in a 30-pixel row between the arrangement toolbar/header and timeline.

## Fields and controls

The left side displays `SELECTED CLIPS:` and the current clip count. Three fields edit the selection:

| Field | Meaning | Absolute input | Relative input |
|---|---|---|---|
| `START` | Start of the reference clip | bars, beats, ticks, e.g. `6.2.240` | fraction or ticks, e.g. `+1/16`, `-120 ticks` |
| `END` | End of the reference clip | bars, beats, ticks | fraction or ticks |
| `DURATION` | Duration of the reference clip | fraction or ticks | fraction or ticks |

The reference clip is the first clip in the ordered selection. A committed edit calculates one shared move or resize delta from that clip and applies the same delta to every selected clip. Consequently, relative spacing and length differences in a multi-clip selection are retained.

Two controls follow the fields:

- `SNAP` — Off, Adaptive, or a fixed value from `1/1` through `1/128`;
- `INSERT LENGTH` — Adaptive or a fixed value from `1/1` through `1/128` for newly created MIDI clips.

Arrangement settings are independent from the equivalent Piano Roll settings. They persist in `EditViewState` as `clipSnapMode`, `clipSnapDenominator`, `clipInsertLengthMode`, and `clipInsertLengthDenominator`. Defaults are Adaptive snapping and a fixed `1/1` insertion length.

## Interaction

Arrangement clip move/copy/resize/stretch uses [shared magnetic mouse snapping](timeline-snapping.md): nearby targets lock exactly, but continued movement reaches between-grid positions. `ClipGestureLimits` supplies the same effective delta for preview and commit. Shift/Snap Off bypass attraction. The property fields below keep their existing exact/discrete parsing and wheel/scrub steps; MIDI-clip click insertion defaults are unchanged.

A property field normally remains read-only and uses an up/down cursor.

- Double-click, or press `Enter`/`F2` while focused, to enter text mode.
- Press `Enter` to commit or `Escape` to restore the displayed value.
- `Tab` and `Shift+Tab` commit valid input and move between fields.
- Use the mouse wheel for snapped one-step changes.
- Drag vertically to scrub; four pixels correspond to one step.

Invalid active input is displayed in red. Empty selections disable the fields and show an em dash (`—`). If selected clips do not share a displayed value, that field also shows `—`.

## Parsing and musical time

Absolute positions use `PositionDisplayHelpers::parseBarsBeatsTicks()` and therefore follow the edit tempo and time-signature sequence. Bars and beats are one-based; ticks are zero-based.

Durations accept:

- fractions such as `1/4`, `3/16`, or `1/1`;
- non-negative tick values such as `120 ticks`.

Fractions are converted with:

```text
beats = 4 × numerator / denominator
```

Absolute start/end input is rejected when parsing fails. A resulting clip may not begin before project time zero, exceed Tracktion's maximum edit end, or have a non-positive duration.

## Preview and commit flow

The component separates planning from mutation with `ClipPropertyEdit`:

```cpp
struct ClipPropertyEdit
{
    te::Clip* clip;
    te::ClipPosition position;
};
```

Vertical drag scrubbing builds a provisional array of these edits. `EditComponent` forwards it to `SongEditorView::setClipPropertyPreview()`, which draws translucent clip bodies with white outlines. The Tracktion model and undo history are unchanged while scrubbing. Mouse-up commits the final plan; returning to the original values produces no mutation.

Text and wheel edits commit immediately. Start changes delegate to `EngineHelpers::moveSelectedClips()`. End and Duration changes delegate to `EngineHelpers::resizeSelectedClips()`. The corresponding pure planning helpers, `calculateSelectedClipMove()` and `calculateSelectedClipResize()`, are shared with the preview path so preview and commit use the same constraints.

Existing arrangement overwrite and playback-graph safeguards remain in the underlying move/resize commands.

## Live canvas values

`setInteractionPreview(std::optional<ClipTimingPreview>)` is display-only. The
snapshot has the grabbed reference's actual time range and real selection count,
with no model pointers. `SongEditorView` and the Piano Roll clip overlay route it
through `EditViewState::clipInteractionPreviewChanged`; `EditComponent` installs
and disconnects the callback. `ClipGestureLimits::previewRange()` is also used by
the ghost so Start/End/Duration agree, including seconds-length moves under tempo
changes. Multi-selection canvas dragging shows `CLIPS (REF):`; ordinary selection
and numeric group edits retain existing common-value/em-dash behavior.

While dragging, values receive only a subtle font tint. No Preview label, badge or
underline is added. The display path does not invoke edit/commit callbacks. Existing
text edits finish synchronously through their normal focus-loss commit/reject
policy before reading the canvas gesture origin, even if JUCE has already moved
focus and queued the notification. The later notification sees a read-only field
and cannot commit again or apply preview text;
fields cannot start a competing text/wheel/scrub edit while its snapshot is active.
Snap controls remain available. Model refreshes cannot overwrite the live snapshot.
Commit/cancel clears it and refreshes model values/color; theme updates apply the
color to existing TextEditor glyphs as well as future text. Duration layout reserves
room for six-digit tick counts instead of silently clipping ordinary four-digit values.

`setSnapFeedback()` changes only the compact status under SNAP, not its combo choice.
It uses the originating arrangement timeline; Piano Roll overlay feedback stays in
the Piano Roll header even though clip values appear here.

## Selection and refresh

`ClipPropertiesBar` reads clips directly from the shared `SelectionManager`. `EditComponent` refreshes it when:

- the global selection changes;
- a selected clip state changes;
- the theme changes.

A selection change can discard an active edit. Unrelated model refreshes preserve focused editor text.

## Snapping and MIDI-clip insertion

`TimeLineComponent` selects either arrangement or Piano Roll settings depending on `m_usePianoRollSnapSettings`. In arrangement mode:

- Off disables snapping;
- Fixed resolves to `4 / denominator` quarter-note beats;
- Adaptive uses the zoom-dependent best snap interval.

`TimeLineComponent::getClipInsertLength()` resolves the independent insertion-length mode. `SongEditorView` and `TrackLaneComponent` use this value when creating MIDI clips, and clip starts are quantized through the arrangement timeline.

## Related documents

- [Getting Started](../user/getting-started.md)
- [Architecture Overview](../architecture/overview.md)
- [Central Clip Overwrite Command](../architecture/clip-overwrite-command.md)
- [State and Event Model](../architecture/state-and-events.md)

# ClipPropertiesBar

- Type: reference
- Audience: contributors
- Scope: current arrangement numeric clip editing and inspector integration

## Purpose and source

`EditComponent` owns this compact inspector between arrangement toolbar and timeline. It edits Start, End and Duration for selected clips, audio-clip Gain, and presents arrangement snap/MIDI-clip insertion controls. User-facing controls are in [Song Editor](../ui/song-editor.md#clip-properties-bar).

Source: `App/include/ClipPropertiesBar.h`, `App/src/ClipPropertiesBar.cpp`, `App/include/ClipPropertyEdit.h`, and wiring in `App/src/EditComponent.cpp`.

The [property-field input reference](property-field-input.md) owns shared text/focus/Tab/wheel/scrub conventions and numeric formats; this page owns clip-specific reference, planning and model constraints. Do not assume the note and clip bars apply absolute multi-selection values identically.

## Fields, reference and settings

The count reflects the selected clips. Idle fields display common values or `—` for mixed values; an empty selection disables them. Numeric edits use the first clip in ordered selection as their reference, deriving one move/right-resize **time delta** and applying it to the whole selection. Absolute input does not independently assign every clip the same start/endpoint. Shared deltas retain relative time relationships, while displayed beat durations can change under variable tempo.

`SNAP` is Off, Adaptive or fixed `1/1`–`1/128`. `INSERT LENGTH` is Adaptive or fixed for new MIDI clips. They persist in `EditViewState` as `clipSnapMode`, `clipSnapDenominator`, `clipInsertLengthMode`, and `clipInsertLengthDenominator`; defaults are Adaptive snap and fixed `1/1` insertion. Arrangement settings are independent of Piano Roll settings. `TimeLineComponent` selects ownership using `m_usePianoRollSnapSettings` and resolves clip insert length; creation uses the resolver's corrected downward anchor.

Canvas movement uses [magnetic mouse snapping](timeline-snapping.md). Numeric field steps remain discrete; never soft-snap a typed value or apply a second magnetic pass to a resolved ghost.

## Planning, constraints and commit

`createEditPlan()` parses once, validates selection/base associations and computes the reference's new global start/end using the actual TempoSequence. Duration in beats is derived from both actual endpoints, not an absolute-time conversion of a duration.

- Start yields one seconds-domain move delta.
- End/Duration yield one right-edge resize delta.
- `EngineHelpers::calculateSelectedClipMove()` / `calculateSelectedClipResize()` supply the same feasible positions used by preview and owner commit.
- Plans must contain the complete selected group, valid member clips, positive lengths, nonnegative starts and endpoints within Tracktion's maximum edit end. Existing source/collision limits can constrain the effective delta; do not advertise an infeasible unconstrained result.

A provisional `ClipPropertyEdit` contains a clip pointer and destination `ClipPosition`. During numeric scrubbing, `EditComponent` forwards plans to `SongEditorView::setClipPropertyPreview()` for translucent bodies/outlines. No Tracktion mutation or undo transaction occurs until release; an unchanged final plan is a no-op. Selection changes can discard the edit, while unrelated refreshes preserve writable text.

Text/wheel apply immediately. Owner callbacks delegate Start to `EngineHelpers::moveSelectedClips()` and End/Duration to `resizeSelectedClips()`; preserve that integration rather than bypassing it through direct component fallback. The [overwrite command](../architecture/clip-overwrite-command.md) retains destination priority, atomic undo and playback-graph safeguards.

## Audio clip gain

`GAIN` is a separate field, not a `ClipPropertyEdit` time plan or an overwrite operation. It is enabled only for a nonempty, all-audio clip selection and disabled during canvas timing previews. Common values use one decimal place in dB; differing values show `—`. `ClipGainEdit::parse()` accepts finite signed decimal dB values (optional `dB` suffix) within the Engine range −100 to +24. A sign is part of an **absolute** value, unlike signed timing input.

Text assigns the same gain to all selected audio clips. Wheel and vertical scrub use a common 0.5 dB delta; the intersection of all selected clips' limits constrains the delta without changing their relative levels. Scrubbing uses four logical pixels per step. `ClipGainEdit` retains audio-clip references and original gain properties, checks membership in the edit and competing gain changes, and ignores MIDI members for canvas operations. Selection/lifetime invalidation cancels the preview.

Live gain preview deliberately writes `te::IDs::gain` without undo so the Engine and existing waveform renderer update immediately. Owners hold `ScopedSaveLock` to defer autosave. On release the helper restores the original properties, then commits through `AudioClipBase::setGainDB()` in one bounded transaction; no-op and canceled gestures create no gain undo entry. Escape restores original values, including an originally absent gain property. Temporary top-level key listeners make Escape reliable even if native drag/focus transitions move keyboard focus away from the originating component; they are removed on completion, cancellation and destruction. Main undo/redo, explicit save and project-workflow entry cancel live gain gestures before changing history or persisting the edit. Component teardown also cancels them.

`ClipGainHandle.h` provides the common rendering/hit-test geometry used by `Utilities.cpp` and `TrackLaneComponent`. Selected audio clips display a horizontal guide with a six-logical-pixel-diameter circle at the clip's horizontal centre. Only the circular distance test is interactive, never the line or its bounding-box corners. The geometry excludes the clip header and fade strip, requires at least 40 pixels of clip width and 24 pixels below the header, and keeps edge-resize zones free. With the default 20-pixel header this means a 44-pixel minimum total height: the default 50-pixel track shows the handle, while a minimized 30-pixel track does not. Pointer and Time Stretch can operate the circle; double-click resets selected audio gains to 0 dB. Mixed canvas selections leave MIDI clips unchanged. Hover/drag highlights the circle and displays gain; the existing waveform drawing already reads `getGain()`.

`EditComponent` refreshes the property display and arrangement waveform on gain changes, including undo/redo. The persisted value is the Engine's existing clip gain, not a new application setting, source-file edit or track-volume change. `AgentDebug` includes `gainDb` in audio-clip summaries for integration checks.

Sources: `App/include/ClipGainEdit.h`, `App/src/ClipGainEdit.cpp`, `App/include/ClipGainHandle.h`. `App/tests/ClipGainEditTests.cpp` covers parsing, shared limits, MIDI exclusion, preview cancellation/deletion/competing edits, XML/edit reload, atomic undo/redo and exact circular hit geometry. The [native gain procedure](../development/testing.md#audio-clip-gain) exercises the real controls; listening and native-platform checks remain separate.

## Live canvas values

`setInteractionPreview(std::optional<ClipTimingPreview>)` is a display-only path. `SongEditorView` and the Piano Roll clip overlay publish the grabbed clip's feasible range and actual selection count through `EditViewState::clipInteractionPreviewChanged`. `EditComponent` installs/disconnects that transient callback. The ghost shares `ClipGestureLimits::previewRange()` so inspector endpoints/duration match seconds-length movement through tempo changes.

A multi-selection canvas gesture uses `CLIPS (REF):` for the grabbed clip, distinct from the numeric edit's ordered-selection reference. Snapshots have no model pointers and do not invoke edit/commit callbacks. Idle common-value display returns afterward.

[Shared feedback](timeline-snapping.md#snap-feedback-and-live-values) owns subtle tint, model-refresh precedence, synchronous queued-text-edit completion and snapshot cleanup. These fields cannot start competing text/wheel/scrub input while a canvas snapshot is active; snap controls remain available. Existing glyphs receive theme/tint updates, not only future TextEditor text. Duration layout reserves space for long tick counts.

`setSnapFeedback()` updates compact status beneath SNAP without changing its combo. Arrangement gestures use the arrangement timeline. A Piano Roll overlay can display clip values here, but its snap status remains in the Piano Roll header.

## Validation and limitations

[Property-field input](property-field-input.md#validation-and-limitations) and [Testing](../development/testing.md) distinguish helper coverage from complete field/owner commit integration. The parser accepts zero tick magnitudes where the note parser does not; final clip lengths must still be positive. Raw clip-pointer plans require selection/lifetime checks. Full focus races, narrow layouts, automation/source combinations and model replacement still require scoped integration/runtime validation.

## Related references

- [Song Editor implementation](song-editor.md)
- [State and events](../architecture/state-and-events.md)
- [Shared numeric input](property-field-input.md)

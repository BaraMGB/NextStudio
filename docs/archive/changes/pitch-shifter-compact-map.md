# Compact Pitch Shifter editor and Pitch Map

- Type: historical record
- Audience: contributors
- Status: archived after current-contract extraction; not an active plan or fresh validation run

Current reference: [Pitch Map contract](../../components/built-in-effect-editors.md#compact-pitch-shifter-and-pitch-map). Original graph/knob/automation coverage distinctions remain intact; local shared/evidence paths are not durable downloads.

## Root cause

The built-in Tracktion Pitch Shifter exposes only one automatable parameter, `semitones up`. Without a dedicated branch in `PluginChainItemView`, it used `VstPluginComponent`: rack width factor 3, a 30 px last-changed parameter row, and another 30 px row for the same parameter inside a viewport. The remaining height was unused.

## Approved design

`PitchShiftPluginComponent` follows the dedicated editor pattern used by Delay, Chorus, and Phaser: a separate parameter-driven graph, a track-coloured `GUIHelpers::drawHeaderBox` header, theme backgrounds, and standard `AutomatableParameterComponent` controls.

The editor requests width factor 1, matching Volume/Pan and Arpeggiator. This reduces its rack-item width to one third of the previous allocation at the same rack height. It stacks a **PITCH MAP** above a single Semitones knob and value label. After the initial layout review, the maintainer requested whole-semitone dragging of the map's output point while retaining the continuous knob and automation range. No viewport or duplicate control is required. The shared rack height is intentionally unchanged.

## Geometry and display

`EffectEditorLayout::pitchShifter()` calculates bounded, vertically and horizontally centred rectangles. Content width is capped at 160 px, map height at 150 px, and parameter height at 135 px. It reserves the standard control's 20 px title and 15 px value heights, prioritises usable control space at small sizes, and clamps all rectangles to the available dimensions.

`PitchShiftDisplay::positionForSemitones()` maps the existing engine range linearly onto a vertical interval scale. Positive shifts move upwards, negative shifts downwards, and zero remains at the centre. Octave labels appear at -24, -12, 0, +12, and +24. The neutral ring and shifted point coincide at zero; fractional values are not rounded. A coloured connection and arrow indicate direction and magnitude.

The diagram represents configured transposition only: it does not detect notes or analyse the audio signal. Its `AutomatableParameter::Listener` observes both `parameterChanged` and `currentValueChanged`, so manual changes and effective automation/modifier updates repaint it without a separate animation timer. Listener registration is removed on destruction.

## Snapped graph input

Only a left-button drag beginning within 12 px of the output marker edits pitch. The marker highlights on hover and uses a vertical-drag cursor; its tooltip describes whole-semitone input. Drawing and hit testing share the same scale geometry. Mouse-down captures the original base value, pointer position, and scale height without changing the value. Subsequent vertical movement maps relatively onto the existing -24 to +24 range and rounds to the nearest integer, preserving the grab offset and clamping out-of-panel movement.

`PitchShiftDrag` owns one parameter gesture and one named undo action per completed graph drag. It applies live changes through `setParameter()` with synchronous notifications and `AutomationWriteGuard`, so the existing knob/value and graph stay updated. The custom action retains the plug-in and parameter and restores the exact starting value, including fractions. No movement, unchanged final values, and Escape cancellation do not create an undo entry. Mouse-up, focus loss, disable/hide, and editor destruction finish the gesture; Escape restores the starting value and closes it without committing.

The graph's explicit undo action must own both parameter and persisted state changes. Around each graph update, the helper temporarily rebinds only the engine's `semitonesValue` CachedValue to the same state/property without its UndoManager, then restores the original plug-in UndoManager. The automatable parameter remains attached to that CachedValue, and updates still go through `setParameter()`. This prevents intermediate engine property-only undo actions from leaving the parameter base and persisted property out of sync, including during graph undo/redo. There are no engine-module changes or permanent binding/range changes.

## Behavior preservation

The editor binds the existing `semitones up` parameter; its native range, fractional precision, value formatter, standard knob input/context menu, automation gestures, MIDI Learn, and existing knob undo path remain unchanged. Integer snapping is confined to graph input. DSP, time-stretcher mode/options, project state, and the engine modules are not modified. State restore delegates to the existing plug-in, and factory state uses `PitchShiftPlugin::create()`.

## Regression and visual validation

`EffectEditorLayoutTests` covers compact rack width, narrow/default/wide editor bounds, disjoint graph/control regions, centering, minimum readable sizes, growth limits, and zero/tiny dimensions. It also checks the octave endpoints, zero alignment, out-of-range display clamping, fractional shift precision, symmetric drag rounding, and reachability of all 49 whole-semitone values.

`PitchShiftDragTests` exercises actual Tracktion parameters and persisted state: live integer updates and range limits, synchronous notifications, one balanced gesture per drag, one undo/redo step restoring the exact previous fraction, no-op clicks/return-to-start, cancellation, and destruction. It verifies that direct native fractional edits still work and that the native range interval remains zero.

Focused runtime review used an isolated debug-shell session on a temporary X display. Screenshots verified the panel header, all five scale labels, the knob/value, and marker positions for 0, +12, -12, +0.5, and +24 semitones. A knob drag produced a fractional native value confirmed by a state dump. Comparison with the previous shared build confirmed the former duplicated rows and the rack-width reduction.

The initial continuous-knob review found that a toolbar undo attempt left the native parameter unchanged in both the previous and new builds; this existing knob path remains unchanged. The new graph input has its own validated undo action. Focused debug-shell review confirmed an upward graph drag to +10, one-step toolbar undo to 0 and redo to +10, a downward drag to -12, clamping at both +/-24 limits, and Escape cancellation restoring a fractional starting value. Dragging the knob still produced a fractional value (-11.232), confirmed through state dumps. The maintainer accepted the refreshed shared artifact and final graph interaction and explicitly approved commit, push, and marking #70 fixed. Actual automation playback was not part of the agent's headless validation; the existing continuous knob and native automation range are retained.

Validation commands: `BUILD_JOBS=12 ./build.sh rd`, `BUILD_JOBS=12 ./test.sh rd` (all 22 tests pass), and `./build_and_copy_shared.sh`. The shared artifact is `/home/ai/Gemeinsam/NextStudio`.

User documentation: [Pitch Shifter](../../plugins/pitch-shifter.md).

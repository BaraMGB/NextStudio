# Built-in effect editor contracts

- Type: reference
- Audience: contributors
- Scope: responsive built-in effect presentation and special graph/reset input

## Boundaries and source

Dedicated editors bind existing Tracktion/native parameters and reusable `AutomatableParameterComponent` controls. Rack composition and [bypass presentation](plugin-chain-view.md#whole-item-bypass-presentation) belong to PluginChainView; user controls belong to the respective [plugin references](../README.md#plugin-reference). Presentation changes do not authorize DSP, routing, native ranges or automation redesign.

| Area | Source / tests |
|---|---|
| Shared responsive rectangles and pitch scale | `App/include/EffectEditorLayout.h`, `App/tests/EffectEditorLayoutTests.cpp` |
| Compressor and Delay | `App/src/CompressorPluginComponent.cpp`, `App/src/DelayPluginComponent.cpp` |
| Compact pitch editor/display | `App/src/PitchShiftPluginComponent.cpp`, `App/include/PitchShiftDisplay.h` |
| Graph pitch gesture/undo | `App/include/PitchShiftDrag.h`, `App/src/PitchShiftDrag.cpp`, `App/tests/PitchShiftDragTests.cpp` |
| EQ graph/reset | `App/src/EqPluginComponent.cpp`, `App/include/EqBandReset.h`, `App/tests/EqBandResetTests.cpp` |
| Reverb chamber | `App/src/ReverbPluginComponent.cpp` |

## Dense control layouts

Compressor and Next Delay request rack width factor 3 and put their graphs beside controls so title/value labels do not consume the knob's remaining height. Compressor has two three-knob rows and a full-width sidechain source/trigger footer. Next Delay has a weighted Mode/Sync/Division choice row above two knob rows; Mode receives more width. Its graph header displays only `DELAY SPACE`, leaving values in the controls. Legacy Tracktion Delay keeps its compact editor.

`EffectEditorLayout` owns the rectangle constants and algorithms: bounded graph widths, reserved usable control space, compact weighted choice row, equal remaining knob rows, safe insets/remainder distribution and tiny-size clamping. Do not copy the helper's pixel constants into each editor or a second reference table. Slider title/value allocation must be considered when assessing actual rotary radius, not just the outer control cell.

These layouts retain parameter binding, presets, automation, MIDI Learn, undo and sidechain behavior. Regression tests cover narrow/default/wide/tiny bounds, non-overlap, minimum cell sizes and choice widths; visual readability still needs native review.

## Compact Pitch Shifter and Pitch Map

The Pitch Shifter requests width factor 1 with one Semitones knob and a bounded, centered `PITCH MAP`; no duplicate generic parameter row/viewport is needed. Shared rack height stays unchanged. `EffectEditorLayout::pitchShifter()` caps growth while prioritizing the standard control's usable area.

`PitchShiftDisplay` maps the existing `-24..+24` range linearly onto a vertical scale, positive upward, with octave labels and coincident neutral/output points at zero. Display preserves fractions. This is configured transposition, not measured notes or audio analysis. A parameter listener repaints for both base `parameterChanged` and effective `currentValueChanged`; it disconnects on destruction and needs no independent animation timer.

Only left-button dragging begun near the output marker edits the graph. Source hit tolerance and shared scale geometry govern drawing/hits. Mouse-down captures base value, pointer and scale without changing pitch; relative vertical motion rounds to whole semitones and clamps outside the panel, preserving grab offset. The standard knob/native automation remain continuous; only this graph input is quantized.

### Gesture, persistence and undo ownership

`PitchShiftDrag` owns one balanced parameter gesture and one explicit undo action per effective graph drag. Live updates use `setParameter()`, synchronous notification and `AutomationWriteGuard`. The action retains plugin/parameter references and restores the exact initial fraction as well as persisted state.

For graph updates, the helper temporarily binds only the engine's `semitonesValue` CachedValue to the same property without its UndoManager, then restores the original manager. The automatable parameter remains attached and updates still use its API. This prevents intermediate property-only undo records from diverging from parameter base state; it is not a permanent range/binding change or a patched engine module.

Mouse-up, focus loss, hide/disable and editor destruction finish a gesture. Escape restores the original base value and closes without commit. No movement or an unchanged final value creates no undo entry. The native continuous-knob undo path is not replaced by the graph's custom action.

`PitchShiftDragTests` covers real parameters/state, bounds, synchronous notifications, balanced gestures, exact fractional undo/redo, no-ops, cancellation/destruction and the unchanged continuous native interval. Layout/display tests cover all 49 whole-semitone inputs and fractional display. Helper tests do not certify live automation playback; the historical graph review also did not establish successful continuous-knob toolbar undo. Preserve those distinctions.

## EQ whole-band reset

`EqResponseGraphComponent` supports double-clicking a band handle or its right-click **reset values** action. `EqBandReset` resets frequency, gain and Q together from each attached parameter's `getDefaultValue()`, not duplicated UI constants. Synchronous notifications update graph/audio filter values immediately; all changes share one named undo transaction.

`EqBandResetTests` instantiates the actual 4 Band Equalizer and verifies every band, parameter-owned defaults, notifications and one-step undo/redo. The [EQ user reference](../plugins/eq.md) describes controls; a historical defaults table is a snapshot, not a second authority when engine defaults change.

## Reverb header

The chamber header displays only `REVERB CHAMBER`. Wet/Dry/Freeze values already have controls; independently painting both strings into one narrow rectangle caused overlap. Removing duplicate status avoids an unnecessary abbreviation/width-policy subsystem. This is paint-only: controls, DSP, notifications, presets and automation stay unchanged.

A removed paint operation introduces no new isolated layout logic warranting an artificial helper test. Build plus focused minimum-width visual validation remains appropriate; do not present the general CTest suite as direct certification of the complete Reverb panel.

## Validation

Use [Testing](../development/testing.md) for commands and coverage boundaries. Current technical contracts above stand alone; [historical records](../archive/README.md) retain original approvals, screenshots and environment-specific results. Large-rack repaint/scroll responsiveness is not a formal frame-time benchmark, and presentation acceptance does not extend audio/automation/platform coverage.

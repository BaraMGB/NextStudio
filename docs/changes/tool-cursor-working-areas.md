# Timeline tool cursor working areas (#90)

## Corrected contract and implementation

MIDI Draw, Knife and Eraser share one working area: MIDI clip time ranges. Their tool cursor remains visible between notes inside a clip; outside clips and in gaps it is the normal pointer. Lasso/Range remain usable in empty space; Pointer retains contextual note-body/edge feedback. Song Knife remains visible throughout clip-capable track lanes, including clip gaps, but not on Master/non-clip lanes. Active drags retain their action cursor.

The first implementation incorrectly exempted MIDI Knife/Eraser from the clip boundary and added an unnecessary policy/hover-event layer. Following maintainer feedback and approval of the reduction plan, that layer, synthetic events/fake moves and its misleading helper tests were removed. `MidiViewport::updateToolCursor()` now directly applies the clip check in the existing hover path and at entry, tool change, release/Draw cancellation and existing non-drag editor refresh. Song Editor refreshes the actual clip/edge/fade hit before reusing its lane cursor method. The Range overlay shares its position-based cursor update between hover and tool changes; the new hit target supplies the cursor to JUCE's still-stationary previous owner, including Pointer's selected-range body/edges and inactive overlays. The external Master lane is refreshed directly. No new gesture architecture, edit actions, snapping, persistence or undo commands are introduced.

## Validation

Native scripted checks use an isolated Xvfb/debug-shell sandbox and inspect real XFixes cursor images, not application-only screenshots. Evidence: `/tmp/nextstudio-cursor-correction/` (`cursors.py`, `actions.py`, `transitions.py`, `fractional.py`, paired cursor PNG/JSON and model dumps).

- The preceding shared binary fails 12 clip-boundary/re-entry checks for MIDI Knife/Eraser (`before-results.json`); the corrected binary has zero working-area failures (`after-results.json`).
- Native 100/125% checks cover all MIDI tools inside/outside clips and in a two-clip gap, note-free clip positions, re-entry, Song Knife gaps and the normal Master pointer.
- Real Draw insertion, MIDI Knife splitting, Eraser deletion and undo/redo are checked through model dumps. Empty-space clicks/selection, provisional drags and cancellation preserve musical data; Pointer edge/body and active Draw/Pointer/Eraser drag cursors are inspected.
- Stationary MIDI tool changes use native toolbar focus/Return activation. Song Range-to-Time-Stretch/Knife transitions include the previous Range overlay and Master. Stationary pan updates all three clip-bound modes. Final replay passes (`final-runtime.log`).

### Review follow-up: stationary Song Editor transitions

The review found two missing cases: Range-to-Pointer retained the I-beam, and Range-to-Time-Stretch used stale lane hover state over clips. Both are corrected without synthetic mouse events or gesture changes. A native 100% before/after regression checks 16 transitions: Range to Pointer/Time Stretch over gaps and clip bodies/both edges, Range/Knife to Pointer over selected-range bodies/both edges, and Master transitions. The preceding shared build fails 13 checks; the corrected build passes all 16 with unchanged musical model dumps. Evidence and the replay script are under `/tmp/nextstudio-code-review/` (`transition-regression.py`, `before-transition-results.json`, `fixed-transition-results.json`). The working-area and action/pan scripts also pass again (`working-areas.log`, `actions.log`). The final build succeeds and all 29 CTest targets pass again (6.21 s). This follow-up does not extend the earlier 125% coverage to these new transition cases.

This is direct GUI behavior, so no artificial boolean-policy helper is extracted just to unit-test it. The isolated-X debug-shell editing smoke test passes. Existing 29 CTest targets pass (6.23 s) after `BUILD_JOBS=12 ./build.sh rd` and `BUILD_JOBS=12 ./test.sh rd`. Native 150/200%, other platforms, monitor transitions and exhaustive automation/overlay combinations are not certified.

## Delivery

The refreshed executable was delivered through `BUILD_JOBS=12 ./build_and_copy_shared.sh` to `/home/ai/Gemeinsam/NextStudio`, mode 0755; the review-fixed built/shared SHA-256 matches (`87399ac5af0c8ea98247f40a84f59a82eee0243f14424a681476eb1ddadc4a5e`). The maintainer accepted the fix with “committen und pushen. die Issue ist gefixed”, explicitly approving commit/push and closure of #90 under the clarified working-area contract. This acceptance does not extend the documented platform/scale/automation validation coverage.

# Snap feedback and live header values — implementation and validation

Status: implemented and approved by the maintainer for commit/push:
“wir committen das feature und pushen es”.
Baseline: `54bb3a0`.

## Delivered behavior

- Actual magnetic plateaus show a fractional-X guide and outlined diamond on the manipulated edge, including the ruler. Knife accents its existing split line instead of adding a duplicate guide.
- Compact status is stacked under SNAP in fixed space: held/free/Shift/off/limit/invalid. The selected grid combo, attraction profiles (18 physical pixels; Song 40%, MIDI 30%), scalar mapping and discrete/numeric/keyboard policy remain unchanged.
- Feasibility limits and invalid destinations clear the held target. Shift/Off do not advertise a hold even exactly on a target. Feedback originates in the relevant view, including Piano Roll clip-overlay gestures.
- Clip Start/End/Duration follow move/copy/resize/stretch timing. Note fields follow Pointer timing, including Pitch, and Draw's provisional note. Velocity is unchanged by Pointer and uses the creation default for Draw.
- Only a subtle theme-derived text tint distinguishes live values. There is no Preview label, badge or underline. Existing glyphs receive the tint via `applyColourToAllText()`; commit/cancel restores normal color and model/selection values. Tick duration fields reserve space for six digits.
- Multi-selection uses the grabbed reference with `CLIPS (REF):` / `NOTES (REF):` and the actual selection count. Ordinary common-value/em-dash display returns afterward. Draw does not add a dummy note/count or label itself as an existing multi-selection reference.
- The snapshots contain values, not model pointers. The clip callback is edit-local transient UI wiring; the note callback belongs to its timeline. No preview-only model mutation, selection broadcast, serialized preference or additional undo transaction is introduced. Automation retains its existing live model-edit lifecycle.
- Active snapshots override model refreshes. Existing text edits finish through normal focus loss before gesture origin is read; conflicting text/wheel/scrub edits are blocked while live values are active without visually disabling their fields.
- Escape/tool changes/source removal clear preview and feedback. Note commit scopes distinguish intentional removal/recreation from external deletion. Callbacks disconnect at teardown and context replay uses safe component references.

## Context and geometry refinements

Detailed kernel/resolver results retain the exact scalar position. Gesture feedback is finalized after constraints, and ghosts/live fields share existing feasible timing and `ClipGestureLimits::previewRange()` rather than applying a second snap.

Tempo/context replay must subtract the primary edge's actual current source time, not convert a global beat saved before a tempo change back to seconds using the new tempo. `MidiNoteGesture::edgeTime()` and `ClipGestureLimits::edgeTime()` provide this basis for Pointer, arrangement and cached overlay timing. The regression deliberately demonstrates that the saved-beat path yields a different note edge, while the corrected path remains held at the same actual global target without modifying the source note. Draw cancels if a changed clip base would make its fixed start internally negative.

`MouseGestureInput` distinguishes modifier-only replay (cached floating coordinates) from context replay (actual pointer and original physical mouse-down position through the new component transform). It reads coordinates; it does not warp the pointer. Pointer re-anchors Pitch without changing it during context replay, and automation preserves current values while refreshing timing. MIDI viewport geometry and vertical view changes request a coalesced timeline refresh even if its horizontal snap context is unchanged. A JUCE ComponentMovementWatcher also observes ancestor translation, peer and visibility changes; an ephemeral geometry revision ensures raw anchors rebase in the new local coordinate system. Automation's marker uses the actual moved point value/time, including clamping.

## Automated validation

Commands from the repository root:

```sh
BUILD_JOBS=12 ./build.sh rd
BUILD_JOBS=12 ./test.sh rd
BUILD_JOBS=12 ./build_and_copy_shared.sh
```

All **29/29 CTest targets pass**. Existing helper/model/numeric/property/creation tests remain registered; new assertions are integrated into the two snapping targets.

Additional coverage:

- Detailed/scalar kernel and resolver equality, plateau boundaries and escape, both attraction profiles, small/large intervals and coordinates, and 100/125/150/200% raster scales.
- Held/free/bypass/off/limit/invalid reporting, infeasible edges, gesture reset, stationary Draw Shift and initial end reporting.
- Clip preview ranges for move/both resize edges/stretch and pointer-free reference/count snapshots without source changes.
- Tempo-change replay through the actual production edge-time, constraint and timing helpers, with an explicit old saved-beat failure guard.
- Theme-derived font tint, including equal normal/accent fallback; fractional guide/diamond raster geometry and lane clipping at four scales.
- Modifier replay preserves float/down coordinates, buttons and stylus metadata. Context replay uses a transformed component, current input-source position and transformed original physical down point, retaining buttons/pressure/tilt/time; cleared input cannot replay.

These are helper/model/raster checks, not an automated end-to-end certification of all TextEditor focus routes, async callbacks or platform input behavior.

## Native GUI evidence

Isolated Linux Xvfb `:98`, 1600×1000, private debug-shell settings/content/project sandbox; artifacts copied to `/tmp/nextstudio-feedback-validation/` before session cleanup.

At 100%, screenshots and state dumps verified:

- Clip move: held Start `1.3.000`, End `3.3.000`, duration `7680 ticks`, while model Start remained `0` seconds; release committed `1` second. Stationary Shift immediately removed the held cue and changed live values; restoration reinstated it. Cancel/commit restored normal text color.
- Pointer move: global Start `2.2.000`, End `2.4.000`, Duration `1/2`, Pitch C4, while internal model Start remained `2` beats; release committed `3` internal beats. Stationary Shift/restoration and exact header/ghost timing were checked.
- Draw showed its new timing/Pitch/Velocity without a dummy model note; Escape retained the original notes and restored the selection display.
- Right resize showed End `2.3.240` and `1200 ticks`; release committed `1.25` beats. This also exposed and corrected the old short tick-display width.
- Multi-note reference display and group copy: two selected notes legitimately created two copies, not one. An initial one-copy expectation was corrected by inspecting both source/copy state dumps.
- MIDI/arrangement Knife, loop preview and Piano Roll clip-overlay cancellation; live clip fields route to the arrangement bar while overlay snap feedback remains in Piano Roll.
- Source deletion during Pointer preview cancelled cleanly; mouse-up did not insert or further change notes.
- Light and dark themes, readable subtle tint and restoration. Pixel inspection confirmed actual existing-text RGB changes, not merely TextEditor color defaults.
- Held Pointer preview survived Ctrl-wheel zoom without provisional note changes; one-pixel continuation retained the held edge and C4, then release committed the displayed timing.
- Resizing the native window from 1600×1000 to 1600×1050 translated the fixed-height Piano Roll panel by 50 pixels. An intermediate check exposed an unwanted C4→D4 change on one-pixel horizontal continuation: observing only the editor itself missed an ancestor translation. The final movement watcher check retains global Start `2.2.000`, Pitch C4 and the unchanged provisional model across resize/continuation, then commits internal Start `3` and C4. Vertical wheel scrolling likewise preserves the provisional model and Pitch; Escape restores the original notes.

Final-build evidence is prefixed `watcher-*`; core later-build evidence is prefixed `final-*` and `delivery-*`;  earlier broader gesture/theme evidence is unprefixed. Native 125% application-scale checks on an earlier implementation iteration verified held note rendering and source deletion (five notes to four, with no additional mouse-up change). That is application transform scaling, not another OS/platform certification.

Logs and evidence: `build*.log`, `final-build.log`, `tempo-replay-build.log`, `tempo-replay-test.log`, `context-build.log`, `geometry-build.log`, `watcher-build.log`, `watcher-all-tests.log` (29/29, 6.17 s), final delivery logs,  `commands.jsonl`, screenshots and paired JSON dumps.

## Maintainer follow-up: ruler feedback above clip headers

The opaque bottom-third clip headers painted by the later `TimelineOverlayComponent`
sibling covered the ruler signal painted inside `TimeLineComponent::paint()`.
The maintainer approved moving it to the final foreground pass. The Piano Roll now
calls the shared translated/clipped ruler renderer from
`PianoRollEditor::paintOverChildren()` after children and separator borders; the
arrangement retains its original paint pass. Overlay body feedback clips below the
ruler, preventing duplicate translucent strokes. Feedback changes/clear explicitly
repaint the parent region. No hit-test/z-order, profile, timing or font changes.

The new JUCE component-layer test covers four scales × three fractional phases ×
four held/free/bypass/limit states (48 cases), checking exact pixels against a
foreground reference with identical child clipping. An explicit old-order render
differs only in held cases. Initial regression output was
`clip header covered ruler feedback or guide was drawn twice`; the corrected tests
pass. These isolated widgets use the production layer selector/ruler helper, not
the complete Piano Roll class; native checks cover its actual integration.

At native 100%, `ruler-before-held.png` versus `ruler-after-held.png` confirms the
ruler marker was hidden and now survives the clip header. The marker's 9×8 pixel
patch around X=774/Y=726 contains 0 accent pixels before, 24 after, 0 under stationary
Shift and 24 after restoration. Draw's initial held End (X=570), clip-overlay Start
(X=502) and exact Knife target (X=774) each show the same 24-pixel marker above the
header. Pointer/Draw/overlay previews leave model timing/count unchanged; Escape
restores the original model. Artifacts are `ruler-*` in the existing evidence folder.

Final build: `ruler-rebuilt.log`; focused tests: `ruler-final-test.log`;
**29/29 CTest targets pass**, 6.20 s (`ruler-all-tests.log`). Native checks for this
layer correction are 100% only; the four-scale results are automated raster coverage.

## Code-review corrections

Three findings were corrected without changing magnetic mapping:

- Both bars use `TimelineInteractionPreview::finishTextEdit()` to run their existing
  focus-loss commit/reject callback synchronously for writable fields. JUCE moves
  focus before canvas `mouseDown()` and queues the notification; testing only
  `hasKeyboardFocus()` missed these pending edits. Completion makes the field
  read-only, so the delayed callback cannot commit again or use live preview text.
- The ruler marks every pointer interaction active from mouse-down. Escape clears
  that state as well as loop flags, feedback and cursor; `mouseDrag()` ignores
  further events until a new mouse-down. This lifecycle flag is separate from the
  existing pan/zoom anchor-paint flag, preserving loop rendering. A cancelled loop
  no longer falls through into pan/zoom while the button remains held.
- `ClipGestureLimits::validMoveDestinations()` checks the whole selected group,
  including bounds, clip-track destinations and the caller's type policy. Move/
  copy commit and arrangement feedback share it. A valid primary target no longer
  hides an invalid secondary lane. Resize/stretch markers stay in their source
  lane, matching commits that do not move vertically.

Automated `TimelineSnapping` regressions exercise a real JUCE TextEditor's queued
focus-loss notification, synchronous completion, valid/rejected input and delayed
callback idempotence after preview text replaces the field. They use a test
commit/reject callback, not the complete property bars. Destination regressions
use real clips/tracks with a supplied type-policy callback: valid primary/invalid
secondary, valid group, out-of-bounds/extreme offsets, empty/null selection and
invalid feedback clearing its held target. Full bar/ruler integration is covered
by the following native checks rather than claimed as CTest coverage.

Native Linux Xvfb `:98`, 1600×1000, isolated debug-shell session:

- A pending clip End `5.1.000` completed before a Piano Roll overlay drag; the model
  length was already 8 seconds during its display-only preview.
- A pending note Duration `2400 ticks` completed before Draw mouse-down. The model
  held exactly one note of 2.5 beats during provisional Draw; Escape discarded the
  provisional creation, not the completed property edit.
- After cancelling an existing loop drag, continued horizontal **and vertical**
  mouse motion with the button still down left timeline start/zoom unchanged.
  The original IN/OUT remained `2.1.000`/`5.1.000`.
- Moving two MIDI clips down one lane left the primary on MIDI but the secondary
  on audio. The header showed `invalid` without a held guide; release preserved
  both original clips/ranges/tracks.

Evidence: `/tmp/nextstudio-review-fixes-validation/` (`loop-before.json`,
`loop-cancel-continued.json`, `clip-pending-edit.json`,
`note-pending-edit-draw.json/.png`, `group-invalid.png`, `group-rejected.json`).
The final artifact repeated the loop cancellation/continuation check; evidence is
`final-loop-before.json`, `final-loop-cancel-continued.json/.png`. The final-only
change separates pointer lifecycle from the existing pan/zoom anchor-paint flag;
header/group behavior is unchanged from the preceding native checks.
Build/test/delivery logs are `/tmp/nextstudio-review-fixes-build.log`,
`nextstudio-review-fixes-tests.log`, `nextstudio-review-fixes-delivery.log` and
`nextstudio-review-fixes-final-tests.log`. Final validation: **29/29 CTest targets
pass**, 6.22 s; `git diff --check` passes. These native checks are at 100% only.

## Delivery and coverage limits

Test executable: `/home/ai/Gemeinsam/NextStudio`, mode `0755`.
Built and shared SHA-256 after the code-review corrections:

```text
0a8c374ad10b516383ca8c28736c485f3975007c8db6ccf3f330bad9c92ac5a1
```

Final copy/build log: `/tmp/nextstudio-review-fixes-delivery.log`.
File hashes match and mode is `0755`. The earlier ruler-layer artifact was
`c4f7f591076316d961565a71dd73f94e6ccfc5a607fe80ba7ca9ea64d7d59326`
(`ruler-sha256.txt`, `ruler-shared-build.log`).
The superseded first delivery was `0f92677512762c94b594e5fcf55cda53227ed92f575522c09361dbd29360394b`
(`delivery-final-sha256.txt`).

The maintainer has authorized committing and pushing this feature, including the
code-review corrections. #77's already-completed closure is unchanged. This
approval does not extend the automated/native coverage beyond the evidence above.

Not claimed: exhaustive native automation/range/audio-drop/stretch/proxy/fade or multi-track/loop/take combinations; every failed-commit/model-refresh/project-replacement/focus race; native 150/200% or OS monitor transitions; other platforms, stylus or live audio hardware. The final context/geometry refinements have focused regression/native checks, not a complete replay of every earlier 125% scenario. ALSA device-access warnings in the sandbox do not certify audio operation.

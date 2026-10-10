# Shared soft timeline snapping — implementation and validation

- Type: historical record
- Audience: contributors
- Status: implemented and accepted; #77 closed in `54bb3a0`
- Current reference: [timeline snapping](../../components/timeline-snapping.md)

Results below identify different implementation iterations. Intermediate "pending" statements are historical; final acceptance does not expand their coverage. Local evidence/delivery paths are not durable downloads. Use [Testing](../../development/testing.md) for current procedures.

## Scope

Approved implementation of [the plan](soft-timeline-snapping-plan.md) for [#77](https://github.com/BaraMGB/NextStudio/issues/77), against source baseline `44f805e`.

The implementation separates hard musical commands from magnetic mouse positioning. Draw, Pointer note edits, both Knife tools, arrangement clip gestures, Piano Roll clip overlays, ranges, loops, automation timing, and browser audio drop use the shared mouse policy. Keyboard nudging, quantization, numeric edits, and playhead clicks keep their discrete policy. Note insertion defaults use the approved upward end alignment.

Draw remains a **creation tool with overlap handling**; this does not introduce direct Pencil resizing of an existing hit note. Pointer resizing is migrated. Insert length is a starting default, not a minimum; the note duration floor is one Tracktion tick (`1/960` beat).

See [Timeline snapping](../../components/timeline-snapping.md) for current APIs and invariants. The initial measurements below describe the original 6px/20% build; the stronger 12px/30% follow-up and current delivery are recorded separately below.

## Automated validation

Completed from the repository root:

```bash
BUILD_JOBS=12 ./build.sh rd
BUILD_JOBS=12 ./test.sh rd
BUILD_JOBS=12 ./build_and_copy_shared.sh
```

All **29 CTest targets passed**, including:

- `TimelineSoftSnapTests`: dense monotonic/continuous sweeps, exact detents, narrow and unequal intervals, long coordinates, inverse anchors, bounded displacement/gain, invalid inputs, and non-idempotence.
- `TimelineSnappingTests`: production fixed/adaptive resolver; tempo ramps, meter/triplet changes and irregular meter boundaries; 100/125/150/200% raster scales; no off-grid grab jump; direction reversal, event-count independence, Shift and Off origin retention, Off-to-different-grid re-anchoring, view changes, and modifier-event coordinate/pressure preservation.
- Production Draw state and real MIDI creation/overlap tests: coarse-snap defaults, sub-insert shortening, tick floor, provisional-state cancellation, offset clips, actual Last Inserted duration, retained split-piece properties, one-step undo/redo, fractional XML persistence, endpoint-based timing, and common multi-note resize limits.
- Shared clip feasibility (including unsupported MIDI stretching) and automation multi-point neighbour/order constraints; valid/invalid MIDI split positions.
- Updated `PianoRollNoteLengthTests`: finite mode fallbacks and tick-only duration bounds rather than the former insert-length minimum.

Two adaptive engine defects discovered by regression sweeps are handled locally in the mouse adapter: stale meter information in bar rounding and non-bracketing extrapolation after a mid-bar meter change. The latter uses the engine's exact straight/triplet fractions from the signature boundary. Existing engine/hard command APIs and third-party sources are unchanged.

## Isolated runtime validation

Used the maintained debug-shell client, isolated temporary content/settings/projects, private Xvfb displays, actual `xdotool` mouse/key input, state dumps, and screenshots. Sessions were run serially because JUCE enforces a single application instance. No maintainer project/settings or external application was manipulated.

### 100% GUI scale

Verified on the original 6px/20% build:

- Snap **1/4**, insert **1/16**: click creation starts at beat 2 and immediately initializes a one-beat duration.
- Relative leftward Draw produces **0.153225806451613 beats**, below the selected 0.25-beat insert default. Preview does not create a model note.
- Shift changes Draw and Pointer previews **without pointer movement**; returning from Shift restores the magnetic position.
- One toolbar undo removes the creation; redo restores the fractional duration.
- Snap Off with Last Inserted preserves that actual fractional default.
- Dragging past the fixed start reaches the one-tick floor; Escape cancels without insertion.
- Pointer movement to **2.193548387096774 beats**, Ctrl-copy, and sub-insert edge resizing preserve expected durations/properties.
- MIDI Knife cuts between grid points at **2.459677419354839 beats**; both fragments retain the original total duration.
- Arrangement clip movement reaches **1/12 second** between targets; clip resize and Piano Roll clip-overlay resize use resolved previews.
- Drawing after a real clip-offset change uses the correct internal/global conversion.
- Loop and range creation/movement show between-grid endpoints without a second hard snap on release. The range operation produces three real clip fragments; the saved moved range fragment starts at **1.333333333333333 seconds**.
- A real Save As and project reload preserve all three clip positions and all 21 sequence-note records (including repeated clip-fragment source sequences), pitches, velocities, and fractional lengths. Maximum observed floating-point serialization difference: **9.99e-16**.

### 125% GUI scale

Verified on the original 6px/20% build with published `rasterScale = 1.25`, 1600×1000 physical X display, and application scale 1.25. Xvfb DPI alone does not enable JUCE scaling in this environment; this is an application-scale check, not a native OS fractional-DPI certification.

- After pulling out and returning to a displacement of five **physical** pixels, the detent preview is pixel-identical to the initialized preview.
- Continued leftward movement commits **0.153846202752529 beats**, matching the physical-pixel kernel prediction within floating-point event precision and below the insert default.
- Shift changes the stationary-pointer preview; releasing Shift restores a pixel-identical preview without cumulative offsets.
- The physical pointer remains at the requested **494,728** position; no cursor warp is introduced.
- Preserved both logical component snapshots and a native 1600×1000 physical-screen capture.

## Evidence and limitations

Local evidence directory: `/tmp/nextstudio-soft-snap-validation/`.

Key artifacts: `final-tests.log`, `runtime-100-final.log`, `runtime-100-checks.json`, `runtime-125-checks.json`, `final-100-*.png/json`, `native-125-*.png/json`, `Soft Snap Validation.tracktionedit`, and `shared-sha256.txt`. Development-session slow-drag recordings are also retained; the original screenshots/state checks identify that implementation's runtime verification. The strengthened profile has separate `stronger-*` evidence.

This is focused runtime coverage, not exhaustive certification of every migrated caller. Automation time dragging, browser audio drop, wave-audio stretching, every multi-track/loop/take combination, and all numeric/keyboard workflows still need broader maintainer testing. Native 150/200% UI, other platforms, stylus hardware, and live audio-device behavior were not tested; the automated resolver/geometry tests cover those two additional scales. ALSA device-access warnings in the isolated environment are not presented as audio validation.

## Follow-up: stronger attraction after maintainer testing

The maintainer found the original snap attraction too weak. Increased `radiusPixels` from 6 to **12 physical pixels** and `intervalFraction` from 0.2 to **0.3**, retaining at least 40% free travel and a maximum free-segment gain of 2.5. Both changes matter: increasing only the pixel radius would leave tightly spaced targets restricted by the old 20% cap.

Added explicit regressions for wide-grid attraction at ten pixels, dense-grid attraction beyond the old cap, and freely reachable intermediate positions. Rebuilt and reran all **29 tests successfully**, including physical-scale sweeps at 100/125/150/200%.

Focused native 100% checks confirm pixel-identical initialized/detent previews after ten physical pixels on a wide grid and nine physical pixels on a 34-pixel interval. Continued movement still shortens below the insert default (committed duration **0.116071428571429 beats**), Shift bypass changes the stationary preview, and releasing Shift restores it pixel-identically. The new profile was not separately rerun at native 125%; automated scaling coverage passed and the original native scaling checks remain recorded above.

Evidence: `stronger-build.log`, `stronger-tests.log`, `stronger-runtime-checks.json`, `stronger-*.png/json`, and `stronger-sha256.txt` in the same local evidence directory. Recreated the shared artifact through `BUILD_JOBS=12 ./build_and_copy_shared.sh` and verified checksum equality. Maintainer input-feel revalidation remains pending.

## Follow-up: editor-specific Song Editor strength

The maintainer accepted MIDI Editor feel but still found the Song Editor too weak. Added an explicit runtime attraction profile to the shared kernel/resolver: **Song Editor 24 physical pixels / 40%**, **MIDI unchanged at 12 pixels / 30%**. A larger percentage cap is important for the arrangement's small visible grid intervals; increasing only the absolute radius would not strengthen those cells. Song Editor still leaves at least 20% free travel (maximum gain 5).

`TimeLineComponent` selects the profile using its existing Piano Roll snap-settings ownership flag, independently of timeline ID. Forward mapping and inverse anchors use the same profile. Invalid profiles fail safely; profile changes re-anchor without replaying displacement. No new persisted setting or command/quantization behavior is introduced.

Rebuilt and passed all **29 tests**, including both profiles' dense continuity/monotonicity/inverse sweeps, unchanged default MIDI curve, physical scales 100/125/150/200%, fixed/adaptive tempo/meter boundaries, raw bypass, and profile-context changes.

Native 100% checks on the delivered build verify an adaptive arrangement clip holds at its exact original start after **15 physical pixels**, escapes at **18 pixels** to an off-grid **0.5-second** start, and one undo restores zero. MIDI dragging by 20 pixels still produces **0.928571428571429 beats** (it would remain in the detent if the Song profile leaked into MIDI); the earlier shortening gesture still produces **0.116071428571428 beats**. These are the accepted MIDI profile's unchanged results. Native higher-scale UI was not repeated for this follow-up; automated scale coverage passed.

Evidence: `song-profile-build.log`, `song-profile-tests.log`, `song-profile-runtime-checks.json`, `song-profile-*.png/json`, and `song-profile-sha256.txt`. Shared artifact rebuilt with `BUILD_JOBS=12 ./build_and_copy_shared.sh`; checksum equality verified. Song Editor input-feel revalidation remains pending.

## Follow-up: both editor radii at 18 pixels

Applied the maintainer's explicit request **“stell beides auf 18 Pixel”**: both editor profiles now use **18 physical pixels per side**. Only the radii change; existing interval caps remain **40% for Song Editor / 30% for MIDI**. No other gesture, bypass, quantization, or persistence policy changes.

Explicit kernel regressions verify both profiles hold at 18 pixels and escape beyond the boundary, including inversion, dense-grid caps, and default MIDI profile selection even though both radii now match. All **29 tests passed** after the update, including the resolver's four-scale and adaptive-boundary coverage. Native 100% MIDI checks show a pixel-identical initialized preview at 17 pixels and an escaped preview at 20 pixels, committing **1.02 beats**. The Song Editor's new radius is covered by the profile/kernel/resolver regressions; its native wide-grid radius was not separately re-measured in this follow-up.

Evidence: `radius18-build.log`, `radius18-tests.log`, `radius18-runtime-checks.json`, `radius18-*.png/json`, and `radius18-sha256.txt`. Shared artifact recreated with `BUILD_JOBS=12 ./build_and_copy_shared.sh` and verified against the built executable.

## Follow-up: Knife preview pixel alignment

The maintainer reported Knife preview offsets of up to two pixels. MIDI Knife stored the timeline's float X as an integer; truncation lost the grid's subpixel phase and magnified the visible error under scaling. Arrangement Knife also dropped small hover events through the generic throttler, potentially retaining a position just outside a detent. Its 1.5 px stroke differed from the grid's 1 px stroke.

MIDI Knife now retains float X through drawing. Arrangement Knife uses the grid's 1 px stroke and resolves every hover event. Musical snap targets, the 18 px attraction profiles, Shift bypass and actual split calculation remain unchanged.

A new regression uses the production MIDI Knife accessor's coordinate type and checks subpixel coordinates and identical grid/preview raster coverage at 100/125/150/200%. It failed before the fix and passes afterward; all **29 CTest targets passed**. This regression does not instantiate the full GUI tool or automate arrangement event routing.

Isolated native runtime checks reproduced the MIDI error at 125% application scale: logical grid coverage was at columns **498/499**, while the old preview covered **497/498**. Afterward both cover **498/499**; a physical desktop capture confirms both cover **622/623/624** with their centre at column **623**. Stationary Shift release restores a pixel-identical preview, and the actual cut yields beat-4 fragments of 4 and 8 beats. Arrangement checks at 100/125% confirm a detent-aligned line, including a final two-pixel move into the detent. Native higher-scale/platform coverage remains outstanding.

Evidence: `/tmp/nextstudio-knife-validation/` (`regression-before.log`, `build.log`, `tests.log`, `runtime-checks.json`, logical/native screenshots and committed state dump). Shared artifact regenerated with the required build/copy script and checksum equality verified.

## Follow-up: review fixes for loop paint, creation anchors and tempo geometry

Fixed three review findings. Existing loop move/resize now paints `m_newLoopRange`
while the gesture is active, just as release commits it. Removed the unused
legacy delta/rendering gate. Earlier loop model/commit checks did not certify
live preview freshness; this review exposed that gap.

Added `startAtOrBefore()` for initial mouse creation anchors. Draw, double-click
note creation, clip creation, ranges and loops use the corrected resolver targets;
keyboard/quantization/numeric/playhead command APIs remain unchanged. The engine
regression now creates the expected beat-24-to-31 Draw interval from raw beat 27,
instead of the erroneous 31-to-38 interval.

Range and audio-drop previews use production `TimeUtils::timeRangeToX()` to project
both actual time endpoints. Moved range content uses the destination beat scale,
provisional sliced clip position/source offset and shifted automation coordinates;
no preview mutates the musical model. Added four-scale, panned/partly off-screen
projection regressions across tempo changes/ramps, guarding both translation and
width against the former averaged-seconds/fixed-width implementation.

All **29 tests passed**. Isolated native **100%** checks on a project with tempo and
meter changes confirm:

- Loop move and both resize edges visibly update during dragging; RGB comparisons
  of the loop strip show identical preview and committed coverage.
- Draw and Pointer double-click near raw beat 27 create notes starting at **24** with
  **7-beat** duration. Draw's preview remains provisional.
- Moving the source range from beats 4–8 across a 120-to-60 BPM change paints the
  target outline at physical columns **795–814**, half its old width. Release
  creates the target segment at **4.5 seconds**, duration **2 seconds**, matching
  the projected beats 8.5–10.5.
- A **0.5-second** browser WAV preview is **10 pixels** wide at 120 BPM and **5 pixels**
  at 60 BPM. Drop on an audio track commits **5 seconds / 0.5-second duration**.

Evidence: `/tmp/nextstudio-review-fixes/` (build/test logs, isolated fixture,
`fixes-*.png/json`, and `runtime-checks.json`). Native higher-scale UI was not
repeated for this follow-up. Automation range rendering, proxy/fade combinations
and multi-track/take/loop combinations still need broader runtime acceptance;
helper tests and the focused checks are not full GUI certification.

## Delivered artifact

`/home/ai/Gemeinsam/NextStudio`, executable mode `0755`, produced by the required shared build/copy script. Built and shared binaries have identical SHA-256:

```text
00c9f789151dcbf1f42871261735e022f36943fb39358bb5e9b4c1e9d2fd0d91
```

The maintainer accepted this implementation and explicitly approved commit/push and marking #77 fixed: “Ich würde es erstmal so committen. pushen. Es fixed die Issue.” #77 is marked complete in `Todo.md`; the implementation commit uses `Fixes #77`. The coverage limitations above remain applicable.

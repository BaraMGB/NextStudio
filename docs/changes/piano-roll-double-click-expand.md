# Piano Roll Double-Click Expansion

## Summary

Double-clicking a MIDI clip in the arrangement now opens the Piano Roll even when the lower range is collapsed. The lower range is expanded first, then the Piano Roll is focused on the clicked clip's track and centered to that clip.

## Previous behavior

The arrangement already routed MIDI-clip double-clicks into the Piano Roll track-selection path, but this did not reopen the lower range. When the lower range was collapsed, the internal target track changed without making the Piano Roll visible.

## Implementation

### `App/include/Utilities.h` and `App/src/Utilities.cpp`

The shared lower-range open logic now lives in `EngineHelpers` instead of a dedicated one-off header:

- `shouldOpenMidiEditorForArrangementClip()` keeps the arrangement-side activation rule in one place;
- `openLowerRangeView()` is used by the collapsed lower-range buttons to select a view and expand the lower range through the shared path;
- `openMidiEditorForTrack()` is used by arrangement MIDI-clip activation to target the track and optionally expand the lower range.

### `App/src/TrackLaneComponent.cpp`

The clip interaction path now:

1. detects whether the current click is a double-click;
2. evaluates the shared activation helper in `EngineHelpers`;
3. opens the MIDI Editor for the clicked track through `EngineHelpers::openMidiEditorForTrack()`;
4. keeps the existing centering behavior for double-clicks.

### `App/src/LowerRangeComponent.cpp`

The collapsed lower-range buttons now call `EngineHelpers::openLowerRangeView()` instead of duplicating the expand logic locally.

This removes the redundant one-off activation header while preserving prior single-click behavior and adding the missing collapsed-state reopen step.

## Tests

This refactor reuses the existing lower-range and arrangement code paths instead of introducing a separate helper-only unit under `App/include/`. Validation is therefore covered by the normal project build plus the existing focused splitter tests.

## Documentation updates

Updated:

- `docs/user/getting-started.md`
- `docs/user/piano-roll.md`
- `docs/ui/song-editor.md`
- `docs/development/testing.md`

## Validation

Validated with:

```bash
BUILD_JOBS=12 ./build.sh rd
cd autobuild/RelWithDebInfo && ctest --output-on-failure -R SplitterCollapseController
```

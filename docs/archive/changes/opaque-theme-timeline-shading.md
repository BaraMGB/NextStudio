# Opaque theme timeline shading

- Type: historical record
- Audience: contributors
- Status: implemented; [#88 closed](https://github.com/BaraMGB/NextStudio/issues/88)
- Current reference: [timeline rendering](../../components/timeline-view-transform.md#timeline-band-rendering)

This is the original rationale/test checklist. A manual checklist is not evidence that every listed theme/lane was runtime-tested; maintainer acceptance is recorded in the issue history and pre-migration work log.

## Problem

Theme normalization intentionally stores every configurable color as opaque `#RRGGBB` data. The alternating timeline-band renderer previously consumed the alpha byte of `timeLineShadowShade` directly. After normalization changed the built-in Dark and Light values from translucent ARGB to opaque ARGB, `GUIHelpers::drawBarBeatsShadow()` filled alternating ranges at full opacity. This obscured piano-key striping in `MidiViewport` and over-darkened arrangement, velocity, and automation lanes.

The other translucent colors used by editor previews, selections, grid lines, and clip ranges are draw-time effects rather than persisted theme values. They do not need to become opaque.

## Design

`timeLineShadowShade` remains an opaque theme RGB tint. `App/include/TimelineGridColours.h` defines the renderer-owned band opacity and converts the tint into the draw color:

```cpp
inline constexpr float bandOverlayAlpha = 0.30f;

inline juce::Colour makeBandOverlay(juce::Colour themeTint)
{
    return themeTint.withAlpha(bandOverlayAlpha);
}
```

`GUIHelpers::drawBarsAndBeatLines()` performs this conversion once before delegating to `drawBarBeatsShadow()`. Every consumer of the shared timeline grid therefore receives the same behavior without duplicating opacity logic.

This keeps the theme editor RGB-only, preserves compatibility with opaque `.nxttheme` files, and allows already painted lane content to remain visible through alternating bands.

## Coverage

`ThemePresetModelTests` verifies that the conversion preserves the configured RGB channels and applies the renderer-owned alpha. Existing preset tests continue to verify that persisted theme colors are normalized to full opacity.

The manual validation checklist covers the built-in Dark and Light themes in:

- Song Editor track and automation lanes;
- Piano Roll note grid and piano-key rows;
- Velocity editor;
- timeline grid overlays and clip ranges.

## Relevant files

- `App/include/TimelineGridColours.h`
- `App/src/Utilities.cpp`
- `App/tests/ThemePresetModelTests.cpp`
- `docs/components/theme-settings.md`
- `docs/components/piano-roll-editor.md`
- `docs/user/piano-roll.md`
- `docs/ui/song-editor.md`

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace TimelineGridColours
{
/** Theme colors are persisted as opaque RGB values. Alternating timeline bands
    interpret their configured shade as a render-time tint so content painted
    below the grid remains visible. */
inline constexpr float bandOverlayAlpha = 0.30f;

struct LineColours
{
    juce::Colour bar, beat, quarterBeat, subdivision;
};

inline LineColours makeLineColours(juce::Colour stroke, int gridLevel)
{
    // At 1/8-beat and finer grids, quarter-beat landmarks must remain stronger
    // than the intervening subdivisions (including adaptive level 6, #99).
    return {stroke.withAlpha(0.4f), stroke.withAlpha(0.25f), stroke.withAlpha(gridLevel <= 6 ? 0.2f : 0.1f), stroke.withAlpha(0.08f)};
}

inline juce::Colour makeBandOverlay(juce::Colour themeTint)
{
    return themeTint.withAlpha(bandOverlayAlpha);
}
} // namespace TimelineGridColours

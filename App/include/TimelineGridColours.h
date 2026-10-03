#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace TimelineGridColours
{
/** Theme colors are persisted as opaque RGB values. Alternating timeline bands
    interpret their configured shade as a render-time tint so content painted
    below the grid remains visible. */
inline constexpr float bandOverlayAlpha = 0.30f;

inline juce::Colour makeBandOverlay(juce::Colour themeTint)
{
    return themeTint.withAlpha(bandOverlayAlpha);
}
} // namespace TimelineGridColours

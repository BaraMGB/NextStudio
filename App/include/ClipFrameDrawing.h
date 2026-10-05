#pragma once

#include <juce_graphics/juce_graphics.h>

namespace ClipFrameDrawing
{
// Keep the same unrounded edges as clip fills/content. Rounding x and width
// separately can move the right frame edge away from the next clip's left edge.
// Called after clip content within the caller's viewport clip region.
inline void draw(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour normalColour, juce::Colour selectedColour, bool isSelected)
{
    g.setColour(normalColour);
    g.drawRect(bounds, 1.0f);
    if (isSelected)
    {
        g.setColour(selectedColour);
        g.drawRect(bounds, 2.0f);
    }
}
} // namespace ClipFrameDrawing

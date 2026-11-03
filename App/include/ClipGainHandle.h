#pragma once

#include <juce_graphics/juce_graphics.h>
#include <optional>

namespace ClipGainHandle
{
constexpr float radius = 3.0f;

struct Geometry
{
    juce::Point<float> centre;
    float left{}, right{};
    juce::Rectangle<float> circle() const { return {centre.x - radius, centre.y - radius, 2 * radius, 2 * radius}; }
    bool hit(juce::Point<float> point) const { return centre.getDistanceSquaredFrom(point) <= radius * radius; }
};

// Shared by drawing and hit testing. Reserve the header, edge resize zones,
// and the upper fade-handle strip. Small clips have no gain interaction.
inline std::optional<Geometry> geometry(juce::Rectangle<float> clip, float headerHeight, float gainDb)
{
    auto body = clip.withTrimmedTop(headerHeight);
    // 24px fits the fade exclusion, circle and bottom padding while leaving
    // room for the handle on the default 50px track with its 20px header.
    if (clip.getWidth() < 40.0f || body.getHeight() < 24.0f) return {};
    const float top = body.getY() + 16.0f, bottom = body.getBottom() - 6.0f;
    const float proportion = (juce::jlimit(-100.0f, 24.0f, gainDb) + 100.0f) / 124.0f;
    return Geometry{{clip.getCentreX(), bottom - proportion * (bottom - top)}, clip.getX() + 12.0f, clip.getRight() - 12.0f};
}
}

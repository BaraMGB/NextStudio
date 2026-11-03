#pragma once

#include <tracktion_engine/tracktion_engine.h>
#include <cmath>

// Shared by the folder lane and its drag ghost. Geometry remains in timeline
// coordinates (including fractional/off-screen edges); drawing never edits the model.
namespace FolderCollectionDrawing
{
inline double luminance(juce::Colour colour)
{
    const auto linear = [](double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
    return 0.2126 * linear(colour.getFloatRed()) + 0.7152 * linear(colour.getFloatGreen())
         + 0.0722 * linear(colour.getFloatBlue());
}

inline double contrast(juce::Colour a, juce::Colour b)
{
    const auto x = luminance(a), y = luminance(b);
    return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
}

// Preserve the theme/clip hue where possible, but don't assume Light has a
// light arrangement background, or that its accent contrasts with that background.
inline juce::Colour legible(juce::Colour colour, juce::Colour first, juce::Colour second, double ratio)
{
    const auto score = [&](juce::Colour c) { return std::min(contrast(c, first), contrast(c, second)); };
    const auto neutral = score(juce::Colours::white) > score(juce::Colours::black)
        ? juce::Colours::white : juce::Colours::black;
    colour = colour.withAlpha(1.0f);
    for (int step = 0; step <= 32; ++step)
    {
        const auto candidate = colour.interpolatedWith(neutral, float(step) / 32.0f);
        if (score(candidate) >= ratio) return candidate;
    }
    return neutral;
}

struct Palette
{
    juce::Colour body, border, accent, text;
};

inline Palette palette(juce::Colour lane, juce::Colour folder, juce::Colour accent)
{
    const auto body = lane.withAlpha(1.0f).interpolatedWith(folder.withAlpha(1.0f), 0.18f);
    return {body, legible(folder, lane, body, 3.0), legible(accent, lane, body, 3.0),
            legible(juce::Colours::white, body, body, 4.5)};
}

template <typename ProjectTime>
void draw(juce::Graphics& g, const tracktion::engine::CollectionClip& collection,
          const juce::Array<tracktion::engine::AudioTrack*>& tracks, juce::Rectangle<float> bounds,
          ProjectTime projectTime, juce::Colour lane, juce::Colour accent,
          bool selected, bool hovered, float opacity = 1.0f)
{
    if (bounds.isEmpty()) return;
    juce::Graphics::ScopedSaveState saved(g);
    juce::Path shape;
    shape.addRoundedRectangle(bounds, juce::jmin(3.0f, bounds.getWidth() * 0.25f));
    g.reduceClipRegion(shape);
    if (opacity < 1.0f) g.beginTransparencyLayer(opacity);
    const auto colours = palette(lane, collection.getTrack()->getColour(), accent);
    const auto body = hovered && !selected ? colours.body.interpolatedWith(colours.text, 0.08f) : colours.body;
    g.setColour(body);
    g.fillRect(bounds); // Opaque surface keeps the timeline grid out of the summary.

    auto contents = bounds.reduced(2.0f);
    const int rows = juce::jmax(1, tracks.size());
    // Small/dense collections keep their full content area instead of squeezing
    // strips below a useful size just to fit a caption.
    if (bounds.getWidth() >= 90.0f && contents.getHeight() >= 12.0f + rows * 2.0f)
    {
        auto caption = contents.removeFromTop(11.0f);
        auto label = juce::String(collection.getNumClips()) + (collection.getNumClips() == 1 ? " clip" : " clips");
        if (bounds.getWidth() >= 180.0f) label = collection.getTrack()->getName() + "  /  " + label;
        g.setColour(legible(colours.text, body, body, 4.5));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawText(label, caption.reduced(3.0f, 0.0f), juce::Justification::centredLeft, true);
        contents.removeFromTop(1.0f);
    }

    const float rowHeight = contents.getHeight() / rows;
    if (rowHeight > 0.0f && contents.getWidth() > 0.0f)
    {
        // Stable child-track rows, including empty rows, preserve the hierarchy.
        // Dense folders use contiguous bands rather than overlapping min-height bars.
        const float gap = rowHeight >= 3.0f ? 1.0f : 0.0f;
        for (int row = 0; row < rows; ++row)
        {
            g.setColour(colours.text.withAlpha(row % 2 == 0 ? 0.07f : 0.03f));
            g.fillRect(contents.withY(contents.getY() + row * rowHeight).withHeight(rowHeight - gap));
        }
        for (auto* clip : collection.getClips())
        {
            const int row = tracks.indexOf(dynamic_cast<tracktion::engine::AudioTrack*>(clip->getTrack()));
            if (row < 0) continue;
            const auto position = clip->getPosition();
            const float left = projectTime(position.getStart()), right = projectTime(position.getEnd());
            auto strip = juce::Rectangle<float>(left, contents.getY() + row * rowHeight, right - left, rowHeight - gap)
                .getIntersection(contents);
            g.setColour(legible(clip->getColour(), body, body, 3.0));
            g.fillRect(strip);
        }
    }

    // A contrast-safe double-width frame remains recognisable without relying
    // solely on the theme accent's hue. Hover has its own lighter, single frame.
    g.setColour(selected ? colours.accent : hovered ? colours.text : colours.border);
    if (bounds.getWidth() <= 2.0f || bounds.getHeight() <= 2.0f)
        g.fillRect(bounds);
    else
        g.drawRoundedRectangle(bounds.reduced(selected ? 1.0f : 0.5f), 2.0f, selected ? 2.0f : 1.0f);
    if (opacity < 1.0f) g.endTransparencyLayer();
}
}

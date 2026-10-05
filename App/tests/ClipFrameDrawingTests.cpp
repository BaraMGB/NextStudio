#include "ClipFrameDrawing.h"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr float viewportWidth = 650.0f;
const juce::Colour background(0xff3a3a3a);
const juce::Colour content(0xff27354a);
const juce::Colour normal(0xffeeeeee);
const juce::Colour selected(0xffe5cf03);

// An independent edge-band reference, rasterized as one union like a frame
// (individual fillRect calls use a different 8-bit coverage fast path). Test rows
// avoid corners. No position/width rounding is permitted here.
void referenceFrame(juce::Graphics &g, juce::Rectangle<float> bounds, bool isSelected)
{
    const auto verticalEdges = [&](float thickness)
    {
        juce::RectangleList<float> edges;
        edges.add(bounds.withWidth(thickness));
        edges.add(bounds.withLeft(bounds.getRight() - thickness));
        g.fillRectList(edges);
    };
    g.setColour(normal);
    verticalEdges(1.0f);
    if (isSelected)
    {
        g.setColour(selected);
        verticalEdges(2.0f);
    }
}

enum class Renderer
{
    production,
    reference,
    legacy
};
juce::Image render(float origin, float width, float scale, int selectedMask, bool reverse, Renderer renderer)
{
    juce::Image image(juce::Image::RGB, int(std::ceil(viewportWidth * scale)), int(std::ceil(48.0f * scale)), true);
    juce::Graphics g(image);
    g.fillAll(background);
    g.addTransform(juce::AffineTransform::scale(scale));
    g.reduceClipRegion(juce::Rectangle<int>(0, 0, int(viewportWidth), 48));
    // Both rectangles come from shared time edges, not independent rounded widths.
    const float join = origin + width;
    const std::array bounds{juce::Rectangle<float>::leftTopRightBottom(origin, 4.0f, join, 40.0f), juce::Rectangle<float>::leftTopRightBottom(join, 4.0f, join + width, 40.0f)};
    for (int index : (reverse ? std::array{1, 0} : std::array{0, 1}))
    {
        const bool isSelected = (selectedMask & (1 << index)) != 0;
        g.setColour(content);
        g.fillRect(bounds[size_t(index)].reduced(1.0f));
        switch (renderer)
        {
        case Renderer::production:
            ClipFrameDrawing::draw(g, bounds[size_t(index)], normal, selected, isSelected);
            break;
        case Renderer::reference:
            referenceFrame(g, bounds[size_t(index)], isSelected);
            break;
        case Renderer::legacy:
            g.setColour(normal);
            g.drawRect(bounds[size_t(index)].toNearestInt());
            if (isSelected)
            {
                g.setColour(selected);
                g.drawRect(bounds[size_t(index)].toNearestInt(), 2);
            }
            break;
        }
    }
    return image;
}

int maxRowDifference(const juce::Image &a, const juce::Image &b, float scale)
{
    int difference = 0;
    for (float logicalY : {20.0f, 28.0f})
        for (int x = 0; x < a.getWidth(); ++x)
        {
            const int y = int(logicalY * scale);
            const auto first = a.getPixelAt(x, y);
            const auto second = b.getPixelAt(x, y);
            difference = std::max({difference, std::abs(int(first.getRed()) - int(second.getRed())), std::abs(int(first.getGreen()) - int(second.getGreen())), std::abs(int(first.getBlue()) - int(second.getBlue()))});
        }
    return difference;
}

void tests()
{
    // The original regression must actually be detectable by the reference.
    const auto legacy = render(12.4f, 28.4f, 1.0f, 0, false, Renderer::legacy);
    const auto reference = render(12.4f, 28.4f, 1.0f, 0, false, Renderer::reference);
    if (maxRowDifference(legacy, reference, 1.0f) < 20)
        throw std::runtime_error("reference does not expose the old independent rounding");

    int maximumReferenceDifference = 0;
    for (float scale : {1.0f, 1.25f, 1.5f, 2.0f})
        for (float origin : {12.0f, 12.137f, 12.4f, 12.5f, 12.83f, -16.533333f})
            for (float width : {40.0f, 28.6f, 28.4f, 26.64f, 256.64f})
                for (int mask : {0, 1, 2, 3})
                    for (bool reverse : {false, true})
                    {
                        const auto actual = render(origin, width, scale, mask, reverse, Renderer::production);
                        const auto expected = render(origin, width, scale, mask, reverse, Renderer::reference);
                        const auto difference = maxRowDifference(actual, expected, scale);
                        maximumReferenceDifference = std::max(maximumReferenceDifference, difference);
                        if (difference > 1)
                        {
                            for (int x = 0; x < actual.getWidth(); ++x)
                            {
                                const auto a = actual.getPixelAt(x, int(20.0f * scale));
                                const auto b = expected.getPixelAt(x, int(20.0f * scale));
                                if (a != b)
                                    std::cerr << "x=" << x << " actual=" << a.toDisplayString(true) << " expected=" << b.toDisplayString(true) << '\n';
                            }
                            std::cerr << "difference=" << difference << " scale=" << scale << " origin=" << origin << " width=" << width << " selectedMask=" << mask << " reverse=" << reverse << '\n';
                            throw std::runtime_error("clip frame does not follow the shared float edges");
                        }
                    }
    std::cout << "Maximum reference channel difference: " << maximumReferenceDifference << '\n';
    // Integer-coordinate appearance and selection colors/thickness remain unchanged.
    for (int mask : {0, 1, 2, 3})
    {
        const auto actual = render(12.0f, 40.0f, 1.0f, mask, false, Renderer::production);
        const auto previous = render(12.0f, 40.0f, 1.0f, mask, false, Renderer::legacy);
        for (int y = 0; y < actual.getHeight(); ++y)
            for (int x = 0; x < actual.getWidth(); ++x)
                if (actual.getPixelAt(x, y) != previous.getPixelAt(x, y))
                    throw std::runtime_error("integer-coordinate frame appearance changed");
    }
}
} // namespace

int main()
{
    try
    {
        tests();
        std::cout << "Clip frame float-edge raster tests passed\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

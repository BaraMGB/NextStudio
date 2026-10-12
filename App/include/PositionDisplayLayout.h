#pragma once

#include <juce_graphics/juce_graphics.h>

namespace PositionDisplayLayout
{
inline constexpr int fullMinimumWidth = 560;
inline constexpr int compactMinimumWidth = 360;
inline constexpr int outerPaddingX = 8;
inline constexpr int outerPaddingY = 6;
inline constexpr int fullInfoWidth = 180;
inline constexpr int compactInfoWidth = 120;
inline constexpr int minimumInfoWidth = 80;
inline constexpr int fullInfoGap = 10;
inline constexpr int compactInfoGap = 8;
inline constexpr int fieldGap = 10;

enum class Mode
{
    full,
    compact,
    minimal
};

struct Layout
{
    Mode mode{Mode::full};
    juce::Rectangle<int> info;
    juce::Rectangle<int> left;
    juce::Rectangle<int> center;
    juce::Rectangle<int> right;

    bool showsSideFields() const noexcept { return mode != Mode::minimal; }
    bool showsLabels() const noexcept { return mode == Mode::full; }
};

inline Mode modeForWidth(int width) noexcept
{
    if (width >= fullMinimumWidth)
        return Mode::full;
    if (width >= compactMinimumWidth)
        return Mode::compact;
    return Mode::minimal;
}

inline Layout calculate(juce::Rectangle<int> bounds, int preferredLeftWidth, int preferredRightWidth)
{
    Layout result;
    result.mode = modeForWidth(bounds.getWidth());

    auto area = bounds.reduced(outerPaddingX, outerPaddingY);
    if (area.isEmpty())
        return result;

    const auto infoWidth = result.mode == Mode::full
                             ? juce::jmin(fullInfoWidth, area.getWidth())
                             : result.mode == Mode::compact
                                 ? juce::jmin(compactInfoWidth, area.getWidth())
                                 : juce::jmin(area.getWidth(), juce::jmax(minimumInfoWidth, area.getWidth() / 3));
    result.info = area.removeFromRight(infoWidth);

    if (area.isEmpty())
        return result;

    const auto infoGap = juce::jmin(result.mode == Mode::full ? fullInfoGap : compactInfoGap,
                                    area.getWidth());
    area.removeFromRight(infoGap);
    if (area.isEmpty())
        return result;

    if (result.mode == Mode::minimal)
    {
        result.center = area;
        return result;
    }

    const auto minimumCenterWidth = result.mode == Mode::full
                                      ? juce::roundToInt(area.getWidth() * 0.36f)
                                      : juce::jmin(116, area.getWidth());
    const auto maximumSideWidth = juce::jmax(0, (area.getWidth() - minimumCenterWidth - fieldGap * 2) / 2);

    result.left = area.removeFromLeft(juce::jmin(juce::jmax(0, preferredLeftWidth), maximumSideWidth));
    area.removeFromLeft(juce::jmin(fieldGap, area.getWidth()));
    result.right = area.removeFromRight(juce::jmin(juce::jmax(0, preferredRightWidth), maximumSideWidth));
    area.removeFromRight(juce::jmin(fieldGap, area.getWidth()));
    result.center = area;
    return result;
}
} // namespace PositionDisplayLayout

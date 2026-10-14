#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

namespace HeaderLayout
{
struct Layout
{
    juce::Rectangle<int> positionDisplay;
    // Play, Stop, Record, Count In, Metronome, Loop, Undo, Redo.
    std::array<juce::Rectangle<int>, 8> buttons;
};

// Keep the widened Position Display centred. At narrow widths it contracts only
// after the side controls have reached compact, non-overlapping slots.
inline Layout calculate(int width, int height, int preferredDisplayWidth)
{
    width = juce::jmax(0, width);
    height = juce::jmax(0, height);
    constexpr int compactSideWidth = 140;
    const auto gap = height / 8;
    const auto displayWidth = juce::jmin(juce::jmax(0, width - compactSideWidth * 2),
                                         juce::jmax(0, preferredDisplayWidth));
    const auto leftWidth = (width - displayWidth) / 2;
    const auto rightWidth = width - displayWidth - leftWidth;
    const auto leftSlot = juce::jmin(height, leftWidth / 4);
    const auto rightSlot = juce::jmin(height, rightWidth / 4);
    const auto leftStart = leftWidth - leftSlot * 4;
    const auto rightStart = leftWidth + displayWidth;

    Layout result;
    result.positionDisplay = juce::Rectangle<int>(leftWidth, 0, displayWidth, height)
                                 .reduced(juce::jmin(gap, displayWidth / 2), gap);
    const auto button = [height, gap](int x, int slot)
    {
        return juce::Rectangle<int>(x, 0, slot, height)
            .reduced(juce::jmin(gap, slot / 8), gap);
    };

    for (int i = 0; i < 4; ++i)
        result.buttons[(size_t) i] = button(leftStart + leftSlot * i, leftSlot);
    for (int i = 0; i < 2; ++i)
        result.buttons[(size_t) (4 + i)] = button(rightStart + rightSlot * i, rightSlot);
    result.buttons[6] = button(width - rightSlot * 2, rightSlot);
    result.buttons[7] = button(width - rightSlot, rightSlot);
    return result;
}
} // namespace HeaderLayout

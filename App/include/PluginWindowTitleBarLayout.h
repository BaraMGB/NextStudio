#pragma once

#include <juce_graphics/juce_graphics.h>

namespace PluginWindowTitleBarLayout
{
struct Layout
{
    juce::Rectangle<int> pinButton;
    juce::Rectangle<int> titleSpace;
    bool controlsOnLeft{};
};

inline Layout calculate(juce::Rectangle<int> titleBarArea, juce::Rectangle<int> closeButtonBounds)
{
    Layout result;
    if (titleBarArea.isEmpty())
        return result;

    const auto localClose = closeButtonBounds.translated(-titleBarArea.getX(), -titleBarArea.getY());
    const auto localArea = titleBarArea.withPosition(0, 0);
    result.controlsOnLeft = !localClose.isEmpty() && localClose.getCentreX() < localArea.getCentreX();

    constexpr int outerMargin = 6;
    constexpr int controlGap = 3;
    constexpr int titleGap = 6;
    const auto buttonSize = juce::jmax(0, juce::jmin(20, localArea.getHeight() - 6));
    const auto fallbackRight = localArea.getRight() - outerMargin;

    int pinX = fallbackRight - buttonSize;
    if (!localClose.isEmpty())
        pinX = result.controlsOnLeft ? localClose.getRight() + controlGap
                                     : localClose.getX() - controlGap - buttonSize;

    pinX = juce::jlimit(localArea.getX(), juce::jmax(localArea.getX(), localArea.getRight() - buttonSize), pinX);
    const auto pinY = localArea.getY() + (localArea.getHeight() - buttonSize) / 2;
    result.pinButton = juce::Rectangle<int>(pinX, pinY, buttonSize, buttonSize)
                           .translated(titleBarArea.getX(), titleBarArea.getY());

    auto titleLeft = localArea.getX() + outerMargin;
    auto titleRight = localArea.getRight() - outerMargin;
    const auto localPin = result.pinButton.translated(-titleBarArea.getX(), -titleBarArea.getY());

    if (result.controlsOnLeft)
        titleLeft = juce::jmax(titleLeft, localPin.getRight() + titleGap);
    else
        titleRight = juce::jmin(titleRight, localPin.getX() - titleGap);

    titleRight = juce::jmax(titleLeft, titleRight);
    result.titleSpace = juce::Rectangle<int>(titleLeft, localArea.getY(), titleRight - titleLeft, localArea.getHeight())
                            .translated(titleBarArea.getX(), titleBarArea.getY());
    return result;
}
} // namespace PluginWindowTitleBarLayout

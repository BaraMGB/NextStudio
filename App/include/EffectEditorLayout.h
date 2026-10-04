/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace EffectEditorLayout
{
struct Rectangle
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr int right() const { return x + width; }
    [[nodiscard]] constexpr int bottom() const { return y + height; }
};

inline constexpr int pitchShifterWidthFactor = 1;

struct PitchShifterLayout
{
    Rectangle graph;
    Rectangle parameter;
};

struct CompressorLayout
{
    Rectangle graph;
    std::array<Rectangle, 6> parameters;
    Rectangle sidechainLabel;
    Rectangle sidechainSource;
    Rectangle sidechainTrigger;
};

struct DelayLayout
{
    Rectangle graph;
    std::array<Rectangle, 3> choices;
    std::array<Rectangle, 7> parameters;
};

[[nodiscard]] constexpr Rectangle inset(Rectangle area, int horizontal, int vertical)
{
    const auto xInset = std::clamp(horizontal, 0, std::max(0, area.width / 2));
    const auto yInset = std::clamp(vertical, 0, std::max(0, area.height / 2));
    return {area.x + xInset, area.y + yInset, std::max(0, area.width - xInset * 2), std::max(0, area.height - yInset * 2)};
}

[[nodiscard]] constexpr Rectangle column(Rectangle area, int count, int index)
{
    if (count <= 0 || index < 0 || index >= count)
        return {};

    const auto left = area.x + (area.width * index) / count;
    const auto right = area.x + (area.width * (index + 1)) / count;
    return {left, area.y, std::max(0, right - left), area.height};
}

[[nodiscard]] constexpr Rectangle row(Rectangle area, int count, int index)
{
    if (count <= 0 || index < 0 || index >= count)
        return {};

    const auto top = area.y + (area.height * index) / count;
    const auto bottom = area.y + (area.height * (index + 1)) / count;
    return {area.x, top, area.width, std::max(0, bottom - top)};
}

[[nodiscard]] constexpr int graphWidthFor(int contentWidth)
{
    constexpr int minimumGraphWidth = 140;
    constexpr int maximumGraphWidth = 210;
    constexpr int minimumControlsWidth = 160;
    constexpr int sectionGap = 4;

    const auto preferred = std::clamp((contentWidth * 42) / 100, minimumGraphWidth, maximumGraphWidth);
    return std::clamp(preferred, 0, std::max(0, contentWidth - sectionGap - minimumControlsWidth));
}

[[nodiscard]] constexpr int delayGraphWidthFor(int contentWidth)
{
    constexpr int minimumGraphWidth = 150;
    constexpr int maximumGraphWidth = 210;
    constexpr int preferredControlsWidth = 250;
    constexpr int sectionGap = 4;

    const auto preferred = std::clamp((contentWidth * 42) / 100, minimumGraphWidth, maximumGraphWidth);
    return std::clamp(preferred, 0, std::max(0, contentWidth - sectionGap - preferredControlsWidth));
}

[[nodiscard]] constexpr PitchShifterLayout pitchShifter(int width, int height)
{
    constexpr int outerMargin = 4;
    constexpr int sectionGap = 4;
    constexpr int maximumContentWidth = 160;
    constexpr int maximumGraphHeight = 150;

    auto content = inset({0, 0, std::max(0, width), std::max(0, height)}, outerMargin, outerMargin);
    const auto contentWidth = std::min(content.width, maximumContentWidth);
    content.x += (content.width - contentWidth) / 2;
    content.width = contentWidth;

    // The standard control reserves 20 px for its title and 15 px for its value.
    const auto parameterHeight = std::min(content.height, std::clamp(content.width + 35, 95, 135));
    const auto gap = std::min(sectionGap, content.height - parameterHeight);
    const auto graphHeight = std::min(maximumGraphHeight, content.height - parameterHeight - gap);
    const auto top = content.y + (content.height - graphHeight - gap - parameterHeight) / 2;

    return {{content.x, top, content.width, graphHeight},
            {content.x, top + graphHeight + gap, content.width, parameterHeight}};
}

[[nodiscard]] constexpr CompressorLayout compressor(int width, int height)
{
    constexpr int outerMargin = 6;
    constexpr int sectionGap = 4;
    constexpr int cellInset = 2;

    CompressorLayout result;
    const auto content = inset({0, 0, std::max(0, width), std::max(0, height)}, outerMargin, outerMargin);
    const auto footerHeight = std::min(content.height, std::clamp(content.height / 8, 28, 36));
    const auto footer = Rectangle{content.x, content.bottom() - footerHeight, content.width, footerHeight};
    const auto mainHeight = std::max(0, content.height - footerHeight - sectionGap);
    const auto main = Rectangle{content.x, content.y, content.width, mainHeight};
    const auto graphWidth = graphWidthFor(main.width);

    result.graph = {main.x, main.y, graphWidth, main.height};
    const auto controlsX = result.graph.right() + sectionGap;
    const auto controls = Rectangle{controlsX, main.y, std::max(0, main.right() - controlsX), main.height};

    for (int i = 0; i < 6; ++i)
        result.parameters[static_cast<std::size_t>(i)] = inset(column(row(controls, 2, i / 3), 3, i % 3), cellInset, cellInset);

    const auto labelWidth = (footer.width * 25) / 100;
    const auto triggerWidth = (footer.width * 32) / 100;
    result.sidechainLabel = inset({footer.x, footer.y, labelWidth, footer.height}, cellInset, 0);
    result.sidechainTrigger = inset({footer.right() - triggerWidth, footer.y, triggerWidth, footer.height}, cellInset, 0);
    result.sidechainSource = inset({footer.x + labelWidth, footer.y, std::max(0, footer.width - labelWidth - triggerWidth), footer.height}, cellInset, 0);

    return result;
}

[[nodiscard]] constexpr DelayLayout delay(int width, int height)
{
    constexpr int outerMargin = 4;
    constexpr int sectionGap = 4;
    constexpr int cellInset = 2;

    DelayLayout result;
    const auto content = inset({0, 0, std::max(0, width), std::max(0, height)}, outerMargin, outerMargin);
    const auto graphWidth = delayGraphWidthFor(content.width);
    result.graph = {content.x, content.y, graphWidth, content.height};

    const auto controlsX = result.graph.right() + sectionGap;
    const auto controls = Rectangle{controlsX, content.y, std::max(0, content.right() - controlsX), content.height};
    const auto choiceRowHeight = std::min(controls.height, std::clamp(controls.height / 4, 64, 68));
    const auto choiceRow = Rectangle{controls.x, controls.y, controls.width, choiceRowHeight};
    const auto knobRowsY = choiceRow.bottom() + sectionGap;
    const auto knobRows = Rectangle{controls.x, knobRowsY, controls.width, std::max(0, controls.bottom() - knobRowsY)};
    const auto mainKnobRow = row(knobRows, 2, 0);
    const auto detailKnobRow = row(knobRows, 2, 1);
    const auto modeWidth = (choiceRow.width * 42) / 100;
    const auto syncWidth = (choiceRow.width * 28) / 100;

    result.choices[0] = inset({choiceRow.x, choiceRow.y, modeWidth, choiceRow.height}, cellInset, cellInset);
    result.choices[1] = inset({choiceRow.x + modeWidth, choiceRow.y, syncWidth, choiceRow.height}, cellInset, cellInset);
    result.choices[2] = inset({choiceRow.x + modeWidth + syncWidth, choiceRow.y, std::max(0, choiceRow.width - modeWidth - syncWidth), choiceRow.height}, cellInset, cellInset);

    for (int i = 0; i < 3; ++i)
        result.parameters[static_cast<std::size_t>(i)] = inset(column(mainKnobRow, 3, i), cellInset, cellInset);

    for (int i = 0; i < 4; ++i)
        result.parameters[static_cast<std::size_t>(i + 3)] = inset(column(detailKnobRow, 4, i), cellInset, cellInset);

    return result;
}
} // namespace EffectEditorLayout

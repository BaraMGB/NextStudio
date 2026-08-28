/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

namespace LowerRangeLayout
{
inline constexpr int collapsedHeight = 38;
inline constexpr int defaultExpandedHeight = 350;
inline constexpr int mainContentMargin = 10;
inline constexpr int editorContainerHeaderHeight = 60;
inline constexpr int editorContainerHeaderGap = 10;
inline constexpr int clipPropertiesHeight = 30;

[[nodiscard]] constexpr int getPreferredExpandedHeight(int storedHeight)
{
    return storedHeight >= defaultExpandedHeight ? storedHeight : defaultExpandedHeight;
}

[[nodiscard]] constexpr int getMinimumEditorContainerHeight(int timelineHeight)
{
    return editorContainerHeaderHeight + editorContainerHeaderGap + clipPropertiesHeight + (2 * timelineHeight);
}

[[nodiscard]] constexpr int getMaximumExpandedHeight(int mainComponentHeight, int timelineHeight)
{
    const auto availableHeight = mainComponentHeight - (2 * mainContentMargin) - getMinimumEditorContainerHeight(timelineHeight);
    return availableHeight >= defaultExpandedHeight ? availableHeight : defaultExpandedHeight;
}

[[nodiscard]] constexpr int clampExpandedHeight(int expandedHeight, int maximumExpandedHeight)
{
    const auto preferredExpandedHeight = getPreferredExpandedHeight(expandedHeight);
    return preferredExpandedHeight <= maximumExpandedHeight ? preferredExpandedHeight : maximumExpandedHeight;
}

[[nodiscard]] constexpr int getTransitionDistance(int expandedHeight, bool collapsed)
{
    const auto preferredExpandedHeight = getPreferredExpandedHeight(expandedHeight);
    return collapsed
             ? preferredExpandedHeight - collapsedHeight
             : (preferredExpandedHeight - defaultExpandedHeight) + (defaultExpandedHeight - collapsedHeight);
}

[[nodiscard]] constexpr int getResizedHeight(int startHeight, int dragDistance, int maximumExpandedHeight)
{
    return clampExpandedHeight(startHeight - dragDistance, maximumExpandedHeight);
}

[[nodiscard]] constexpr int getAppliedDragDistance(int startHeight, int dragDistance, int maximumExpandedHeight)
{
    return startHeight - getResizedHeight(startHeight, dragDistance, maximumExpandedHeight);
}
} // namespace LowerRangeLayout

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

namespace SidebarLayout
{
inline constexpr int collapsedWidth = 70;
inline constexpr int defaultExpandedWidth = 300;
inline constexpr int minimumExpandedWidth = 250;
inline constexpr int collapseDragResistance = 100;

[[nodiscard]] constexpr int getPreferredWidth(int storedWidth)
{
    return storedWidth >= minimumExpandedWidth ? storedWidth : defaultExpandedWidth;
}

[[nodiscard]] constexpr int getTransitionDistance(int expandedWidth, bool collapsed)
{
    return collapsed
             ? expandedWidth - collapsedWidth
             : (expandedWidth - minimumExpandedWidth) + collapseDragResistance;
}

[[nodiscard]] constexpr int getResizedWidth(int startWidth, int dragDistance)
{
    const auto requestedWidth = startWidth + dragDistance;
    return requestedWidth >= minimumExpandedWidth ? requestedWidth : minimumExpandedWidth;
}
} // namespace SidebarLayout

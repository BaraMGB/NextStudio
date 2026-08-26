/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include <algorithm>

namespace PluginChainLayout
{
constexpr int getMaxScrollX(int contentWidth, int visibleWidth) { return std::max(0, contentWidth - visibleWidth); }

constexpr int getReorderDestinationIndex(int sourceIndex, int targetIndex, bool placeAfter, int itemCount)
{
    if (itemCount <= 0 || sourceIndex < 0 || sourceIndex >= itemCount || targetIndex < 0 || targetIndex >= itemCount)
        return -1;

    int destinationIndex = targetIndex + (placeAfter ? 1 : 0);
    if (sourceIndex < destinationIndex)
        --destinationIndex;

    return std::clamp(destinationIndex, 0, itemCount - 1);
}
} // namespace PluginChainLayout

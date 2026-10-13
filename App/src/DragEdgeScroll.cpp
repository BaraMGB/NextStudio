/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "DragEdgeScroll.h"

#include <algorithm>
#include <cmath>

namespace DragEdgeScroll
{
double velocityForAxis(double position, double extent, const Config& config)
{
    if (!std::isfinite(position) || !std::isfinite(extent) || extent <= 0.0
        || !std::isfinite(config.edgeSize) || config.edgeSize <= 0.0
        || !std::isfinite(config.maximumSpeed) || config.maximumSpeed <= 0.0)
        return 0.0;

    const double edge = std::min(config.edgeSize, extent * 0.5);
    if (edge <= 0.0)
        return 0.0;

    double amount = 0.0;
    if (position < edge)
        amount = -std::clamp((edge - position) / edge, 0.0, 1.0);
    else if (position > extent - edge)
        amount = std::clamp((position - (extent - edge)) / edge, 0.0, 1.0);

    return std::copysign(amount * amount * config.maximumSpeed, amount);
}

Velocity velocity(double x, double y, double width, double height, Axes axes, const Config& config)
{
    return {axes.horizontal ? velocityForAxis(x, width, config) : 0.0,
            axes.vertical ? velocityForAxis(y, height, config) : 0.0};
}

Delta advance(Velocity speed, double elapsedSeconds, const Config& config)
{
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0
        || !std::isfinite(config.maximumStepSeconds) || config.maximumStepSeconds <= 0.0)
        return {};

    const double elapsed = std::min(elapsedSeconds, config.maximumStepSeconds);
    return {speed.x * elapsed, speed.y * elapsed};
}

double clampInteractionPosition(double position, double extent, double inset)
{
    if (!std::isfinite(position) || !std::isfinite(extent) || extent <= 0.0)
        return 0.0;

    inset = std::isfinite(inset) ? std::clamp(inset, 0.0, extent * 0.5) : 0.0;
    return std::clamp(position, inset, extent - inset);
}
} // namespace DragEdgeScroll

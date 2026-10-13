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

#pragma once

namespace DragEdgeScroll
{
struct Config
{
    double edgeSize = 40.0;
    double maximumSpeed = 900.0;
    double maximumStepSeconds = 0.05;
};

struct Axes
{
    bool horizontal = true;
    bool vertical = true;
};

struct Velocity
{
    double x = 0.0;
    double y = 0.0;

    bool isZero() const { return x == 0.0 && y == 0.0; }
};

struct Delta
{
    double x = 0.0;
    double y = 0.0;
};

// Positions and extents are logical pixels. The quiet area is inset from each
// edge by Config::edgeSize; speed ramps quadratically towards the edge and is
// capped once the pointer leaves the viewport.
double velocityForAxis(double position, double extent, const Config&);
Velocity velocity(double x, double y, double width, double height, Axes, const Config& = {});
Delta advance(Velocity, double elapsedSeconds, const Config& = {});
double clampInteractionPosition(double position, double extent, double inset = 1.0);
} // namespace DragEdgeScroll

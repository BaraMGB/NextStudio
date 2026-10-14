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

#include <algorithm>
#include <cmath>
#include <optional>

class PanDragGesture
{
public:
    struct Point
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct Bounds
    {
        double minimum = 0.0;
        double maximum = 0.0;
    };

    bool begin(Point pointer, Point view, Point unitsPerPixel, Bounds horizontal, Bounds vertical)
    {
        if (!isFinite(pointer) || !isFinite(view) || !isFinite(unitsPerPixel)
            || !isValid(horizontal) || !isValid(vertical))
        {
            cancel();
            return false;
        }

        m_pointerStart = pointer;
        m_viewStart = view;
        m_unitsPerPixel = unitsPerPixel;
        m_horizontal = horizontal;
        m_vertical = vertical;
        m_active = true;
        return true;
    }

    std::optional<Point> update(Point pointer) const
    {
        if (!m_active || !isFinite(pointer))
            return {};

        return Point{
            std::clamp(m_viewStart.x - (pointer.x - m_pointerStart.x) * m_unitsPerPixel.x,
                       m_horizontal.minimum, m_horizontal.maximum),
            std::clamp(m_viewStart.y - (pointer.y - m_pointerStart.y) * m_unitsPerPixel.y,
                       m_vertical.minimum, m_vertical.maximum)};
    }

    void cancel() noexcept { m_active = false; }
    bool active() const noexcept { return m_active; }

private:
    static bool isFinite(Point point) noexcept
    {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    static bool isValid(Bounds bounds) noexcept
    {
        return std::isfinite(bounds.minimum) && std::isfinite(bounds.maximum)
            && bounds.minimum <= bounds.maximum;
    }

    Point m_pointerStart;
    Point m_viewStart;
    Point m_unitsPerPixel;
    Bounds m_horizontal;
    Bounds m_vertical;
    bool m_active = false;
};

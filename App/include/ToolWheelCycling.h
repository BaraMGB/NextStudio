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

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ToolWheelCycling
{
template <typename Value, std::size_t Size>
Value cycle(Value current, const std::array<Value, Size>& order, int direction)
{
    static_assert(Size > 0);

    std::size_t index = 0;
    while (index < Size && order[index] != current)
        ++index;

    if (index == Size)
        return direction >= 0 ? order.front() : order.back();

    if (direction >= 0)
        return order[(index + 1) % Size];

    return order[(index + Size - 1) % Size];
}

class Accumulator
{
public:
    int update(float delta, bool isSmooth, bool isInertial, std::int64_t timestampMs)
    {
        if (isInertial || !std::isfinite(delta) || std::abs(delta) <= std::numeric_limits<float>::epsilon())
            return 0;

        const int direction = delta > 0.0f ? 1 : -1;

        if (!isSmooth)
        {
            reset();
            return direction;
        }

        if (m_hasTimestamp && (timestampMs < m_lastTimestampMs || timestampMs - m_lastTimestampMs > resetDelayMs))
            m_accumulatedDelta = 0.0f;

        if ((m_accumulatedDelta > 0.0f && direction < 0) || (m_accumulatedDelta < 0.0f && direction > 0))
            m_accumulatedDelta = 0.0f;

        m_lastTimestampMs = timestampMs;
        m_hasTimestamp = true;
        m_accumulatedDelta += delta;

        if (std::abs(m_accumulatedDelta) < smoothStepThreshold)
            return 0;

        // A high-resolution event may be much larger than one threshold. Discard
        // the remainder so one event can never skip over several tools.
        m_accumulatedDelta = 0.0f;
        return direction;
    }

    void reset()
    {
        m_accumulatedDelta = 0.0f;
        m_lastTimestampMs = 0;
        m_hasTimestamp = false;
    }

private:
    static constexpr float smoothStepThreshold = 0.2f;
    static constexpr std::int64_t resetDelayMs = 250;

    float m_accumulatedDelta{0.0f};
    std::int64_t m_lastTimestampMs{0};
    bool m_hasTimestamp{false};
};
} // namespace ToolWheelCycling

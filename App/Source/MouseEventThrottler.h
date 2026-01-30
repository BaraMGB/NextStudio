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

#include "../JuceLibraryCode/JuceHeader.h"

/**
 * Utility class for throttling mouse events to prevent excessive processing.
 *
 * This class tracks mouse event timing and position to throttle high-frequency
 * events like mouseMove. It allows events through at a specified interval
 * (default ~60 FPS) and also when the mouse position changes significantly.
 */
class MouseEventThrottler
{
public:
    /**
     * Checks if a mouse event should be processed based on throttling rules.
     *
     * @param event The mouse event to check
     * @param intervalMs Minimum interval between processed events in milliseconds (default 16ms = ~60 FPS)
     * @return true if the event should be processed, false if it should be throttled
     */
    bool shouldProcess(const juce::MouseEvent &event, int intervalMs = 16)
    {
        const auto currentTime = juce::Time::getCurrentTime().toMilliseconds();
        const auto currentPos = event.getPosition();

        // Always process if enough time has passed
        if (currentTime - m_lastTime >= intervalMs)
        {
            m_lastTime = currentTime;
            m_lastPos = currentPos;
            return true;
        }

        // Always process if position changed significantly (more than 2 pixels)
        if (currentPos.getDistanceFrom(m_lastPos) > 2)
        {
            m_lastTime = currentTime;
            m_lastPos = currentPos;
            return true;
        }

        // Throttle this event
        return false;
    }

    /**
     * Resets the throttler state.
     * Call this when mouse handling is interrupted (e.g., on mouseExit).
     */
    void reset()
    {
        m_lastTime = 0;
        m_lastPos = juce::Point<int>(0, 0);
    }

private:
    juce::int64 m_lastTime = 0;
    juce::Point<int> m_lastPos;
};

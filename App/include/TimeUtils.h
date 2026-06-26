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
#include "EditViewState.h"

namespace te = tracktion_engine;

/**
 * Utility class for time/position conversions in the timeline.
 *
 * This class provides static helper methods for converting between:
 * - TimePosition (seconds) and screen X coordinates
 * - Applying snap/grid quantization
 *
 * All methods are static and thread-safe (read-only operations on EditViewState).
 */
class TimeUtils
{
public:
    /**
     * Converts a time position to screen X coordinate.
     *
     * @param time The time position to convert
     * @param editViewState The edit view state containing zoom/pan information
     * @param timeLineID The timeline identifier for view data lookup
     * @param width The width of the component in pixels
     * @return The X coordinate in pixels
     */
    static float timeToX(tracktion::TimePosition time, EditViewState &editViewState, const juce::String &timeLineID, int width)
    {
        auto x1 = editViewState.getVisibleBeatRange(timeLineID, width).getStart().inBeats();
        auto x2 = editViewState.getVisibleBeatRange(timeLineID, width).getEnd().inBeats();
        return editViewState.timeToX(time.inSeconds(), width, x1, x2);
    }

    /**
     * Converts a screen X coordinate to time position.
     *
     * @param x The X coordinate in pixels
     * @param editViewState The edit view state containing zoom/pan information
     * @param timeLineID The timeline identifier for view data lookup
     * @param width The width of the component in pixels
     * @return The time position
     */
    static tracktion::TimePosition xToTime(int x, EditViewState &editViewState, const juce::String &timeLineID, int width)
    {
        auto x1 = editViewState.getVisibleBeatRange(timeLineID, width).getStart().inBeats();
        auto x2 = editViewState.getVisibleBeatRange(timeLineID, width).getEnd().inBeats();
        auto time = editViewState.xToTime(x, width, x1, x2);
        return tracktion::TimePosition::fromSeconds(time);
    }

    /**
     * Applies snap/grid quantization to a time position.
     *
     * @param time The time position to snap
     * @param editViewState The edit view state containing snap settings
     * @param timeLineID The timeline identifier for view data lookup
     * @param width The width of the component in pixels
     * @param downwards If true, snaps to the previous grid line; otherwise to nearest
     * @return The snapped time position
     */
    static tracktion::TimePosition getSnappedTime(tracktion::TimePosition time, EditViewState &editViewState, const juce::String &timeLineID, int width, bool downwards = false)
    {
        double x1 = editViewState.getVisibleBeatRange(timeLineID, width).getStart().inBeats();
        double x2 = editViewState.getVisibleBeatRange(timeLineID, width).getEnd().inBeats();
        auto snapType = editViewState.getBestSnapType(x1, x2, width);
        auto t = time.inSeconds();
        auto st = editViewState.getSnappedTime(t, snapType, downwards);

        return tracktion::TimePosition::fromSeconds(st);
    }

    /**
     * Checks if snap to grid is enabled.
     *
     * @param editViewState The edit view state
     * @return true if snapping is enabled
     */
    static bool isSnapEnabled(const EditViewState &editViewState) { return editViewState.m_snapToGrid; }

private:
    // Private constructor to prevent instantiation
    TimeUtils() = delete;
    ~TimeUtils() = delete;
    TimeUtils(const TimeUtils &) = delete;
    TimeUtils &operator=(const TimeUtils &) = delete;
};

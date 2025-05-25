
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
#include "Utilities.h"

namespace te = tracktion_engine;


class LassoSelectionTool : public juce::Component
{
public:
    struct LassoRect
    {
        LassoRect ()= default;
        [[maybe_unused]] LassoRect (tracktion::core::TimeRange timeRange, double top, double bottom)
            : m_timeRange(timeRange)
            , m_verticalRange(top, bottom)
            , m_startTime(timeRange.getStart ().inSeconds())
            , m_endTime (timeRange.getEnd ().inSeconds())
            , m_top (top)
            , m_bottom (bottom){}

        juce::Rectangle<int> getRect (EditViewState& evs, double viewX1, double viewX2, int viewWidth) const;
        tracktion::core::TimeRange m_timeRange;
        juce::Range<int> m_verticalRange { 0,0 };

        double m_startTime { 0 };
        double m_endTime { 0 };
        int m_top { 0 };
        int m_bottom { 0 };
    };

    explicit LassoSelectionTool(EditViewState& evs, juce::String timeLineID)
        : m_editViewState(evs)
        , m_timeLineID(timeLineID)

    {}

    void drawLasso(juce::Graphics &g);
    void startLasso(const juce::Point<int> mousePos, int startYScroll, bool isRangeTool);
    void updateLasso(const juce::Point<int> mousePos, int yScroll);
    void stopLasso();
    LassoSelectionTool::LassoRect getLassoRect() const;

private:

    double xToTime(const int x);
    bool                           m_isLassoSelecting {false};
    bool                           m_isRangeSelecting {false};
    EditViewState&                 m_editViewState;
    LassoRect                      m_lassoRect;

    double                         m_clickedTime{};
    int                            m_startYScroll;
    juce::Point<int>               m_startPos{};

    void updateClipCache();
    juce::String m_timeLineID;
    juce::Range<double>
        getVerticalRangeOfTrack(double trackPosY,
                                tracktion_engine::AudioTrack* track) const;


};

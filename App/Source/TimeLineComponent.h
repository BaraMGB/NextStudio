
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"

    
class TimeLineComponent : public juce::Component
{
public:
    TimeLineComponent(EditViewState&
                      , juce::String timeLineID
                    );
    ~TimeLineComponent() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& event) override;

    te::TimecodeSnapType    getBestSnapType();
    EditViewState&          getEditViewState();


    void centerView()
    {
        if (m_evs.viewFollowsPos())
        {
            auto posBeats = m_evs.timeToBeat (
            m_evs.m_edit.getTransport ().getCurrentPosition ());
        
            auto bx1 = getCurrentBeatRange().getStart().inBeats();
            auto bx2 = getCurrentBeatRange().getEnd().inBeats();
            if (posBeats < bx1 || posBeats > bx2)
                m_evs.setNewStartAndZoom(m_timeLineID, posBeats);

            auto zoom = bx2 - bx1;
            m_evs.setNewStartAndZoom(m_timeLineID, juce::jmax(bx1, posBeats - zoom/2));
        }
    }
    tracktion::BeatRange    getCurrentBeatRange()
    {
        auto x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart();
        auto x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getLength();

        return tracktion::BeatRange(x1beats, x2beats);

    }
    double getBeatsPerPixel();

    int                     timeToX (double time);
    int                     beatsToX (double beats);
    tracktion::TimeDuration xToTimeDuration (int x);
    tracktion::TimePosition beatToTime(tracktion::BeatPosition beats);
    tracktion::TimePosition xToTimePos(int x);
    tracktion::BeatPosition xToBeatPos(int x);

    juce::String getTimeLineID() { return m_timeLineID; }
    void setTimeLineID(juce::String timeLineID);

private:

    void                    drawLoopRange(juce::Graphics& g);
    tracktion::TimeRange    getLoopRangeToBeMovedOrResized();
    void                    updateViewRange(const juce::MouseEvent& e);

    juce::Rectangle<int>    getTimeRangeRect(tracktion::TimeRange tr);

    juce::String            m_timeLineID;

    EditViewState &         m_evs;
    juce::ValueTree m_tree;
    double                  m_cachedBeat{};
    bool                    m_isMouseDown,
                            m_isSnapping {true},
                            m_leftResized,
                            m_rightResized,
                            m_changeLoopRange{false},
                            m_loopRangeClicked;

    tracktion::TimeRange    m_cachedLoopRange; 
    tracktion::TimeRange    m_newLoopRange; 
    tracktion::TimeDuration m_draggedTime;
    int                     m_oldDragDistanceY, m_oldDragDistanceX;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};

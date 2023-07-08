
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
                    , juce::CachedValue<double> & x1
                    , juce::CachedValue<double> & x2
                    );
    ~TimeLineComponent() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& event) override;

    te::TimecodeSnapType    getBestSnapType();
    EditViewState&          getEditViewState();

    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;

private:

    void                    drawLoopRange(juce::Graphics& g);
    tracktion::TimeRange    getLoopRangeToBeMovedOrResized();
    void                    updateViewRange(const juce::MouseEvent& e);

    juce::Rectangle<int>    getTimeRangeRect(tracktion::TimeRange tr);

    int                     timeToX (double time);
    int                     beatsToX (double beats);
    tracktion::TimeDuration xToTimeDuration (int x);
    tracktion::TimePosition beatToTime(tracktion::BeatPosition beats);
    tracktion::TimePosition xToTimePos(int x);

    EditViewState &         m_editViewState;
    double                  m_cachedBeat{};
    bool                    m_isMouseDown,
                            m_isSnapping {true},
                            m_leftResized,
                            m_rightResized,
                            m_changeLoopRange{false},
                            m_loopRangeClicked;
    double                  m_cachedX1{}, m_cachedX2{};

    tracktion::TimeRange    m_cachedLoopRange; 
    tracktion::TimeRange    m_newLoopRange; 
    tracktion::TimeDuration m_draggedTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};


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

class TimeLineComponent : public juce::Component
{
public:
    TimeLineComponent(EditViewState &, juce::String timeLineID);
    ~TimeLineComponent() override;

    void paint(juce::Graphics &g) override;

    void mouseMove(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &event) override;

    te::TimecodeSnapType getBestSnapType();
    EditViewState &getEditViewState();

    tracktion::BeatRange getCurrentBeatRange();
    tracktion::TimeRange getCurrentTimeRange();
    double getBeatsPerPixel();

    int timeToX(double time);
    int beatsToX(double beats);
    tracktion::TimeDuration xToTimeDuration(int x);
    tracktion::TimePosition beatToTime(tracktion::BeatPosition beats);
    tracktion::TimePosition xToTimePos(int x);
    tracktion::BeatPosition xToBeatPos(int x);

    juce::String getTimeLineID() { return m_timeLineID; }
    void setTimeLineID(juce::String timeLineID);

    // snapes relative to clip start
    double getQuantisedNoteBeat(double beat, const te::MidiClip *c, bool down = true) const;
    double getQuantisedBeat(double beat, bool down) const;
    te::TimecodeSnapType getBestSnapType() const;
    double getSnappedTime(double time);

private:
    void drawLoopRange(juce::Graphics &g);
    tracktion::TimeRange getLoopRangeToBeMovedOrResized();
    void updateViewRange(const juce::MouseEvent &e);

    juce::Rectangle<int> getTimeRangeRect(tracktion::TimeRange tr);

    juce::String m_timeLineID;

    EditViewState &m_evs;
    juce::ValueTree m_tree;
    double m_cachedBeat{};
    bool m_isMouseDown, m_isSnapping{true}, m_leftResized, m_rightResized, m_changeLoopRange{false}, m_loopRangeClicked;

    tracktion::TimeRange m_cachedLoopRange;
    tracktion::TimeRange m_newLoopRange;
    tracktion::TimeDuration m_draggedTime;
    int m_oldDragDistanceY, m_oldDragDistanceX;
    bool m_cachedFollowPlayhead;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeLineComponent)
};

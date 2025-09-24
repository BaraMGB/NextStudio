
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


/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Steffen Baranowsky

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLineComponent.h"
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"


TimeLineComponent::TimeLineComponent(EditViewState & evs, juce::String timeLineID)
    : m_evs(evs)
    , m_isMouseDown(false)
{
    setTimeLineID(timeLineID);
}

TimeLineComponent::~TimeLineComponent()
{
    if (auto* viewData = m_evs.componentViewData[m_timeLineID])
    {
        delete viewData;
        m_evs.componentViewData.erase (m_timeLineID);
    }
    if (m_tree.getParent().isValid())
        m_tree.getParent().removeChild(m_tree, nullptr);
}

void TimeLineComponent::paint(juce::Graphics& g)
{
    g.setColour(m_evs.m_applicationState.getTimeLineBackGroundColour());
    g.fillAll();
    g.setFont(12);
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

    GUIHelpers::drawBarsAndBeatLines (
                g
              , m_evs
              , x1beats
              , x2beats
              , getLocalBounds().toFloat()
              , true);

    if (m_isMouseDown)
    {
        auto mouseDown = m_evs.beatsToX(m_cachedBeat, getWidth (), x1beats, x2beats) ;
        g.setColour(juce::Colours::white);
        auto rect = juce::Rectangle<int>(mouseDown, 1.f, 1.f, float(getHeight()) - 1.f);
        auto bounds = getLocalBounds();
        if ( bounds.contains(rect))
        {
            g.fillRect(rect);
        }
    }

    if (m_evs.m_edit.getTransport().looping)
        g.setColour(juce::Colour(0x99FFFFFF));
    else
        g.setColour(juce::Colour(0x44FFFFFF));

    drawLoopRange(g);
}

void TimeLineComponent::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);

    auto loopRange = m_evs.m_edit.getTransport().getLoopRange();
    auto loopRangeRect = getTimeRangeRect(loopRange);

    if (loopRangeRect.contains(e.getPosition()))
    {
        m_changeLoopRange = false;
        if (e.x > loopRangeRect.getHorizontalRange().getStart()
            && e.x < loopRangeRect.getHorizontalRange().getStart() + 10)
        {
            setMouseCursor (juce::MouseCursor::LeftEdgeResizeCursor);
            m_leftResized = true;
            m_rightResized = false;
        }
        else if (e.x > loopRangeRect.getHorizontalRange ().getEnd () - 10
                 && e.x < loopRangeRect.getHorizontalRange ().getEnd ())
        {
            setMouseCursor (juce::MouseCursor::RightEdgeResizeCursor);
            m_rightResized = true;
            m_leftResized = false;
        }
        else
        {
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            m_leftResized = false;
            m_rightResized = false;
        }
    }
}

void TimeLineComponent::mouseDown(const juce::MouseEvent& e)
{
    //init
    m_cachedFollowPlayhead = m_evs.m_followPlayhead;
    m_evs.followsPlayhead(false);
    m_changeLoopRange = false;
    m_loopRangeClicked = false;
    m_cachedLoopRange = m_evs.m_edit.getTransport().getLoopRange();
    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;

    auto loopRangeArea = getTimeRangeRect(m_evs.getVisibleTimeRange(m_timeLineID, getWidth()));
    auto loopRange = m_evs.m_edit.getTransport().getLoopRange();

    if (loopRangeArea.contains(e.getPosition()))
        m_changeLoopRange = true;

    if (getTimeRangeRect(loopRange).contains(e.getPosition()))
    {
        m_changeLoopRange = false;
        m_loopRangeClicked = true;
    }
    else if (!m_changeLoopRange)
    {
        double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

        e.source.enableUnboundedMouseMovement(true, false);

        m_isMouseDown = true;
        m_cachedBeat = m_evs.xToBeats(e.getMouseDownPosition().getX(), getWidth (), x1beats, x2beats);

        if (e.getNumberOfClicks() > 1)
        {
            auto newTime = m_evs.beatToTime(m_cachedBeat);
            m_evs.m_playHeadStartTime = newTime;
            m_evs.m_edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(newTime));
        }
    }
}

void TimeLineComponent::mouseDrag(const juce::MouseEvent& e)
{
    m_draggedTime = tracktion::TimeDuration();
    m_isSnapping = !e.mods.isShiftDown();

    if (m_loopRangeClicked)
    {
        auto scroll = beatsToX(0) *-1;

        m_draggedTime = xToTimeDuration(e.getDistanceFromDragStartX() - scroll) ;
        repaint();
    }
    else if (m_changeLoopRange)
    {
        auto& ts = m_evs.m_edit.tempoSequence;
        auto t1 = xToTimePos(e.getMouseDownX());
        auto t2 = xToTimePos(e.x);

        if (m_isSnapping)
        {
            t1 = getBestSnapType().roundTimeDown(t1,ts);
            t2 = getBestSnapType().roundTimeDown(t2,ts);
        }

        if (t1 < t2)
            m_newLoopRange = {t1, t2};
        else
            m_newLoopRange = {t2, t1};

        repaint();
    }
    else
    {
        updateViewRange(e);
    }
}

void TimeLineComponent::mouseUp(const juce::MouseEvent&)
{
    m_evs.followsPlayhead(m_cachedFollowPlayhead);
    auto& t = m_evs.m_edit.getTransport();
    if (m_loopRangeClicked) 
        t.setLoopRange(getLoopRangeToBeMovedOrResized());
    else if (m_changeLoopRange)
        t.setLoopRange(m_newLoopRange);

    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;
    m_draggedTime = tracktion::TimeDuration();  
    m_leftResized= false;
    m_rightResized= false;
    m_loopRangeClicked= false;
    m_isMouseDown = false;
    m_changeLoopRange = false;
    m_newLoopRange = {};
    m_isSnapping = true;

    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}
void TimeLineComponent::updateViewRange(const juce::MouseEvent& e)
{
    const auto sensitivity = 0.03f;

    auto oldViewRange = m_evs.getVisibleBeatRange(m_timeLineID, getWidth());

    //rescale ViewRange at beat on mouse down
    bool isNotToBig = oldViewRange.getLength().inBeats() < 100240.0;
    bool isNotToSmall = oldViewRange.getLength().inBeats() > 0.05;
    auto dragDistanceY = e.getDistanceFromDragStartY();
    auto anchorTime = tracktion::BeatPosition::fromBeats(m_cachedBeat);
    float scaleFactor = 1.0f + ((dragDistanceY > m_oldDragDistanceY) && isNotToBig ? sensitivity 
                             : ((dragDistanceY < m_oldDragDistanceY) && isNotToSmall ? -sensitivity : 0.0f));
    m_oldDragDistanceY = dragDistanceY;
    // m_oldDragDistanceY = scaleFactor == 1.0f ? m_oldDragDistanceY : dragDistanceY;

    auto newViewRange = oldViewRange.rescaled(anchorTime, scaleFactor);

    //move horizontal if mouse dragged on X-Axis
    auto newBeatsPerPixel = newViewRange.getLength().inBeats() / getWidth();
    auto dragDistanceX = m_oldDragDistanceX - e.getDistanceFromDragStartX();
    m_oldDragDistanceX = e.getDistanceFromDragStartX();
    auto moveCorrection = tracktion::BeatDuration::fromBeats(dragDistanceX * newBeatsPerPixel);

    newViewRange = newViewRange.movedToStartAt(newViewRange.getStart() + moveCorrection);

    m_evs.setNewBeatRange(m_timeLineID, newViewRange, getWidth());
}

tracktion::BeatRange TimeLineComponent::getCurrentBeatRange()
{
    auto start = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart();
    auto length = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getLength();

    return tracktion::BeatRange(start, length);

}

tracktion::TimeRange TimeLineComponent::getCurrentTimeRange()
{
    auto start = m_evs.getVisibleTimeRange(m_timeLineID, getWidth()).getStart();
    auto length = m_evs.getVisibleTimeRange(m_timeLineID, getWidth()).getLength();

    return tracktion::TimeRange(start, length);

}


void TimeLineComponent::centerView()
{
    if (m_evs.viewFollowsPos())
    {
        auto posBeats = m_evs.timeToBeat (
            m_evs.m_edit.getTransport ().getPosition ().inSeconds());
        auto bx1 = getCurrentBeatRange().getStart().inBeats();
        auto bx2 = getCurrentBeatRange().getEnd().inBeats();

        auto zoom = bx2 - bx1;
        m_evs.setNewStartAndZoom(m_timeLineID, juce::jmax(0.0 , posBeats - zoom/2));
    }
}

tracktion_engine::TimecodeSnapType TimeLineComponent::getBestSnapType()
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.getBestSnapType (x1beats, x2beats, getWidth());
}

EditViewState &TimeLineComponent::getEditViewState()
{
    return m_evs;
}

int TimeLineComponent::timeToX(double time)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.timeToX(time, getWidth(), x1beats, x2beats);
}

void TimeLineComponent::drawLoopRange(juce::Graphics& g)
{
    juce::Rectangle<int> loopRect;

    if (m_draggedTime != tracktion::TimeDuration())
    {
        loopRect = getTimeRangeRect(getLoopRangeToBeMovedOrResized());
    }
    else if (m_changeLoopRange)
    {
        loopRect = getTimeRangeRect(m_newLoopRange);
    }
    else
    {
        auto loopRange = m_evs.m_edit.getTransport().getLoopRange();
        loopRect = getTimeRangeRect(loopRange);
    }

    loopRect = loopRect.getIntersection(getLocalBounds());

    g.setColour(m_evs.m_applicationState.getPrimeColour().withAlpha(0.5f));
    g.fillRect(loopRect);
}

juce::Rectangle<int> TimeLineComponent::getTimeRangeRect(tracktion::TimeRange tr)
{
    auto x = timeToX (tr.getStart().inSeconds());
    auto w = timeToX (tr.getEnd().inSeconds()) - x;
    auto h = getHeight() / 5;

    return {x, getHeight() - h, w, h};
}

tracktion::TimeRange TimeLineComponent::getLoopRangeToBeMovedOrResized()
{
    auto draggedLoopRange = m_cachedLoopRange;
    auto& t = m_evs.m_edit.tempoSequence;

    if (m_leftResized)
    {
        auto newStart = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getStart() + m_draggedTime, t) 
                                     : draggedLoopRange.getStart() + m_draggedTime;

        if (newStart > draggedLoopRange.getEnd())
            draggedLoopRange = {draggedLoopRange.getEnd(), newStart};
        else
            draggedLoopRange = {newStart, draggedLoopRange.getEnd()};
    }
    else if (m_rightResized)
    {
        auto newEnd = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getEnd() + m_draggedTime, t) 
                                     : draggedLoopRange.getEnd() + m_draggedTime;

        if (newEnd < draggedLoopRange.getStart())
            draggedLoopRange = {newEnd, draggedLoopRange.getStart()};
        else
            draggedLoopRange = {draggedLoopRange.getStart(), newEnd};
    }
    else
    {
        auto newStart = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getStart() + m_draggedTime, t) 
                                     : draggedLoopRange.getStart() + m_draggedTime;
        newStart = juce::jmax(tracktion::TimePosition::fromSeconds(0.0), newStart);

        draggedLoopRange = draggedLoopRange.movedToStartAt(newStart);
    }

    return draggedLoopRange;
}

tracktion::TimeDuration TimeLineComponent::xToTimeDuration(int x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimeDuration::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}

tracktion::TimePosition TimeLineComponent::beatToTime(tracktion::BeatPosition beats)
{
    return tracktion::TimePosition::fromSeconds(m_evs.beatToTime(beats.inBeats()));
}

int TimeLineComponent::beatsToX(double beats)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.beatsToX(beats, getWidth(), x1beats, x2beats);
}

tracktion::TimePosition TimeLineComponent::xToTimePos(int x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimePosition::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}
tracktion::BeatPosition TimeLineComponent::xToBeatPos(int x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    double beats = m_evs.xToBeats(x, getWidth(), x1beats, x2beats);
    return tracktion::BeatPosition::fromBeats(beats);
}

double TimeLineComponent::getBeatsPerPixel()
{
    auto visibleBeatRange = m_evs.getVisibleBeatRange(m_timeLineID, getWidth());
    double beatLength = visibleBeatRange.getLength().inBeats();
    int componentWidth = getWidth();

    if (componentWidth <= 0)
        return 0.0;

    return beatLength / componentWidth;
}
void TimeLineComponent::setTimeLineID(juce::String timeLineID)
{
    if (m_timeLineID.isNotEmpty())
    {
        if (auto* viewData = m_evs.componentViewData[m_timeLineID])
        {
            delete viewData;
            m_evs.componentViewData.erase (m_timeLineID);
        }
        if (m_tree.getParent().isValid())
            m_tree.getParent().removeChild(m_tree, nullptr);
    }

    m_timeLineID = timeLineID;
    m_tree = m_evs.m_viewDataTree.getOrCreateChildWithName(timeLineID, nullptr);
    m_evs.componentViewData[m_timeLineID] = new ViewData(m_tree);
}

double TimeLineComponent::getQuantisedNoteBeat(double beat,const te::MidiClip* c, bool down) const
{
    auto editBeat = c->getStartBeat().inBeats() + beat;

    return getQuantisedBeat(editBeat, down) - c->getStartBeat().inBeats();
}

double TimeLineComponent::getQuantisedBeat(double beat, bool down) const
{
    auto snapType = getBestSnapType();
    auto time = m_evs.beatToTime(beat);
    auto snapedTime = m_evs.getSnapedTime(time, snapType, down);
    auto quantisedBeat = m_evs.timeToBeat (snapedTime);

    return quantisedBeat;
}

te::TimecodeSnapType TimeLineComponent::getBestSnapType() const
{
    auto x1 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    auto x2 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

    return m_evs.getBestSnapType(x1, x2, getWidth());
}

double TimeLineComponent::getSnapedTime(double time)
{
    return m_evs.getSnapedTime(time, getBestSnapType(), false);
}

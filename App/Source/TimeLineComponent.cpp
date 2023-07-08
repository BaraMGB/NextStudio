
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


TimeLineComponent::TimeLineComponent(EditViewState & evs, juce::CachedValue<double> &x1, juce::CachedValue<double> &x2)
    : m_editViewState(evs)
    , m_X1(x1)
    , m_X2(x2)
    , m_isMouseDown(false)
{
}

TimeLineComponent::~TimeLineComponent()
= default;

void TimeLineComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff555555));
    g.setFont(12);

    g.setColour(juce::Colour(0xffffffff));
    double x1beats = m_X1;
    double x2beats = m_X2;

    GUIHelpers::drawBarsAndBeatLines (
                g
              , m_editViewState
              , x1beats
              , x2beats
              , getLocalBounds()
              , true);

    if (m_isMouseDown)
    {
        auto md = m_editViewState.beatsToX(m_cachedBeat, getWidth (), m_X1, m_X2) ;
        g.setColour(juce::Colours::white);
        g.fillRect(md, 1, 1, getHeight()-1);
    }

    if (m_editViewState.m_edit.getTransport().looping)
        g.setColour(juce::Colour(0x99FFFFFF));
    else
        g.setColour(juce::Colour(0x44FFFFFF));

    drawLoopRange(g);
}

void TimeLineComponent::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);

    auto loopRange = m_editViewState.m_edit.getTransport().getLoopRange();
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
    m_editViewState.followsPlayhead(false);
    m_changeLoopRange = false;
    m_cachedLoopRange = m_editViewState.m_edit.getTransport().getLoopRange();
        
    auto loopRangeArea = getTimeRangeRect(m_editViewState.getSongEditorVisibleTimeRange());
    auto loopRange = m_editViewState.m_edit.getTransport().getLoopRange();

    if (loopRangeArea.contains(e.getPosition()))
        m_changeLoopRange = true;

    if (getTimeRangeRect(loopRange).contains(e.getPosition()))
    {
        m_changeLoopRange = false;
        m_loopRangeClicked = true;
    }
    else if (!m_changeLoopRange)
    {
        e.source.enableUnboundedMouseMovement(true, false);
        m_isMouseDown = true;
        m_cachedBeat = m_editViewState.xToBeats(
            e.getMouseDownPosition().getX(), getWidth (), m_X1, m_X2);
        m_cachedX1 = m_X1;
        m_cachedX2 = m_X2;
        if (e.getNumberOfClicks() > 1)
        {
            auto newTime = m_editViewState.beatToTime(m_cachedBeat);
            m_editViewState.m_playHeadStartTime = newTime;
            m_editViewState.m_edit.getTransport().setCurrentPosition(newTime);
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
        auto& ts = m_editViewState.m_edit.tempoSequence;
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
    m_editViewState.followsPlayhead(true);
    auto& t = m_editViewState.m_edit.getTransport();
    if (m_loopRangeClicked) 
        t.setLoopRange(getLoopRangeToBeMovedOrResized());
    else if (m_changeLoopRange)
        t.setLoopRange(m_newLoopRange);

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
    auto scaleFactor = GUIHelpers::getZoomScaleFactor(
        e.getDistanceFromDragStartY(), m_editViewState.getTimeLineZoomUnit());

    auto newVisibleLengthBeats = juce::jlimit(
        0.05, 240.0, (m_cachedX2 - m_cachedX1) * scaleFactor);

    auto currentXinBeats = juce::jmap(
        (double) e.x, 0.0,(double) getWidth(), 0.0, newVisibleLengthBeats);

    auto newRangeStartBeats = juce::jlimit(
        0.0,
        m_editViewState.getEndScrollBeat() - newVisibleLengthBeats,
        m_cachedBeat - currentXinBeats);

    m_X1 = newRangeStartBeats;
    m_X2 = newRangeStartBeats + newVisibleLengthBeats;
}

tracktion_engine::TimecodeSnapType TimeLineComponent::getBestSnapType()
{
    return m_editViewState.getBestSnapType (m_X1, m_X2, getWidth());
}

EditViewState &TimeLineComponent::getEditViewState()
{
    return m_editViewState;
}

int TimeLineComponent::timeToX(double time)
{
    return m_editViewState.timeToX(time, getWidth(), m_X1, m_X2);
}

void TimeLineComponent::drawLoopRange(juce::Graphics& g)
{
    if (m_draggedTime != tracktion::TimeDuration())
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour().withAlpha(0.5f));
        g.fillRect(getTimeRangeRect(getLoopRangeToBeMovedOrResized()));
    }
    else if (m_changeLoopRange)
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour().withAlpha(0.5f));
        g.fillRect(getTimeRangeRect(m_newLoopRange));
    }
    else
    {
        auto loopRange = m_editViewState.m_edit.getTransport().getLoopRange();
        g.fillRect(getTimeRangeRect(loopRange));
    }        
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
    auto& t = m_editViewState.m_edit.tempoSequence;

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
    return tracktion::TimeDuration::fromSeconds(m_editViewState.xToTime(x, getWidth(), m_X1, m_X2));
}

tracktion::TimePosition TimeLineComponent::beatToTime(tracktion::BeatPosition beats)
{
    return tracktion::TimePosition::fromSeconds(m_editViewState.beatToTime(beats.inBeats()));
}

int TimeLineComponent::beatsToX(double beats)
{
    return m_editViewState.beatsToX(beats, getWidth(), m_X1, m_X2);
}

tracktion::TimePosition TimeLineComponent::xToTimePos(int x)
{
    return tracktion::TimePosition::fromSeconds(m_editViewState.xToTime(x, getWidth(), m_X1, m_X2));
}

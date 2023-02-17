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
        g.setColour(juce::Colour(0xffFFFFFF));
    else
        g.setColour(juce::Colour(0x22FFFFFF));
    drawLoopRange(g);
}

void TimeLineComponent::mouseMove(const juce::MouseEvent& e)
{
    auto loopRangeRect = getLoopRangeRect();
    auto loopRangeArea = juce::Rectangle<int>(0, getHeight() - loopRangeRect.getHeight(), getWidth(), loopRangeRect.getHeight());

    if (loopRangeArea.contains(e.getPosition()))
        m_changeLoopRange = true;
    else
        m_changeLoopRange = false;

    if (loopRangeRect.contains(e.getPosition()))
    {
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
    if (getLoopRangeRect().contains(e.getPosition()))
    {
        m_loopRangeClicked = true;
        m_cachedLoopRange = {m_editViewState.m_edit.getTransport().loopPoint1,
                             m_editViewState.m_edit.getTransport().loopPoint2};
    }
    else
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
    m_draggedTime = {};
    m_isSnapping = !e.mods.isShiftDown();

    if (m_loopRangeClicked)
    {
        auto scroll = beatsToX(0) *-1;

        m_draggedTime = xToTimeDuration(e.getDistanceFromDragStartX() - scroll) ;
        repaint();
    }
    else
    {
        updateViewRange(e);
    }
}

void TimeLineComponent::mouseUp(const juce::MouseEvent&)
{
    if (m_loopRangeClicked)
        moveOrResizeLoopRange();

    m_draggedTime = {};  
    m_leftResized= false;
    m_rightResized= false;
    m_loopRangeClicked= false;
    m_isMouseDown = false;
    m_changeLoopRange = false;
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

void TimeLineComponent::moveOrResizeLoopRange()
{
    auto& t = m_editViewState.m_edit.getTransport();

    if (t.getLoopRange ().getLength ().inSeconds() > 0)
        t.setLoopRange(getLoopRangeToBeMovedOrResized());
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
    g.fillRect(getLoopRangeRect());
    if (m_draggedTime != tracktion::TimeDuration())
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour().withAlpha(0.5f));
        g.fillRect(getTimeRangeRect(getLoopRangeToBeMovedOrResized()));
    }
}

juce::Rectangle<int> TimeLineComponent::getTimeRangeRect(tracktion::TimeRange tr)
{
    auto x = timeToX (tr.getStart().inSeconds());
    auto w = timeToX (tr.getEnd().inSeconds()) - x;  
    auto h = 5;

    return {x, getHeight() - h, w, h};
}

juce::Rectangle<int> TimeLineComponent::getLoopRangeRect()
{
    auto& t = m_editViewState.m_edit.getTransport();

    auto start = timeToX(t.getLoopRange().getStart().inSeconds());
    auto end = timeToX(t.getLoopRange().getEnd().inSeconds());
    auto h = 5;

    return {start, getHeight() - h, end - start, h};
}

tracktion::TimeRange TimeLineComponent::getLoopRangeToBeMovedOrResized()
{
    auto draggedLoopRange = m_cachedLoopRange;
    auto& t = m_editViewState.m_edit.tempoSequence;

    if (m_leftResized)
    {
        auto newStart = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getStart() + m_draggedTime, t) 
                                     : draggedLoopRange.getStart() + m_draggedTime;

        draggedLoopRange = draggedLoopRange.withStart(newStart);
    }
    else if (m_rightResized)
    {
        auto newEnd = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getEnd() + m_draggedTime, t) 
                                     : draggedLoopRange.getEnd() + m_draggedTime;
        draggedLoopRange = draggedLoopRange.withEnd(newEnd);
    }
    else
    {
        auto newStart = m_isSnapping ? getBestSnapType().roundTimeDown(draggedLoopRange.getStart() + m_draggedTime, t) 
                                     : draggedLoopRange.getStart() + m_draggedTime;

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

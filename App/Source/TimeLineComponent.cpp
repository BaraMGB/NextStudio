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


TimeLineComponent::TimeLineComponent(EditViewState & evs, juce::CachedValue<double> &x1, juce::CachedValue<double> &x2)
    : m_editViewState(evs)
    , m_isMouseDown(false)
    , m_X1(x1)
    , m_X2(x2)
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
              , getBounds ()
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
    auto llr = getLoopRangeRect();

    if (llr.contains(e.getPosition()))
    {
        if (e.x > llr.getHorizontalRange().getStart()
            && e.x < llr.getHorizontalRange().getStart() + 10)
        {
            setMouseCursor (juce::MouseCursor::LeftEdgeResizeCursor);
            m_leftResized = true;
            m_rightResized = false;
        }
        else if (e.x > llr.getHorizontalRange ().getEnd () - 10
                 && e.x < llr.getHorizontalRange ().getEnd ())
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
        m_cachedL1 = m_editViewState.m_edit.getTransport().loopPoint1->inSeconds();
        m_cachedL2 = m_editViewState.m_edit.getTransport().loopPoint2->inSeconds();
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
    if (m_loopRangeClicked)
        moveLoopRange(e);
    else
        updateViewRange(e);
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
void TimeLineComponent::moveLoopRange(const juce::MouseEvent& e)
{
    auto& t = m_editViewState.m_edit.getTransport();

    if (t.getLoopRange ().getLength ().inSeconds() > 0)
    {
        if (m_leftResized)
            t.loopPoint1 = EngineHelpers::getTimePos(getMovedTime(e, m_cachedL1));
        else if (m_rightResized)
            t.loopPoint2 = EngineHelpers::getTimePos(getMovedTime(e, m_cachedL2));
        else
            t.setLoopRange({EngineHelpers::getTimePos(getMovedTime(e, m_cachedL1)),
                            EngineHelpers::getTimePos(getMovedTime(e, m_cachedL1)) + t.getLoopRange().getLength()});
    }
}
double TimeLineComponent::getMovedTime(const juce::MouseEvent& e, double oldTime)
{
    auto scroll = beatToTime(m_X1);
    auto offset = xToTime(e.getDistanceFromDragStartX());
    auto movedTime = oldTime + offset - scroll;
    auto snaped = getBestSnapType().roundTimeDown (
       EngineHelpers::getTimePos(movedTime),
        m_editViewState.m_edit.tempoSequence).inSeconds();
    movedTime = e.mods.isShiftDown () ? movedTime : snaped;

    return movedTime;
}
void TimeLineComponent::mouseUp(const juce::MouseEvent&)
{
    m_leftResized= false;
    m_rightResized= false;
    m_loopRangeClicked= false;
    m_isMouseDown = false;

    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
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
    if (getLoopRangeRect().getWidth() == 0)
        return;

    g.fillRect(getLoopRangeRect());
}
juce::Rectangle<int> TimeLineComponent::getLoopRangeRect()
{
    auto& t = m_editViewState.m_edit.getTransport();

    auto start = timeToX(t.getLoopRange().getStart().inSeconds());
    auto end = timeToX(t.getLoopRange().getEnd().inSeconds());
    auto h = 5;

    return {start, getHeight() - h, end - start, h};
}
double TimeLineComponent::xToTime(int x)
{
    return m_editViewState.xToTime(x, getWidth(), m_X1, m_X2);
}
double TimeLineComponent::beatToTime(double beats)
{
    return m_editViewState.beatToTime(beats);
}
int TimeLineComponent::beatsToX(double beats)
{
    return m_editViewState.beatsToX(beats, getWidth(), m_X1, m_X2);
}

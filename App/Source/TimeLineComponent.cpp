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

}

void TimeLineComponent::mouseDown(const juce::MouseEvent& event)
{
    event.source.enableUnboundedMouseMovement(true, false);
    m_isMouseDown = true;
    m_cachedBeat = m_editViewState.xToBeats(event.getMouseDownPosition().getX(), getWidth (), m_X1, m_X2);
    m_cachedX1 = m_X1;
    m_cachedX2 = m_X2;
    if (event.getNumberOfClicks() > 1)
    {
        auto newTime = m_editViewState.beatToTime(m_cachedBeat);
        m_editViewState.m_playHeadStartTime = newTime;
        m_editViewState.m_edit.getTransport().setCurrentPosition(newTime);
    }
}

void TimeLineComponent::mouseDrag(const juce::MouseEvent& event)
{
    auto scaleFactor = GUIHelpers::getZoomScaleFactor(
        event.getDistanceFromDragStartY(), m_editViewState.getTimeLineZoomUnit());
    auto newVisibleLengthBeats = juce::jlimit(
        0.05, 240.0
        , (m_cachedX2 - m_cachedX1) * scaleFactor);

    auto currentXinBeats = juce::jmap(
        (double)event.x
        , 0.0,(double) getWidth()
        , 0.0, newVisibleLengthBeats);

    auto newRangeStartBeats = juce::jlimit(
        0.0
        , m_editViewState.getEndScrollBeat() - newVisibleLengthBeats
        , m_cachedBeat - currentXinBeats);

    m_X1 = newRangeStartBeats;
    m_X2 = newRangeStartBeats + newVisibleLengthBeats;
}

void TimeLineComponent::mouseUp(const juce::MouseEvent&)
{
    m_isMouseDown = false;
    repaint();
}

tracktion_engine::TimecodeSnapType TimeLineComponent::getBestSnapType()
{
    return m_editViewState.getBestSnapType (
                m_X1
              , m_X2
              , getWidth());
}

EditViewState &TimeLineComponent::getEditViewState()
{
    return m_editViewState;
}


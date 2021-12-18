/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

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
{
}

void TimeLineComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xff555555));
    //g.fillRect (0, getHeight () - 1, getWidth(), 1);
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
    // Work out the scale of the new range
    auto unitDistance = 50.0f;
    auto scaleFactor
            = 1.0 / std::pow (
                2, (float) event.getDistanceFromDragStartY() / unitDistance);

    // Now position it so that the mouse continues to point at the same
    // place on the ruler.
    auto visibleLength = std::min(240.0,
                                  std::max (0.05,
                                            (m_cachedX2 - m_cachedX1) / scaleFactor));
    auto rangeBegin =  std::max (
                0.0
              , m_cachedBeat - visibleLength * (event.x)
                / getWidth());
    rangeBegin = juce::jmin(m_editViewState.getEndScrollBeat () - visibleLength, rangeBegin);
    m_X1 = rangeBegin;
    m_X2 = rangeBegin + visibleLength;
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


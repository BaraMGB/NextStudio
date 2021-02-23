/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLineComponent.h"

TimeLineComponent::TimeLineComponent(EditViewState & evs, juce::CachedValue<double> &x1, juce::CachedValue<double> &x2)
    : m_state(evs)
    , m_mouseDown(false)
    , m_X1(x1)
    , m_X2(x2)
{
}

TimeLineComponent::~TimeLineComponent()
{
}

void TimeLineComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff1b1b1b));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xff555555));
    g.drawRect(0, getHeight () - 1, getWidth (), 1);
    g.setFont(12);

    g.setColour(juce::Colour(0xffffffff));
    double x1 = m_X1;
    double x2 = m_X2;
    double zoom = x2 - x1;
    int firstBeat = static_cast<int>(x1);
    if(beatsToX(firstBeat) < 0)
    {
        firstBeat++;
    }

    auto pixelPerBeat = getWidth() / zoom;
    for (int beat = firstBeat - 1; beat <= x2; beat++)
    {
        const int BeatX = beatsToX(beat) + 1;

        auto zBars = 16;

        if (zoom < 240)
        {
            zBars /= 2;
        }
        if (zoom < 120)
        {
            zBars /=2;
        }
        if (beat % zBars == 0)
        {
            g.drawLine(BeatX, getHeight()/ 3, BeatX, getHeight());
            g.drawSingleLineText(juce::String((beat/4)+1)
                                 ,BeatX + 3
                                 ,getHeight()/3  + g.getCurrentFont().getHeight());
        }
        if (zoom < 60)
        {
            //g.setColour(juce::Colours::white.darker(0.5f));
            g.drawLine(BeatX,getHeight() - getHeight()/ 3, BeatX, getHeight());
            g.setColour(juce::Colours::white);
        }
        if (zoom < 25)
        {
            //g.setColour(juce::Colours::white.darker(0.5f));
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5) {
                 g.drawLine(BeatX + quarterBeat * i ,(getHeight() - getHeight()/3) + getHeight()/10,
                 BeatX + quarterBeat * i ,getHeight());
                 i++;
            }
            g.setColour(juce::Colours::white);
        }
        if (zoom < 12)
        {
//            auto quarterBeat = pixelPerBeat / 4;
//            auto i = 1;
//            while ( i < 5) {
//                g.drawSingleLineText(String((beat/4)+1)
//                                     ,BeatX + (pixelPerBeat * i)
//                                     ,getHeight()/3  + g.getCurrentFont().getHeight());
//                i++;
//            }
        }
    }

    if (m_mouseDown)
    {
        auto md = beatsToX(m_BeatAtMouseDown);
        g.setColour(juce::Colours::white.darker(0.9f));
        g.fillRect(md-1, 1, 1, getHeight()-1);
        g.setColour(juce::Colours::white);
        g.fillRect(md,0,1,getHeight());
        g.setColour(juce::Colours::white.darker(0.9f));
        g.fillRect(md+1, 1, 1, getHeight()-1);
        g.setColour(juce::Colours::white);
    }

}

void TimeLineComponent::mouseDown(const juce::MouseEvent& event)
{
    event.source.enableUnboundedMouseMovement(true, false);
    m_mouseDown = true;
    m_BeatAtMouseDown = xToBeats(event.getMouseDownPosition().getX());
    m_x1atMD = m_X1;
    m_x2atMD = m_X2;
    m_oldDragDistX = 0;
    m_oldDragDistY = 0;
    if (event.getNumberOfClicks() > 1)
    {
        m_state.m_edit.getTransport().setCurrentPosition(m_state.beatToTime(m_BeatAtMouseDown));
    }
}

void TimeLineComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Work out the scale of the new range
    auto unitDistance = 50.0f;
    auto scaleFactor  = 1.0 / std::pow (2, (float) event.getDistanceFromDragStartY() / unitDistance);

    // Now position it so that the mouse continues to point at the same
    // place on the ruler.
    auto visibleLength = std::min(480.0,
                                  std::max (0.12,
                                            (m_x2atMD - m_x1atMD) / scaleFactor));
    auto rangeBegin =  std::max (0.0,  m_BeatAtMouseDown - visibleLength * event.x / getWidth());
    rangeBegin = juce::jmin(m_state.getEndScrollBeat () - visibleLength, rangeBegin);
    m_X1 = rangeBegin;
    m_X2 = rangeBegin + visibleLength;
}

void TimeLineComponent::mouseUp(const juce::MouseEvent&)
{
    m_mouseDown = false;
    repaint();
}

int TimeLineComponent::beatsToX(double beats)
{
    return juce::roundToInt (((beats - m_X1) *  getWidth())
                             / (m_X2 - m_X1));
}

double TimeLineComponent::xToBeats(int x)
{
    return (double (x) / getWidth()) * (m_X2 - m_X1) + m_X1;
}


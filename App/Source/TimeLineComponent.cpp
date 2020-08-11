/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLineComponent.h"

TimeLineComponent::TimeLineComponent(EditViewState& state)
    : m_mouseDown(false)
    , m_state(state)
{
}

TimeLineComponent::~TimeLineComponent()
{
}

void TimeLineComponent::paint(Graphics& g)
{
    g.setColour(Colour(0xff181818));
    g.fillRect(getLocalBounds());
    g.setColour(Colours::white);
    g.drawRect(getLocalBounds());
    g.setFont(12);

    double x1 = m_state.viewX1;
    double x2 = m_state.viewX2;
    double zoom = x2 -x1;
    int firstBeat = static_cast<int>(x1);
    if(m_state.beatsToX(firstBeat,getWidth()) < 0)
    {
        firstBeat++;
    }

    auto pixelPerBeat = getWidth() / zoom;
    //std::cout << zoom << std::endl;
    for (int beat = firstBeat - 1; beat <= m_state.viewX2; beat++)
    {
        int BeatX = m_state.beatsToX(beat, getWidth());

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
            g.drawSingleLineText(String((beat/4)+1)
                                 ,BeatX + 3
                                 ,getHeight()/3  + g.getCurrentFont().getHeight());
        }
        if (zoom < 60)
        {
            g.setColour(Colours::white.darker(0.5f));
            g.drawLine(BeatX,getHeight() - getHeight()/ 3, BeatX, getHeight());
            g.setColour(Colours::white);
        }
        if (zoom < 25)
        {
            g.setColour(Colours::white.darker(0.5f));
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5) {
                 g.drawLine(BeatX + quarterBeat * i ,(getHeight() - getHeight()/3) + getHeight()/10,
                 BeatX + quarterBeat * i ,getHeight());
                 i++;
            }
            g.setColour(Colours::white);
        }
//        if (zoom < 12)
//        {
////            auto quarterBeat = pixelPerBeat / 4;
////            auto i = 1;
////            while ( i < 5) {
////                g.drawSingleLineText(String((beat/4)+1)
////                                     ,BeatX + (pixelPerBeat * i)
////                                     ,getHeight()/3  + g.getCurrentFont().getHeight());
////                i++;
////            }
//        }
    }

    if (m_mouseDown)
    {
        auto md = m_state.beatsToX(m_BeatAtMouseDown, getWidth());
        g.setColour(Colours::white.darker(0.9f));
        g.fillRect(md-1, 1, 1, getHeight()-1);
        g.setColour(Colours::white);
        g.fillRect(md,0,1,getHeight());
        g.setColour(Colours::white.darker(0.9f));
        g.fillRect(md+1, 1, 1, getHeight()-1);
        g.setColour(Colours::white);
    }

}

void TimeLineComponent::mouseDown(const MouseEvent& event)
{
    event.source.enableUnboundedMouseMovement(true, false);
    m_mouseDown = true;
    m_BeatAtMouseDown = m_state.xToBeats(event.getMouseDownPosition().getX(), getWidth());
    m_x1atMD = m_state.viewX1;
    m_x2atMD = m_state.viewX2;
    m_oldDragDistX = 0;
    m_oldDragDistY = 0;
    if (event.getNumberOfClicks() > 1)
    {
        m_state.edit.getTransport().setCurrentPosition(m_state.beatToTime(m_BeatAtMouseDown));
    }
}

void TimeLineComponent::mouseDrag(const MouseEvent& event)
{
    // Work out the scale of the new range
    auto unitDistance = 100.0f;
    auto scaleFactor  = 1.0 / std::pow (2, (float) event.getDistanceFromDragStartY() / unitDistance);

    // Now position it so that the mouse continues to point at the same
    // place on the ruler.
    auto visibleLength = std::min(480.0,
                                  std::max (0.12,
                                            (m_x2atMD - m_x1atMD) / scaleFactor));
    auto rangeBegin = std::max (0.0,  m_BeatAtMouseDown - visibleLength * event.x / getWidth());

    m_state.viewX1 = rangeBegin;
    m_state.viewX2 = rangeBegin + visibleLength;
}

void TimeLineComponent::mouseUp(const MouseEvent&)
{
    m_mouseDown = false;
    repaint();
}


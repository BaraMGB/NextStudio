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
    g.setFont(15);

    double x1 = m_state.viewX1;
    double x2 = m_state.viewX2;

    int firstBeat = static_cast<int>(x1);
    if(m_state.beatsToX(firstBeat,getWidth()) < 0)
    {
        firstBeat++;
    }


    for (int beat = firstBeat; beat <= m_state.viewX2; beat++)
    {
        int BeatX = m_state.beatsToX(beat,getWidth());
        if(x2 - x1 < 35*4)
        {
            g.drawLine(BeatX, getHeight()/ 2, BeatX, getHeight());
        }
        if (beat%16 == 0)
        {
            g.drawLine(BeatX, getHeight()/ 2, BeatX, getHeight());
            g.drawSingleLineText(String((beat/4)+1),BeatX,g.getCurrentFont().getHeight());
        }
    }

    if (m_mouseDown)
    {
        g.setColour(Colours::yellow);
        g.drawLine(m_state.beatsToX(m_BeatAtMouseDown, getWidth())
                   , 0
                   , m_state.beatsToX(m_BeatAtMouseDown
                                      , getWidth())
                   , getHeight() );
        g.setColour(Colours::white);
    }

}

void TimeLineComponent::mouseDown(const MouseEvent& event)
{
    m_mouseDown = true;

    m_BeatAtMouseDown = m_state.xToBeats(event.getMouseDownPosition().getX(), getWidth());
    m_oldDragDistX = 0;
    m_oldDragDistY = 0;
}

void TimeLineComponent::mouseDrag(const MouseEvent& event)
{
    event.source.enableUnboundedMouseMovement(true);
    double dragDistY = event.getDistanceFromDragStartY();
    double dragDistX = event.getDistanceFromDragStartX();


    auto accelY = (dragDistY - m_oldDragDistY) / 10;


    //Zoom
    m_state.viewX2 = m_state.viewX2 + accelY;

    //correct Viewport

    double zoomOffsetBeats = m_state.xToBeats(event.getMouseDownPosition().getX()
                                              + dragDistX, getWidth())
            - m_BeatAtMouseDown;
    std::cout << zoomOffsetBeats << std::endl;

    if(m_state.viewX1 - zoomOffsetBeats >= 0)
    {
        m_state.viewX1 = m_state.viewX1 - zoomOffsetBeats ;
        m_state.viewX2 = m_state.viewX2 - zoomOffsetBeats;
    }

    //Move
    auto accelX = m_state.xToBeats(dragDistX, getWidth()) - m_state.xToBeats(m_oldDragDistX, getWidth());
    if( m_state.viewX1 - accelX >= 0)
    {
        m_state.viewX1 = m_state.viewX1 - accelX;
        m_state.viewX2 = m_state.viewX2 - accelX;
    }

    m_oldDragDistX = dragDistX;
    m_oldDragDistY = dragDistY;

    repaint();
}

void TimeLineComponent::mouseUp(const MouseEvent& event)
{
    Desktop::setMousePosition(Point<int>( getX() + m_state.beatsToX( m_BeatAtMouseDown, getWidth()) ,
                                          event.getScreenY()));
    m_mouseDown = false;
    repaint();
}


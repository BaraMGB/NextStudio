/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLineComponent.h"

TimeLineComponent::TimeLineComponent(SongEditorViewState& state, Viewport& pos)
    : m_screenX(0)
    , m_screenW(0)
    , m_state(state)
    , m_viewPort(pos)
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

    auto beatLine = 0, barline = 0, barNum = 0;
    while (beatLine < getWidth())
    {
        int pixelPerBeat = jmax(3, static_cast<int>(m_state.m_pixelPerBeat));
        barline++;
        if (barline == 1)
        {
            barNum++;
            if (pixelPerBeat > 70)
            {
                for (auto beat = 1; beat < 5; beat++)
                {
                    g.drawSingleLineText(
                        String(barNum) + "." + String(5 - beat)
                        , (barNum * pixelPerBeat * 4) - (pixelPerBeat * beat) + 3
                        , g.getCurrentFont().getHeight()
                    );
                }
            }
            else if (pixelPerBeat > 3)
            {
                g.drawSingleLineText(
                    String(barNum)
                    , (barNum * pixelPerBeat * 4) - (pixelPerBeat * 4) + 3
                    , g.getCurrentFont().getHeight()
                );
            }
            else
            {
                if (barNum % 4 == 1)
                {
                    g.drawSingleLineText(
                        String(barNum)
                        , (barNum * pixelPerBeat * 4) - (pixelPerBeat * 4) + 3
                        , g.getCurrentFont().getHeight()
                    );
                }
            }
        }
        auto lineStartY = g.getCurrentFont().getHeight();
        if (barline == 4)
        {
            lineStartY = 0;
            barline = 0;
        }
        beatLine = beatLine + pixelPerBeat;
        if (pixelPerBeat > 8.0 || barline == 0)
        {
            
            if (pixelPerBeat > 3 || barNum % 4 == 0)
            {
                g.drawLine(
                    beatLine
                    , lineStartY
                    , beatLine
                    , getHeight()
                );
            }
        }
        if (pixelPerBeat > 100)
        {
            for (auto i = 0; i < 4; i++)
            {
                auto pos = (beatLine - pixelPerBeat) + (i * pixelPerBeat / 4);
                g.drawLine(
                    pos
                    , g.getCurrentFont().getHeight()
                    , pos
                    , getHeight()
                );
            }
        }
    }
    
}

void TimeLineComponent::mouseDown(const MouseEvent& event)
{
    m_posAtMouseDown = event.getMouseDownScreenPosition();
}

void TimeLineComponent::mouseDrag(const MouseEvent& event)
{
    setMouseCursor(MouseCursor::NoCursor);
    
    auto distanceX = 0;
    if (event.getDistanceFromDragStartX() > 5 || event.getDistanceFromDragStartX() < -5)
    {
        //move
        distanceX = event.getDistanceFromDragStartX() * 2;
        m_viewPort.setViewPosition(
            m_viewPort.getViewPosition().getX()
            - event.getDistanceFromDragStartX()
            , m_viewPort.getViewPosition().getY()
        );
    }
    else
    {
        //zoom
        double clickPos = event.getMouseDownPosition().getX() / m_state.m_pixelPerBeat;

        m_state.m_pixelPerBeat = jmax(3.0, m_state.m_pixelPerBeat - (event.getDistanceFromDragStartY()));

        double currentPos = event.getMouseDownPosition().getX() / m_state.m_pixelPerBeat;
        double newPos = (currentPos - clickPos) * m_state.m_pixelPerBeat;

        m_viewPort.setViewPosition(
            m_viewPort.getViewPositionX()
            - newPos
            , m_viewPort.getViewPositionY()
        );
    }
    
    

    m_distanceX = m_distanceX - distanceX;
    Desktop::setMousePosition(m_posAtMouseDown);
    sendChangeMessage();
}

void TimeLineComponent::mouseUp(const MouseEvent& /*event*/)
{
    auto distanceX = m_posAtMouseDown.getX() - m_distanceX < m_screenX ? m_screenX : m_posAtMouseDown.getX() - m_distanceX;
    distanceX = m_posAtMouseDown.getX() - m_distanceX > m_screenW ? m_screenW : distanceX;
    Desktop::setMousePosition(Point<int>(distanceX, m_posAtMouseDown.getY()));
    m_distanceX = 0;
    setMouseCursor(MouseCursor::NormalCursor);
}

void TimeLineComponent::setScreen(int screenPosX, int screenWidth)
{
    m_screenX = screenPosX;
    m_screenW = screenWidth;
}

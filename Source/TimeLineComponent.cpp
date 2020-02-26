/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLineComponent.h"

TimeLineComponent::TimeLineComponent(int& pixelPerBeat, Viewport& pos)
    : m_pixelPerBeat(pixelPerBeat)
    , m_viewPort(pos)
    , m_screenX(0)
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
    auto area = getLocalBounds();
    auto beatLine = 0, barline = 0, barNum = 0;
    while (beatLine < getWidth())
    {
        barline++;
        if (barline == 1)
        {
            barNum++;
            if (m_pixelPerBeat > 70)
            {
                for (auto beat = 1; beat < 5; beat++)
                {
                    g.drawSingleLineText(
                        String(barNum) + "." + String(5 - beat)
                        , (barNum * m_pixelPerBeat * 4) - (m_pixelPerBeat * beat) + 3
                        , g.getCurrentFont().getHeight()
                    );
                }
            }
            else if (m_pixelPerBeat > 3)
            {
                g.drawSingleLineText(
                    String(barNum)
                    , (barNum * m_pixelPerBeat * 4) - (m_pixelPerBeat * 4) + 3
                    , g.getCurrentFont().getHeight()
                );
            }
            else
            {
                if (barNum % 4 == 1)
                {
                    g.drawSingleLineText(
                        String(barNum)
                        , (barNum * m_pixelPerBeat * 4) - (m_pixelPerBeat * 4) + 3
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
        beatLine = beatLine + m_pixelPerBeat;
        if (m_pixelPerBeat > 8.0 || barline == 0)
        {
            
            if (m_pixelPerBeat > 3 || barNum % 4 == 0)
            {
                g.drawLine(
                    beatLine
                    , lineStartY
                    , beatLine
                    , getHeight()
                );
            }
        }
        if (m_pixelPerBeat > 100.0)
        {
            for (auto i = 0; i < 4; i++)
            {
                auto pos = (beatLine - m_pixelPerBeat) + (i * m_pixelPerBeat / 4);
                g.drawLine(
                    pos
                    , g.getCurrentFont().getHeight()
                    , pos
                    , getHeight()
                );
            }
        }
        //std::cout << m_pixelPerBeat << std::endl;
    }
    
}

void TimeLineComponent::mouseDown(const MouseEvent& event)
{
    m_posAtMouseDown = event.getMouseDownScreenPosition();
}

void TimeLineComponent::mouseDrag(const MouseEvent& event)
{
    setMouseCursor(MouseCursor::NoCursor);
    
    //move
    auto distanceX = 0;
    if (event.getDistanceFromDragStartX() > 5 || event.getDistanceFromDragStartX() < -5)
    {
        distanceX = event.getDistanceFromDragStartX();
        m_viewPort.setViewPosition(
            m_viewPort.getViewPosition().getX()
            - event.getDistanceFromDragStartX()
            , m_viewPort.getViewPosition().getY()
        );
    }
    //zoom
    else
    {
        auto clickPos = event.getMouseDownPosition().getX() / static_cast<double>(m_pixelPerBeat);
        m_pixelPerBeat = m_pixelPerBeat - event.getDistanceFromDragStartY();
        m_pixelPerBeat = m_pixelPerBeat < 3 ? 3 : m_pixelPerBeat;
        auto currentPos = event.getMouseDownPosition().getX() / static_cast<double>(m_pixelPerBeat);
        auto newPos = (currentPos - clickPos) * m_pixelPerBeat;

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

void TimeLineComponent::mouseUp(const MouseEvent& event)
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

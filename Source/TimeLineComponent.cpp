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
            else
            {
                g.drawSingleLineText(
                    String(barNum)
                    , (barNum * m_pixelPerBeat * 4) - (m_pixelPerBeat * 4) + 3
                    , g.getCurrentFont().getHeight()
                );
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
            g.drawLine(
                beatLine
                , lineStartY
                , beatLine
                , getHeight()
            );
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

    m_viewPort.setViewPosition(
        m_viewPort.getViewPosition().getX()
        - event.getDistanceFromDragStartX()
        , m_viewPort.getViewPosition().getY()
    );

    double oldPPB = m_pixelPerBeat;
    auto delta = event.getDistanceFromDragStartY();
    
    m_pixelPerBeat = m_pixelPerBeat - delta;
    m_pixelPerBeat = m_pixelPerBeat < 2 ? 2 : m_pixelPerBeat;


    double newPPB = m_pixelPerBeat;
    double newPos = m_viewPort.getHorizontalScrollBar().getCurrentRangeStart()  / oldPPB * newPPB;

    m_viewPort.getHorizontalScrollBar().setCurrentRangeStart(newPos);
    m_distanceX = m_distanceX - event.getDistanceFromDragStartX();
    
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

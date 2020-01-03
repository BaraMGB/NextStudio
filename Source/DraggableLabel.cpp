/*
  ==============================================================================

    DraggableLabel.cpp
    Created: 14 Dec 2019 5:17:01pm
    Author:  ubu1710

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "DraggableLabel.h"

//==============================================================================
DraggableLabel::DraggableLabel(int value, int minValue, int maxValue, String separator, Justification justification, int position)
    : m_valueTmp            (0),
      m_overflow            (0),
      m_downStop            (false),
      m_position            (position),
      m_separator           (separator),
      m_accuracy            (0.1f)
{
    setJustificationType(justification);
    m_minValue = minValue;
    m_maxValue = maxValue;
    setValue(value);
}

void DraggableLabel::paint (Graphics& g)
{
    g.fillAll(Colours::black);
    Label::paint(g);
}

void DraggableLabel::mouseDrag(const MouseEvent& event)
{
    int dragDistance = event.getDistanceFromDragStartY();
    if (dragDistance - m_valueTmp > 0)
    {
        count(-1);
    }
    else
    {
        count(1);
        
    }
    m_valueTmp = dragDistance;
    Label::mouseDrag(event);
}

void DraggableLabel::resized()
{
}

void DraggableLabel::changeListenerCallback(ChangeBroadcaster *source)
{
    DraggableLabel * sourceDragLabel = dynamic_cast<DraggableLabel*> (source);
    if (sourceDragLabel)
    {
        //do we underrun?
        if (m_value + sourceDragLabel->overflowCount() < m_minValue)
        {
            if (m_position == 1)
            {
                m_downStop = true;
            }
            if (m_downStop)
            {
                sourceDragLabel->m_downStop = true;
            }
        }
        //or overrun
        else
        {
            if (m_position == 1)
            {
                m_downStop = false;
            }
            sourceDragLabel->m_downStop = false;
        }
        count(sourceDragLabel->overflowCount());
    }
    sourceDragLabel = nullptr;
    delete sourceDragLabel;
}

void DraggableLabel::count(const int step)
{
    int value = m_value + step;
    const int rangeDistance = m_maxValue - m_minValue + 1;
    if (value > m_maxValue)
    {
        value = value - rangeDistance;
        m_value = m_value - rangeDistance;
        m_overflow = 1;
        sendChangeMessage();
    }
    else if (value < m_minValue)
    {
        m_overflow = -1;
        sendChangeMessage();
        if (m_downStop)
        {
            value = m_minValue;
        }
        else
        {
            value = value + rangeDistance;
            m_value = m_value + rangeDistance;
        }
    }
    setValue(value);
}

void DraggableLabel::setValue(const int value)
{
    if (value > m_maxValue)
    {
        m_value = m_maxValue;
    }
    else if (value < m_minValue)
    {
        m_value = m_minValue;
    }
    else
    {
        m_value = value;
    }
    setText(String(m_value) + m_separator, juce::NotificationType::sendNotification);
}

void DraggableLabel::setDownStop(bool downStop)
{
    m_downStop = downStop;
}

int DraggableLabel::getMinValue() const
{
    return m_minValue;
}


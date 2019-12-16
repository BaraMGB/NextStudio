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
DraggableLabel::DraggableLabel(int value, int minValue, int maxValue, String separator, Justification justification)
    : m_valueTmp            (0),
      m_overflow            (0),
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

void DraggableLabel::mouseUp(const MouseEvent& /*event*/)
{
    m_overflow = 0;
}

void DraggableLabel::mouseDrag(const MouseEvent& event)
{
    int dragDistance = event.getDistanceFromDragStartY();
//    if (dragDistance * m_accuracy == static_cast<int>(dragDistance * m_accuracy))
//    {
    count(dragDistance - m_valueTmp > 0 ? -1 : 1);
//    }
    m_valueTmp = dragDistance;
}

void DraggableLabel::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void DraggableLabel::changeListenerCallback(ChangeBroadcaster *source)
{

    DraggableLabel * sourceDragLabel = dynamic_cast<DraggableLabel*> (source);
    if (sourceDragLabel)
    {
        count(sourceDragLabel->overflowCount());
    }
    sourceDragLabel = nullptr;
    delete sourceDragLabel;
}

void DraggableLabel::count(const int step)
{
    int value = m_value;
    value = value + step;
    const int rangeDistance = m_maxValue - m_minValue + 1;
    if (value > m_maxValue)
    {
        value = value - rangeDistance;
        m_value = m_value - rangeDistance;
        sendChangeMessage();
        m_overflow = 1;
    }
    else if (value < m_minValue)
    {
            value = value + rangeDistance;
            m_value = m_value + rangeDistance;
            m_overflow = -1;
            sendChangeMessage();
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



/*
  ==============================================================================

    SpinBoxGroup.cpp
    Created: 26 Jan 2020 8:05:58pm
    Author:  BaraMGB

  ==============================================================================
*/

#include "SpinBoxGroup.h"

SpinBoxGroup::SpinBoxGroup() 
    : m_font(Font(15))
    , m_fontColour(Colours::beige)
{
}

SpinBoxGroup::~SpinBoxGroup()
{
    m_spinBoxes.clear();
}

void SpinBoxGroup::paint(Graphics &/*g*/)
{
}

void SpinBoxGroup::resized()
{
    auto area = getLocalBounds();
    for (auto& spinBox : m_spinBoxes)
    {
        spinBox->setBounds(area.removeFromLeft(spinBox->getNeededWidth()));
    }
}

void SpinBoxGroup::changeListenerCallback(ChangeBroadcaster *source)
{
    
    auto spinBox = dynamic_cast<SpinBox*>(source);
    if(spinBox)
    {
        if (spinBox->getParentComponent() == this)
        {
            
        
        int indexOfSpinBox = -1;

        for (auto &i : m_spinBoxes)
        {
            if (i.get() == spinBox)
            {
                indexOfSpinBox = m_spinBoxes.indexOf(i);
            }
        }

        if (indexOfSpinBox > 0)
        {
            auto & preSpinBox = m_spinBoxes.getReference(indexOfSpinBox - 1);
            auto preValue = preSpinBox->getValue();
            auto preStep  = preSpinBox->getStep();

            if (spinBox->overrun())
            {
                preSpinBox->setValue(preValue + preStep);
                spinBox->setValue(spinBox->getMin());
            }

            if (spinBox->underrun())
            {
                preSpinBox->setValue(preValue - preStep);
                if(preSpinBox->getValue() != preValue)
                {
                    spinBox->setValue(spinBox->getMax());
                }
            }
        }
        }
    }
}

void SpinBoxGroup::addSpinBox(int digits, int init, int min, int max, String separator, int step)
{
    auto spinbox = std::make_unique<SpinBox>(digits, init, min, max, m_font, m_fontColour, separator, step);
    addAndMakeVisible(*spinbox);
    spinbox->addChangeListener(this);
    m_spinBoxes.add(std::move(spinbox));
}

bool SpinBoxGroup::setValue(int part, int value)
{
    if(part < 0 || part > m_spinBoxes.size())
    {
        return false;
    }
    m_spinBoxes.getReference(part)->setValue(value);
    return true;
}

int SpinBoxGroup::getSpinBoxCount()
{
    return m_spinBoxes.size();
}

bool SpinBoxGroup::isDragging()
{
    for (auto i = 0; i < m_spinBoxes.size(); i++)
    {
        if (m_spinBoxes.getReference(i)->isDragging())
        {
            m_draggedBox = i;
            return true;
        }
    }
    return false;
}

int SpinBoxGroup::getValue(int part)
{
    if(m_spinBoxes.getReference(part))
    {
        return m_spinBoxes.getReference(part)->getValue();

    }
    else
    {
        return -1;
    }
}

int SpinBoxGroup::getDraggedBox() const
{
    return m_draggedBox;
}

void SpinBoxGroup::setFontColour(juce::Colour colour)
{
    m_fontColour = colour;
}

void SpinBoxGroup::setFont(juce::Font font)
{
    m_font = font;
}

juce::Font SpinBoxGroup::getFont()
{
    return m_font;
}

int SpinBoxGroup::getNeededWidth()
{
    auto width(0);
    for (auto& spinbox : m_spinBoxes)
    {
        width = width + spinbox->getNeededWidth();
    }
    return width;
}

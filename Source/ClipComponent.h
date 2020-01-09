/*
  ==============================================================================

    ClipComponent.h
    Created: 8 Jan 2020 11:53:16pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
 
//==============================================================================
/*
*/
class ClipComponent    : public Component
{
public:
    ClipComponent(double position, double lenght)
        : m_position(position)
        , m_length(lenght)
    {}
    ~ClipComponent(){}

    void paint (Graphics&) override;
    void resized() override;

    double clipPosition()
    {
        return m_position;
    }

    void setClipPostion(double pos)
    {
        m_position = pos;
    }

    double clipLength()
    {
        return m_length;
    }

    void setClipLength(double length)
    {
        m_length = length;
    }

    void setClipColour(Colour colour)
    {
        m_colour = colour;
    }
private:
    double m_position, m_length;
    Colour m_colour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};

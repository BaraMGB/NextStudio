/*
  ==============================================================================

    TransportDisplayComponent.h
    Created: 7 Jan 2020 8:30:43pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "DraggableLabel.h"


class TransportDisplayComponent : public Component
{
public:
    //==============================================================================
    TransportDisplayComponent();
    ~TransportDisplayComponent() override;

    void paint(Graphics& g) override;
    void resized() override;
    void setPosition(double ppqPosition);


private:
    DraggableLabel m_bars;
    DraggableLabel m_beat;
    DraggableLabel m_quat;
    DraggableLabel m_cent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportDisplayComponent)
};

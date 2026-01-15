/*
  ==============================================================================

    NonAutomatableParameter.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Utilities.h"

class NonAutomatableParameterComponent : public juce::Component
{
public:
    NonAutomatableParameterComponent(juce::Value v, juce::String name, int rangeStart, int rangeEnd);
    void resized() override;

    void updateLabel ();
    void setSliderRange(double start, double end, double interval = 1.0);

private:
    juce::Slider       m_knob;
    juce::Label        m_valueLabel;
    juce::Label        m_titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutomatableParameterComponent)
};

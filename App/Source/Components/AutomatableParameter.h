/*
  ==============================================================================

    AutomatableParameter.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Utilities.h"
#include "AutomatableSlider.h"

namespace te = tracktion_engine;

class AutomatableParameterComponent : public juce::Component
, private te::AutomatableParameter::Listener
{
public:
    AutomatableParameterComponent(const te::AutomatableParameter::Ptr ap, juce::String name);

    ~AutomatableParameterComponent() override;

    void resized() override;

    void setKnobColour (juce::Colour colour);
    void updateLabel ();

    void curveHasChanged(te::AutomatableParameter&) override {}
    void currentValueChanged(te::AutomatableParameter&) override { updateLabel(); }

private:
    std::unique_ptr<AutomatableSliderComponent>       m_knob;
    juce::Label                                       m_valueLabel;
    juce::Label                                       m_titleLabel;
    te::AutomatableParameter::Ptr                     m_automatableParameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableParameterComponent)
};

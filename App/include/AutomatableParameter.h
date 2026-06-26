/*
  ==============================================================================

    AutomatableParameter.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomatableSlider.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableParameterComponent
    : public juce::Component
    , private te::AutomatableParameter::Listener
{
public:
    AutomatableParameterComponent(const te::AutomatableParameter::Ptr ap, juce::String name);

    ~AutomatableParameterComponent() override;

    void resized() override;

    void setKnobColour(juce::Colour colour);
    void setKnobSkewFromMidPoint(double midPoint);
    void updateLabel();

    void curveHasChanged(te::AutomatableParameter &) override {}
    void currentValueChanged(te::AutomatableParameter &) override;
    void parameterChanged(te::AutomatableParameter &, float) override;

protected:
    // Allow derived classes to access these members
    std::unique_ptr<AutomatableSliderComponent> m_knob;
    juce::Label m_valueLabel;
    juce::Label m_titleLabel;
    te::AutomatableParameter::Ptr m_automatableParameter;

    // Virtual method for custom display formatting
    virtual juce::String getCustomDisplayString() const { return {}; }

private:
    void updateLabelForValue(float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableParameterComponent)
};

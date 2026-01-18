/*
  ==============================================================================

    AutomatableEnvelopeParameter.cpp
    Created: 18 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableEnvelopeParameter.h"

AutomatableEnvelopeParameter::AutomatableEnvelopeParameter(const te::AutomatableParameter::Ptr ap, juce::String name)
    : AutomatableParameterComponent(ap, name)
{
}

juce::String AutomatableEnvelopeParameter::getCustomDisplayString() const
{
    auto value = m_automatableParameter->getCurrentValue();
    auto paramName = m_titleLabel.getText();
    
    // Time parameters (Attack, Decay, Release)
    if (paramName.containsIgnoreCase("attack") || paramName.containsIgnoreCase("decay") || 
        paramName.containsIgnoreCase("release"))
    {
        float timeMs = value * 1000.0f;
        if (timeMs < 10.0f)
            return juce::String(timeMs, 1) + "ms";
        else if (timeMs < 1000.0f)
            return juce::String(timeMs, 0) + "ms";
        else
            return juce::String(timeMs / 1000.0f, 2) + "s";
    }
    
    // Sustain parameters (display as percentage)
    if (paramName.containsIgnoreCase("sustain"))
    {
        return juce::String(value * 100.0f, 1) + "%";
    }
    
    // Return empty string to use default formatting
    return {};
}
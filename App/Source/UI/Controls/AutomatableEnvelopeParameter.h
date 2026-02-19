/*
  ==============================================================================

    AutomatableEnvelopeParameter.h
    Created: 18 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "UI/Controls/AutomatableParameter.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class AutomatableEnvelopeParameter : public AutomatableParameterComponent
{
public:
    AutomatableEnvelopeParameter(const te::AutomatableParameter::Ptr ap, juce::String name);
    ~AutomatableEnvelopeParameter() override = default;

    void updateLabel();

protected:
    juce::String getCustomDisplayString() const override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableEnvelopeParameter)
};
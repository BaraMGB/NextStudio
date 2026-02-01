/*
  ==============================================================================

    ParameterComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Utilities/Utilities.h"
#include "Utilities/ApplicationViewState.h"
#include "UI/Controls/AutomatableSlider.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class ParameterComponent : public juce::Component
{
public:
    explicit ParameterComponent(te::AutomatableParameter &ap, ApplicationViewState &appstate);
    ~ParameterComponent() override = default;
    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    bool isDragged() { return m_isDragged; }
    te::AutomatableParameter &getParameter() { return m_parameter; }

private:
    te::AutomatableParameter &m_parameter;
    ApplicationViewState &m_appState;
    juce::Label m_parameterName;
    AutomatableSliderComponent m_parameterSlider;

    bool m_isDragged{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterComponent)
};

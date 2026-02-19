/*
  ==============================================================================

    AutomatableToggle.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class AutomatableToggleButton
    : public juce::ToggleButton
    , public te::AutomatableParameter::Listener
    , public te::AutomationDragDropTarget
{
public:
    explicit AutomatableToggleButton(te::AutomatableParameter::Ptr ap);

    ~AutomatableToggleButton() override;

    void mouseDown(const juce::MouseEvent &e) override;

    // AutomatableParameter::Listener overrides
    void curveHasChanged(te::AutomatableParameter &) override {}
    void currentValueChanged(te::AutomatableParameter &p) override;

    // AutomationDragDropTarget overrides
    bool hasAnAutomatableParameter() override { return true; }
    void chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/) override;

private:
    te::AutomatableParameter::Ptr m_automatableParameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableToggleButton)
};

class AutomatableToggleComponent : public juce::Component
{
public:
    AutomatableToggleComponent(te::AutomatableParameter::Ptr ap, juce::String name);

    void resized() override;

private:
    std::unique_ptr<AutomatableToggleButton> m_button;
    juce::Label m_titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableToggleComponent)
};

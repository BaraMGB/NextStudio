/*
  ==============================================================================

    AutomatableComboBox.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomatableMidiLearnSupport.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableComboBoxComponent
    : public juce::ComboBox
    , public te::AutomatableParameter::Listener
    , public te::AutomationDragDropTarget
{
public:
    explicit AutomatableComboBoxComponent(te::AutomatableParameter::Ptr ap);

    ~AutomatableComboBoxComponent() override;

    // AutomatableParameter::Listener overrides
    void curveHasChanged(te::AutomatableParameter &) override {}
    void currentValueChanged(te::AutomatableParameter &p) override;
    void parameterChanged(te::AutomatableParameter &p, float) override;

    // AutomationDragDropTarget overrides
    bool hasAnAutomatableParameter() override { return true; }
    void chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/) override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void paintOverChildren(juce::Graphics &g) override;
    bool keyPressed(const juce::KeyPress &key) override;

private:
    te::AutomatableParameter::Ptr m_automatableParameter;
    std::unique_ptr<AutomatableMidiLearnSupport> m_midiLearn;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableComboBoxComponent)
};

class AutomatableChoiceComponent : public juce::Component
{
public:
    AutomatableChoiceComponent(te::AutomatableParameter::Ptr ap, juce::String name);

    void resized() override;

private:
    std::unique_ptr<AutomatableComboBoxComponent> m_combo;
    juce::Label m_titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableChoiceComponent)
};

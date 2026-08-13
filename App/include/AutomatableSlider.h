/*
  ==============================================================================

    AutomatableSlider.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomatableMidiLearnSupport.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableSliderComponent
    : public juce::Slider
    , public te::AutomationDragDropTarget
    , public te::AutomatableParameter::Listener
{
public:
    explicit AutomatableSliderComponent(const te::AutomatableParameter::Ptr ap);
    ~AutomatableSliderComponent() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    te::AutomatableParameter::Ptr getAutomatableParameter();
    void setParameter(te::AutomatableParameter::Ptr newParam);
    void bindSliderToParameter();

    [[nodiscard]] juce::Colour getTrackColour() const;
    void setTrackColour(juce::Colour colour);

    // AutomationDragDropTarget overrides
    bool hasAnAutomatableParameter() override;
    void chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> startLearnMode) override;

    void curveHasChanged(te::AutomatableParameter &) override;
    void currentValueChanged(te::AutomatableParameter &) override;
    void parameterChanged(te::AutomatableParameter &, float) override;

    void startedDragging() override;
    void stoppedDragging() override;
    void valueChanged() override;

    void mouseEnter(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void enablementChanged() override;
    void paintOverChildren(juce::Graphics &g) override;
    bool keyPressed(const juce::KeyPress &key) override;
    void resized() override;

private:
    void updateModDepthVisibility();
    void showValueEntryDialog();

    juce::Slider m_modDepthSlider;
    te::AutomatableParameter::Ptr m_automatableParameter;
    std::unique_ptr<AutomatableMidiLearnSupport> m_midiLearn;
    juce::Colour m_trackColour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableSliderComponent)
};

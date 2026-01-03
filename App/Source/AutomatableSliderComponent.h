/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/


#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"
#include "EditViewState.h"

namespace te = tracktion_engine;

class AutomatableSliderComponent : public juce::Slider
                                 , public te::AutomationDragDropTarget
                                 , public te::AutomatableParameter::Listener
{
public:

    explicit AutomatableSliderComponent(const te::AutomatableParameter::Ptr ap);
    ~AutomatableSliderComponent() override;

    void mouseDown (const juce::MouseEvent& e) override;

    te::AutomatableParameter::Ptr getAutomatableParameter();
    void bindSliderToParameter ();

    [[nodiscard]] juce::Colour getTrackColour() const;
    void setTrackColour(juce::Colour colour);

    // AutomationDragDropTarget overrides
    bool hasAnAutomatableParameter() override;
    void chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam,
                                     std::function<void()> startLearnMode) override;

    void curveHasChanged(te::AutomatableParameter&) override;
    void currentValueChanged(te::AutomatableParameter&) override;

    void startedDragging() override;
    void stoppedDragging() override;
    void valueChanged() override;

private:

    te::AutomatableParameter::Ptr m_automatableParameter;
    juce::Colour m_trackColour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableSliderComponent)
};

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

class AutomatableComboBoxComponent : public juce::ComboBox
    , public te::AutomatableParameter::Listener
    , public te::AutomationDragDropTarget
{
public:
    explicit AutomatableComboBoxComponent(te::AutomatableParameter::Ptr ap);

    ~AutomatableComboBoxComponent() override;

    // AutomatableParameter::Listener overrides
    void curveHasChanged(te::AutomatableParameter&) override {}
    void currentValueChanged(te::AutomatableParameter& p) override;

    // AutomationDragDropTarget overrides
    bool hasAnAutomatableParameter() override { return true; }
    void chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/) override;

private:
    te::AutomatableParameter::Ptr m_automatableParameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableComboBoxComponent)
};

class AutomatableChoiceComponent : public juce::Component
{
public:
    AutomatableChoiceComponent(te::AutomatableParameter::Ptr ap, juce::String name);

    void resized() override;

private:
    std::unique_ptr<AutomatableComboBoxComponent> m_combo;
    juce::Label m_titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableChoiceComponent)
};


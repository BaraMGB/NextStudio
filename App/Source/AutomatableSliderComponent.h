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
/**
     Wraps a te::AutomatableParameter as a juce::ValueSource so it can be used as
     a Value for example in a Slider.
 */
class ParameterValueSource  : public juce::Value::ValueSource,
                              private te::AutomatableParameter::Listener
{
public:

    ParameterValueSource (te::AutomatableParameter::Ptr p)
    : param (p)
    {
        param->addListener (this);
    }

    ~ParameterValueSource() override
    {
        param->removeListener (this);
    }

    juce::var getValue() const override
    {
        return param->getCurrentValue();
    }

    void setValue (const juce::var& newValue) override
    {
        param->setParameter (static_cast<float> (newValue), juce::sendNotification);
    }

private:

    te::AutomatableParameter::Ptr param;

    void curveHasChanged (te::AutomatableParameter&) override
    {
        sendChangeMessage (false);
    }

    void currentValueChanged (te::AutomatableParameter&) override
    {
        sendChangeMessage (false);
    }

};

class AutomatableSliderComponent : public juce::Slider
{
public:

    explicit AutomatableSliderComponent(const te::AutomatableParameter::Ptr ap);
    ~AutomatableSliderComponent() override;

    void mouseDown (const juce::MouseEvent& e) override;

    te::AutomatableParameter::Ptr getAutomatableParameter();
    void bindSliderToParameter ();

    [[nodiscard]] juce::Colour getTrackColour() const;
    void setTrackColour(juce::Colour colour);

private:

    te::AutomatableParameter::Ptr m_automatableParameter;
    juce::Colour m_trackColour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableSliderComponent)
};

class AutomatableParameterComponent : public juce::Component
{
public:
    AutomatableParameterComponent(const te::AutomatableParameter::Ptr ap, juce::String name)
    : m_automatableParameter (ap)
    {
        m_knob = std::make_unique<AutomatableSliderComponent>(ap);
        m_knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        m_knob->setDoubleClickReturnValue(true, ap->getCurrentValue());
        m_knob->onValueChange = [this] {
            updateLabel();
        };

        addAndMakeVisible(*m_knob);

        m_valueLabel.setJustificationType(juce::Justification::centredTop);
        m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
        updateLabel();
        addAndMakeVisible(m_valueLabel);

        m_titleLabel.setJustificationType(juce::Justification::centredBottom);
        m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
        m_titleLabel.setText(name, juce::dontSendNotification);

        addAndMakeVisible(m_titleLabel);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto h = area.getHeight() / 4;

        m_titleLabel.setBounds(area.removeFromTop(h));
        m_knob->setBounds(area.removeFromTop(h * 2));
        m_valueLabel.setBounds(area.removeFromTop(h));
    }

    void setKnobColour (juce::Colour colour)
    {
        m_knob->setTrackColour(colour);
        repaint();
    }

    void updateLabel ()
    {
        m_valueLabel.setText(m_automatableParameter->getCurrentValueAsString(), juce::NotificationType::dontSendNotification);
    }

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
    NonAutomatableParameterComponent(juce::Value v, juce::String name, int rangeStart, int rangeEnd)
    {
        m_knob.setRange(rangeStart, rangeEnd, 1); 
        m_knob.getValueObject().referTo(v);
        m_titleLabel.setText(name, juce::dontSendNotification);
        m_knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);

        m_knob.onValueChange = [this] { updateLabel(); };
        m_valueLabel.setJustificationType(juce::Justification::centredTop);
        m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
        updateLabel();

        m_titleLabel.setJustificationType(juce::Justification::centredBottom);
        m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));

        Helpers::addAndMakeVisible(*this,{&m_titleLabel, &m_knob, &m_valueLabel});
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto h = area.getHeight() / 4;

        m_titleLabel.setBounds(area.removeFromTop(h));
        m_knob.setBounds(area.removeFromTop(h * 2));
        m_valueLabel.setBounds(area.removeFromTop(h));
    }

    void updateLabel ()
    {
        m_valueLabel.setText(juce::String(m_knob.getValue()), juce::dontSendNotification);
    }

    void setSliderRange(double start, double end, double interval = 1.0)
    {
        m_knob.setRange(start, end, interval);
    }

private:

    juce::Slider       m_knob;
    juce::Label        m_valueLabel;
    juce::Label        m_titleLabel;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NonAutomatableParameterComponent)
};


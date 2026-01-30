/*
  ==============================================================================

    NonAutomatableParameter.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "NonAutomatableParameter.h"

NonAutomatableParameterComponent::NonAutomatableParameterComponent(juce::Value v, juce::String name, int rangeStart, int rangeEnd)
{
    m_knob.setRange(rangeStart, rangeEnd, 1);
    m_knob.getValueObject().referTo(v);
    m_titleLabel.setText(name, juce::dontSendNotification);
    m_knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);

    m_knob.onValueChange = [this] { updateLabel(); };
    m_valueLabel.setJustificationType(juce::Justification::centred);
    m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    updateLabel();

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));

    Helpers::addAndMakeVisible(*this, {&m_titleLabel, &m_knob, &m_valueLabel});
}

void NonAutomatableParameterComponent::resized()
{
    auto area = getLocalBounds();

    // Top: Title (Name)
    m_titleLabel.setBounds(area.removeFromTop(20));

    // Bottom: Value
    m_valueLabel.setBounds(area.removeFromBottom(15));

    // Rest: Knob
    m_knob.setBounds(area);
}

void NonAutomatableParameterComponent::updateLabel() { m_valueLabel.setText(juce::String(m_knob.getValue()), juce::dontSendNotification); }

void NonAutomatableParameterComponent::setSliderRange(double start, double end, double interval) { m_knob.setRange(start, end, interval); }

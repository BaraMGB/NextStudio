/*
  ==============================================================================

    AutomatableParameter.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableParameter.h"

AutomatableParameterComponent::AutomatableParameterComponent(const te::AutomatableParameter::Ptr ap, juce::String name)
    : m_automatableParameter (ap)
{
    m_knob = std::make_unique<AutomatableSliderComponent>(ap);
    m_knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_knob->onValueChange = [this] {
        updateLabel();
    };

    addAndMakeVisible(*m_knob);

    m_valueLabel.setJustificationType(juce::Justification::centred);
    m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    updateLabel();
    addAndMakeVisible(m_valueLabel);

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    m_titleLabel.setText(name, juce::dontSendNotification);

    addAndMakeVisible(m_titleLabel);

    m_automatableParameter->addListener(this);
}

AutomatableParameterComponent::~AutomatableParameterComponent()
{
    m_automatableParameter->removeListener(this);
}
void AutomatableParameterComponent::resized() 
{
    auto area = getLocalBounds();
    
    // Top: Title (Name)
    m_titleLabel.setBounds(area.removeFromTop(20));
    
    // Bottom: Value
    m_valueLabel.setBounds(area.removeFromBottom(15));
    
    // Rest: Knob
    m_knob->setBounds(area);
}

void AutomatableParameterComponent::setKnobColour (juce::Colour colour)
{
    m_knob->setTrackColour(colour);
    repaint();
}

void AutomatableParameterComponent::updateLabel()
{
    m_valueLabel.setText(m_automatableParameter->getCurrentValueAsString(), juce::NotificationType::dontSendNotification);
}

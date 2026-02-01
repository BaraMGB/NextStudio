/*
  ==============================================================================

    ParameterComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "UI/Controls/ParameterComponent.h"

ParameterComponent::ParameterComponent(te::AutomatableParameter &ap, ApplicationViewState &appstate)
    : m_parameter(ap),
      m_appState(appstate),
      m_parameterSlider(&ap)
{
    m_parameterName.setText(m_parameter.getParameterName(), juce::dontSendNotification);
    m_parameterName.setInterceptsMouseClicks(false, false);

    m_parameterSlider.setOpaque(false);
    addAndMakeVisible(m_parameterName);
    addAndMakeVisible(m_parameterSlider);
    m_parameterSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_parameterSlider.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
}

void ParameterComponent::paint(juce::Graphics &g)
{
    g.setColour(m_appState.getBackgroundColour1());
    auto area = getLocalBounds();
    area.reduce(2, 2);
    area.removeFromRight(10);
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, false, true, false);
}

void ParameterComponent::resized()
{
    auto area = getLocalBounds();

    m_parameterSlider.setBounds(area.removeFromLeft(area.getHeight()));
    m_parameterName.setBounds(area);
}

void ParameterComponent::mouseDown(const juce::MouseEvent &e) { m_isDragged = true; }

void ParameterComponent::mouseUp(const juce::MouseEvent &e) { m_isDragged = false; }

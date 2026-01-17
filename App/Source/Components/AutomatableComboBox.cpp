/*
  ==============================================================================

    AutomatableComboBox.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableComboBox.h"

AutomatableComboBoxComponent::AutomatableComboBoxComponent(te::AutomatableParameter::Ptr ap)
: m_automatableParameter(ap)
{
    ap->addListener(this);
    auto range = ap->valueRange;
    int index = 1;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    for (float v = range.start; v <= range.end + 0.001f; v += step)
    {
        juce::String text;
        if (ap->hasLabels())
            text = ap->getLabelForValue(v);

        if (text.isEmpty())
            text = ap->valueToString(v);

        addItem(text, index++);
    }

    // Initial update
    currentValueChanged(*ap);

    onChange = [this]
        {
            int index = getSelectedId();
            if (index > 0)
            {
                auto range = m_automatableParameter->valueRange;
                float step = range.interval > 0.0f ? range.interval : 1.0f;
                float newVal = range.start + ((float)(index - 1) * step);

                if (m_automatableParameter->getCurrentValue() != newVal)
                    m_automatableParameter->setParameter(newVal, juce::sendNotification);
            }
        };
}
AutomatableComboBoxComponent::~AutomatableComboBoxComponent()
{
    m_automatableParameter->removeListener(this);
}

void AutomatableComboBoxComponent::currentValueChanged(te::AutomatableParameter& p)
{
    auto range = p.valueRange;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    int id = (int)std::round((p.getCurrentValue() - range.start) / step) + 1;

    if (getSelectedId() != id)
        setSelectedId(id, juce::dontSendNotification);
}

void AutomatableComboBoxComponent::chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/)
{
    handleChosenParam(m_automatableParameter);
}


// ---------------------------------------------------------------------------------------------------------------------------------

AutomatableChoiceComponent::AutomatableChoiceComponent(te::AutomatableParameter::Ptr  ap, juce::String name)
{
    m_combo = std::make_unique<AutomatableComboBoxComponent>(ap);
    addAndMakeVisible(*m_combo);

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    m_titleLabel.setText(name, juce::dontSendNotification);
    addAndMakeVisible(m_titleLabel);
}

void AutomatableChoiceComponent::resized()
{
    auto area = getLocalBounds();
    m_titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(8); // Intermediate padding
    
    // Position ComboBox
    auto comboArea = area.removeFromTop(30); 
    m_combo->setBounds(comboArea.withSizeKeepingCentre(area.getWidth() - 10, 30));
}

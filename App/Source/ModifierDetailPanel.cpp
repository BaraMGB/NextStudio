/*
  ==============================================================================

    ModifierDetailPanel.cpp
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#include "ModifierDetailPanel.h"
#include "Utilities.h"

ModifierDetailPanel::ModifierDetailPanel(EditViewState &evs)
    : m_evs(evs)
{
    m_placeholderLabel.setText("No Modifier Selected", juce::dontSendNotification);
    m_placeholderLabel.setJustificationType(juce::Justification::centred);
    m_placeholderLabel.setColour(juce::Label::textColourId, evs.m_applicationState.getTextColour().withAlpha(0.5f));
    addAndMakeVisible(m_placeholderLabel);
}

ModifierDetailPanel::~ModifierDetailPanel() {}

void ModifierDetailPanel::setModifier(te::Modifier::Ptr m)
{
    m_view.reset();

    if (m) {
        if (dynamic_cast<te::LFOModifier *>(m.get()))
            m_view = std::make_unique<LFOModifierComponent>(m_evs, m);
        else if (dynamic_cast<te::StepModifier *>(m.get()))
            m_view = std::make_unique<StepModifierComponent>(m_evs, m);
        else
            m_view = std::make_unique<ModifierViewComponent>(m_evs, m);

        addAndMakeVisible(m_view.get());
        m_placeholderLabel.setVisible(false);
    }
    else {
        m_placeholderLabel.setVisible(true);
    }

    resized();
}

void ModifierDetailPanel::paint(juce::Graphics &g)
{
    g.fillAll(m_evs.m_applicationState.getBackgroundColour2());
}

void ModifierDetailPanel::resized()
{
    auto area = getLocalBounds();
    m_placeholderLabel.setBounds(area);

    if (m_view)
        m_view->setBounds(area);
}

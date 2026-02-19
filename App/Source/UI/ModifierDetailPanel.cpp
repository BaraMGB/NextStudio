/*
  ==============================================================================

    ModifierDetailPanel.cpp
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#include "UI/ModifierDetailPanel.h"
#include "Utilities/Utilities.h"

namespace
{
constexpr int kHeaderWidth = 20;
}

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
    m_currentModifier = m;
    m_view.reset();

    if (m)
    {
        if (dynamic_cast<te::LFOModifier *>(m.get()))
            m_view = std::make_unique<LFOModifierComponent>(m_evs, m);
        else if (dynamic_cast<te::StepModifier *>(m.get()))
            m_view = std::make_unique<StepModifierComponent>(m_evs, m);
        else
            m_view = std::make_unique<ModifierViewComponent>(m_evs, m);

        addAndMakeVisible(m_view.get());
        m_placeholderLabel.setVisible(false);
    }
    else
    {
        m_placeholderLabel.setVisible(true);
    }

    resized();
}

void ModifierDetailPanel::paint(juce::Graphics &g)
{
    auto headerColour = m_evs.m_applicationState.getPrimeColour();
    if (auto track = m_currentModifier != nullptr ? te::getTrackContainingModifier(m_evs.m_edit, m_currentModifier) : nullptr)
        headerColour = track->getColour();

    auto title = juce::String("Modifier");
    if (m_currentModifier != nullptr)
    {
        title = m_currentModifier->getName();
        if (title.isEmpty())
        {
            if (m_currentModifier->state.hasType(te::IDs::LFO))
                title = "LFO";
            else if (m_currentModifier->state.hasType(te::IDs::STEP))
                title = "Step Seq";
            else if (m_currentModifier->state.hasType(te::IDs::RANDOM))
                title = "Random";
        }
    }

    GUIHelpers::drawHeaderBox(g, getLocalBounds().reduced(2).toFloat(), headerColour, m_evs.m_applicationState.getBorderColour(), m_evs.m_applicationState.getBackgroundColour2(), (float)kHeaderWidth, GUIHelpers::HeaderPosition::left, title);
}

void ModifierDetailPanel::resized()
{
    auto area = getLocalBounds().reduced(2);
    area.removeFromLeft(kHeaderWidth);
    area.removeFromRight(kHeaderWidth);

    m_placeholderLabel.setBounds(area);

    if (m_view)
        m_view->setBounds(area);
}

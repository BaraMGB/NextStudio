/*
  ==============================================================================

    FilterPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Filter/FilterPluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"

FilterPluginComponent::FilterPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    auto um = &evs.m_edit.getUndoManager();

    m_freqPar = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("frequency"), "Freq");
    m_modeLabel.setJustificationType(juce::Justification::centred);

    m_modeButton.onStateChange = [this, um]
    {
        if (m_modeButton.getToggleState())
            m_plugin->state.setProperty(te::IDs::mode, "highpass", um);
        else
            m_plugin->state.setProperty(te::IDs::mode, "lowpass", um);
        updateLabel(*um);
    };

    addAndMakeVisible(m_modeButton);
    addAndMakeVisible(m_modeLabel);
    addAndMakeVisible(*m_freqPar);
    m_plugin->state.addListener(this);
    updateLabel(*um);
}

void FilterPluginComponent::resized()
{
    auto bounds = getLocalBounds();
    auto h = bounds.getHeight() / 12;
    bounds.removeFromTop(h);
    auto modeButtonRect = bounds.removeFromTop(h * 3);
    m_modeButton.setBounds(modeButtonRect.reduced(modeButtonRect.getWidth() / 4, modeButtonRect.getHeight() / 4));
    m_modeLabel.setBounds(bounds.removeFromTop(h));

    bounds.removeFromTop(h * 2);
    m_freqPar->setBounds(bounds.removeFromTop(h * 4));
}

void FilterPluginComponent::paint(juce::Graphics &g) {}

void FilterPluginComponent::updateLabel(juce::UndoManager &um)
{
    auto mode = m_plugin->state.getPropertyAsValue(te::IDs::mode, &um).toString();
    if (mode == "highpass")
        mode = "Highpass";
    else
        mode = "Lowpass";

    m_modeLabel.setText(mode, juce::NotificationType::dontSendNotification);
}

juce::ValueTree FilterPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree FilterPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "lowpass", nullptr);
    defaultState.setProperty(te::IDs::mode, "lowpass", nullptr);
    defaultState.setProperty(te::IDs::frequency, 12000.0, nullptr);
    return defaultState;
}

void FilterPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String FilterPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String FilterPluginComponent::getPluginTypeName() const { return "lowpass"; }

ApplicationViewState &FilterPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

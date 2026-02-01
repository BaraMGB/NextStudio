/*
  ==============================================================================

    DelayPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Delay/DelayPluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"

DelayPluginComponent::DelayPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_fbParCom = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
    addAndMakeVisible(*m_fbParCom);
    m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix proportion"), "Mix");
    addAndMakeVisible(*m_mix);

    auto timeVal = m_plugin->state.getPropertyAsValue(te::IDs::length, &m_editViewState.m_edit.getUndoManager());
    if (static_cast<double>(timeVal.getValue()) < 1.0)
        timeVal = 1.0;

    m_time = std::make_unique<NonAutomatableParameterComponent>(timeVal, "Time", 1, 1000);
    addAndMakeVisible(*m_time);
    m_plugin->state.addListener(this);
}

void DelayPluginComponent::resized()
{
    auto bounds = getLocalBounds();
    auto h = bounds.getHeight() / 12;
    m_fbParCom->setBounds(bounds.removeFromTop(h * 4));
    m_mix->setBounds(bounds.removeFromTop(h * 4));
    m_time->setBounds(bounds.removeFromTop(h * 4));
}

juce::ValueTree DelayPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree DelayPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "delay", nullptr);
    return defaultState;
}

void DelayPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String DelayPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String DelayPluginComponent::getPluginTypeName() const { return "delay"; }

ApplicationViewState &DelayPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

/*
  ==============================================================================

    VolumePluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Volume/VolumePluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"

VolumePluginComponent::VolumePluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_volParComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("volume"), "Vol");
    addAndMakeVisible(*m_volParComp);
    m_panParComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("pan"), "Pan");
    addAndMakeVisible(*m_panParComp);
    m_plugin->state.addListener(this);
}

void VolumePluginComponent::paint(juce::Graphics &) {}

void VolumePluginComponent::resized()
{
    auto bounds = getLocalBounds();

    auto h = bounds.getHeight() / 12;
    bounds.removeFromTop(h);
    m_volParComp->setBounds(bounds.removeFromTop(h * 4));
    bounds.removeFromTop(h * 2);
    m_panParComp->setBounds(bounds.removeFromTop(h * 4));
}

juce::ValueTree VolumePluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree VolumePluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "volume", nullptr);
    return defaultState;
}

void VolumePluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String VolumePluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String VolumePluginComponent::getPluginTypeName() const { return "volume"; }

ApplicationViewState &VolumePluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

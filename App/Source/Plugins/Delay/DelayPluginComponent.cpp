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
    if (isNextDelay())
    {
        m_mode = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("mode"), "Mode");
        m_syncEnabled = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("syncEnabled"), "Sync");
        m_syncDivision = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("syncDivision"), "Division");

        m_time = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("timeMs"), "Time");
        m_fbParCom = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
        m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix"), "Mix");
        m_stereoOffset = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("stereoOffsetMs"), "Offset");
        m_pingPongAmount = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("pingPongAmount"), "PingPong");
        m_hpCutoff = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("hpCutoff"), "HP");
        m_lpCutoff = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("lpCutoff"), "LP");

        addAndMakeVisible(*m_mode);
        addAndMakeVisible(*m_syncEnabled);
        addAndMakeVisible(*m_syncDivision);
        addAndMakeVisible(*m_time);
        addAndMakeVisible(*m_fbParCom);
        addAndMakeVisible(*m_mix);
        addAndMakeVisible(*m_stereoOffset);
        addAndMakeVisible(*m_pingPongAmount);
        addAndMakeVisible(*m_hpCutoff);
        addAndMakeVisible(*m_lpCutoff);
    }
    else
    {
        m_fbParCom = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
        addAndMakeVisible(*m_fbParCom);
        m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix proportion"), "Mix");
        addAndMakeVisible(*m_mix);

        auto timeVal = m_plugin->state.getPropertyAsValue(te::IDs::length, &m_editViewState.m_edit.getUndoManager());
        if (static_cast<double>(timeVal.getValue()) < 1.0)
            timeVal = 1.0;

        m_legacyTime = std::make_unique<NonAutomatableParameterComponent>(timeVal, "Time", 1, 1000);
        addAndMakeVisible(*m_legacyTime);
    }

    m_plugin->state.addListener(this);
}

void DelayPluginComponent::resized()
{
    auto bounds = getLocalBounds();

    if (isNextDelay())
    {
        const int rowHeight = juce::jmax(28, bounds.getHeight() / 4);

        auto row = bounds.removeFromTop(rowHeight);
        m_mode->setBounds(row.removeFromLeft(bounds.getWidth() / 3));
        m_syncEnabled->setBounds(row.removeFromLeft(bounds.getWidth() / 2));
        m_syncDivision->setBounds(row);

        row = bounds.removeFromTop(rowHeight);
        m_time->setBounds(row.removeFromLeft(bounds.getWidth() / 3));
        m_fbParCom->setBounds(row.removeFromLeft(bounds.getWidth() / 2));
        m_mix->setBounds(row);

        row = bounds.removeFromTop(rowHeight);
        m_stereoOffset->setBounds(row.removeFromLeft(bounds.getWidth() / 3));
        m_pingPongAmount->setBounds(row.removeFromLeft(bounds.getWidth() / 2));
        m_hpCutoff->setBounds(row);

        m_lpCutoff->setBounds(bounds.removeFromTop(rowHeight));
    }
    else
    {
        auto h = bounds.getHeight() / 12;
        m_fbParCom->setBounds(bounds.removeFromTop(h * 4));
        m_mix->setBounds(bounds.removeFromTop(h * 4));
        m_legacyTime->setBounds(bounds.removeFromTop(h * 4));
    }
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
    defaultState.setProperty("type", getPluginTypeName(), nullptr);
    return defaultState;
}

void DelayPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String DelayPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String DelayPluginComponent::getPluginTypeName() const { return isNextDelay() ? NextDelayPlugin::xmlTypeName : juce::String("delay"); }

ApplicationViewState &DelayPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

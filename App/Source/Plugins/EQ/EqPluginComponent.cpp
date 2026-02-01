/*
  ==============================================================================

    EqPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/EQ/EqPluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"

EqPluginComponent::EqPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_lowFreqComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass freq"), "Freq");
    m_lowGainComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass gain"), "Gain");
    m_lowQComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass Q"), "Q");
    m_midFreq1Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid freq 1"), "Freq");
    m_midGain1Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid gain 1"), "Gain");
    m_midQ1Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid Q 1"), "Q");
    m_midFreq2Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid freq 2"), "Freq");
    m_midGain2Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid gain 2"), "Gain");
    m_midQ2Comp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid Q 2"), "Q");
    m_hiFreqComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass freq"), "Freq");
    m_hiGainComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass gain"), "Gain");
    m_hiQComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass Q"), "Q");

    addAndMakeVisible(*m_lowFreqComp);
    addAndMakeVisible(*m_lowGainComp);
    addAndMakeVisible(*m_lowQComp);
    addAndMakeVisible(*m_midFreq1Comp);
    addAndMakeVisible(*m_midGain1Comp);
    addAndMakeVisible(*m_midQ1Comp);
    addAndMakeVisible(*m_midFreq2Comp);
    addAndMakeVisible(*m_midGain2Comp);
    addAndMakeVisible(*m_midQ2Comp);
    addAndMakeVisible(*m_hiFreqComp);
    addAndMakeVisible(*m_hiGainComp);
    addAndMakeVisible(*m_hiQComp);

    m_plugin->state.addListener(this);
}

void EqPluginComponent::resized()
{
    auto area = getLocalBounds();
    auto w = getWidth() / 4;
    auto h = getHeight() / 12;
    auto low = area.removeFromLeft(w);
    auto mid1 = area.removeFromLeft(w);
    auto mid2 = area.removeFromLeft(w);
    auto hi = area.removeFromLeft(w);

    m_lowFreqComp->setBounds(low.removeFromTop(h * 4));
    m_lowGainComp->setBounds(low.removeFromTop(h * 4));
    m_lowQComp->setBounds(low.removeFromTop(h * 4));
    m_midFreq1Comp->setBounds(mid1.removeFromTop(h * 4));
    m_midGain1Comp->setBounds(mid1.removeFromTop(h * 4));
    m_midQ1Comp->setBounds(mid1.removeFromTop(h * 4));
    m_midFreq2Comp->setBounds(mid2.removeFromTop(h * 4));
    m_midGain2Comp->setBounds(mid2.removeFromTop(h * 4));
    m_midQ2Comp->setBounds(mid2.removeFromTop(h * 4));
    m_hiFreqComp->setBounds(hi.removeFromTop(h * 4));
    m_hiGainComp->setBounds(hi.removeFromTop(h * 4));
    m_hiQComp->setBounds(hi.removeFromTop(h * 4));
}

void EqPluginComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (i == te::IDs::loFreq)
        m_lowFreqComp->updateLabel();
    if (i == te::IDs::loGain)
        m_lowGainComp->updateLabel();
    if (i == te::IDs::loQ)
        m_lowQComp->updateLabel();

    if (i == te::IDs::midFreq1)
        m_midFreq1Comp->updateLabel();
    if (i == te::IDs::midGain1)
        m_midGain1Comp->updateLabel();
    if (i == te::IDs::midQ1)
        m_midQ1Comp->updateLabel();

    if (i == te::IDs::midFreq2)
        m_midFreq2Comp->updateLabel();
    if (i == te::IDs::midGain2)
        m_midGain2Comp->updateLabel();
    if (i == te::IDs::midQ2)
        m_midQ2Comp->updateLabel();

    if (i == te::IDs::hiQ)
        m_hiQComp->updateLabel();
    if (i == te::IDs::hiFreq)
        m_hiFreqComp->updateLabel();
    if (i == te::IDs::hiGain)
        m_hiGainComp->updateLabel();
}

juce::ValueTree EqPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree EqPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "4bandEq", nullptr);
    return defaultState;
}

void EqPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String EqPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String EqPluginComponent::getPluginTypeName() const { return "4bandEq"; }

ApplicationViewState &EqPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

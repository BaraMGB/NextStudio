/*
  ==============================================================================

    SimpleSynthPluginComponent.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../../PluginViewComponent.h"
#include "../../Components/AutomatableSlider.h"
#include "../../Components/AutomatableParameter.h"
#include "../../Components/AutomatableToggle.h"
#include "../../Components/AutomatableComboBox.h"
#include "../../Utilities.h"
#include "SimpleSynthPlugin.h"

//==============================================================================
class SimpleSynthOscSection : public juce::Component
{
public:
    SimpleSynthOscSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState, int oscIndex);
    ~SimpleSynthOscSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin& m_plugin;
    ApplicationViewState& m_appState;
    int m_oscIndex;
    
    AutomatableChoiceComponent m_waveComp;
    AutomatableParameterComponent m_coarseTuneComp;
    AutomatableParameterComponent m_fineTuneComp;
    
    // Osc 1 Specific
    std::unique_ptr<AutomatableParameterComponent> m_unisonOrderComp;
    std::unique_ptr<AutomatableParameterComponent> m_unisonDetuneComp;
    std::unique_ptr<AutomatableParameterComponent> m_unisonSpreadComp;
    std::unique_ptr<AutomatableToggleComponent> m_retriggerComp;

    // Osc 2 Specific
    std::unique_ptr<AutomatableParameterComponent> m_levelComp;
    
    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthOscSection)
};

//==============================================================================
class SimpleSynthMixSection : public juce::Component
{
public:
    SimpleSynthMixSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState);
    ~SimpleSynthMixSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin& m_plugin;
    ApplicationViewState& m_appState;

    AutomatableChoiceComponent m_mixModeComp;
    AutomatableParameterComponent m_crossModComp;
    
    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthMixSection)
};

//==============================================================================
class SimpleSynthFilterSection : public juce::Component
{
public:
    SimpleSynthFilterSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState);
    ~SimpleSynthFilterSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin& m_plugin;
    ApplicationViewState& m_appState;

    AutomatableChoiceComponent m_filterTypeComp;
    AutomatableParameterComponent m_cutoffComp;
    AutomatableParameterComponent m_resComp;
    AutomatableParameterComponent m_driveComp;
    AutomatableParameterComponent m_envAmountComp;
    
    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthFilterSection)
};

//==============================================================================
class SimpleSynthEnvSection : public juce::Component
{
public:
    SimpleSynthEnvSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState, const juce::String& name, bool isFilterEnv=false);
    ~SimpleSynthEnvSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SimpleSynthPlugin& m_plugin;
    ApplicationViewState& m_appState;

    AutomatableParameterComponent m_attackComp;
    AutomatableParameterComponent m_decayComp;
    AutomatableParameterComponent m_sustainComp;
    AutomatableParameterComponent m_releaseComp;
    
    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthEnvSection)
};

//==============================================================================
class SimpleSynthPluginComponent : public PluginViewComponent,
                                   private juce::ValueTree::Listener
{
public:
    SimpleSynthPluginComponent(EditViewState& evs, te::Plugin::Ptr p);
    ~SimpleSynthPluginComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    int getNeededWidth() override { return 6; } 

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree& state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState& getApplicationViewState() override;

private:
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    SimpleSynthPlugin* m_synth = nullptr;

    // Sections
    SimpleSynthOscSection m_osc1Section;
    SimpleSynthOscSection m_osc2Section;
    SimpleSynthMixSection m_mixSection;
    SimpleSynthFilterSection m_filterSection;
    SimpleSynthEnvSection m_ampEnvSection;
    SimpleSynthEnvSection m_filterEnvSection;
    
    // Master
    AutomatableSliderComponent m_levelSlider;
    juce::Label m_levelLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthPluginComponent)
};

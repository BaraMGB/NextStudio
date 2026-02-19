/*
  ==============================================================================

    SimpleSynthPluginComponent.h
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/SimpleSynth/SimpleSynthPlugin.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableEnvelopeParameter.h"
#include "UI/Controls/AutomatableParameter.h"
#include "UI/Controls/AutomatableSlider.h"
#include "UI/Controls/AutomatableToggle.h"
#include "Utilities/Utilities.h"

//==============================================================================
class SimpleSynthOscSection : public juce::Component
{
public:
    SimpleSynthOscSection(SimpleSynthPlugin &plugin, ApplicationViewState &appState, int oscIndex);
    ~SimpleSynthOscSection() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin &m_plugin;
    ApplicationViewState &m_appState;
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
    std::unique_ptr<AutomatableToggleComponent> m_enabledComp;
    std::unique_ptr<AutomatableChoiceComponent> m_mixModeComp;
    std::unique_ptr<AutomatableParameterComponent> m_crossModComp;

    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthOscSection)
};

//==============================================================================
class SimpleSynthFilterSection : public juce::Component
{
public:
    SimpleSynthFilterSection(SimpleSynthPlugin &plugin, ApplicationViewState &appState);
    ~SimpleSynthFilterSection() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin &m_plugin;
    ApplicationViewState &m_appState;

    AutomatableChoiceComponent m_filterTypeComp;
    AutomatableParameterComponent m_cutoffComp;
    AutomatableParameterComponent m_resComp;
    AutomatableParameterComponent m_driveComp;
    AutomatableParameterComponent m_envAmountComp;

    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthFilterSection)
};

//==============================================================================
class SimpleSynthEnvelopeDisplay : public juce::Component
{
public:
    SimpleSynthEnvelopeDisplay(te::AutomatableParameter::Ptr attack, te::AutomatableParameter::Ptr decay, te::AutomatableParameter::Ptr sustain, te::AutomatableParameter::Ptr release);
    ~SimpleSynthEnvelopeDisplay() override = default;

    void paint(juce::Graphics &g) override;

    void setColour(juce::Colour c)
    {
        m_colour = c;
        repaint();
    }

private:
    te::AutomatableParameter::Ptr m_attack, m_decay, m_sustain, m_release;
    juce::Colour m_colour = juce::Colours::white;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthEnvelopeDisplay)
};

//==============================================================================
class SimpleSynthEnvSection : public juce::Component
{
public:
    SimpleSynthEnvSection(SimpleSynthPlugin &plugin, ApplicationViewState &appState, const juce::String &name, bool isFilterEnv = false);
    ~SimpleSynthEnvSection() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void updateUI();

private:
    SimpleSynthPlugin &m_plugin;
    ApplicationViewState &m_appState;

    SimpleSynthEnvelopeDisplay m_display;

    AutomatableEnvelopeParameter m_attackComp;
    AutomatableEnvelopeParameter m_decayComp;
    AutomatableEnvelopeParameter m_sustainComp;
    AutomatableEnvelopeParameter m_releaseComp;

    juce::Label m_nameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthEnvSection)
};

//==============================================================================
class SimpleSynthPluginComponent
    : public PluginViewComponent
    , private juce::ValueTree::Listener
{
public:
    SimpleSynthPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~SimpleSynthPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int getNeededWidth() override { return 7; }

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}

    SimpleSynthPlugin *m_synth = nullptr;

    // Sections
    SimpleSynthOscSection m_osc1Section;
    SimpleSynthOscSection m_osc2Section;
    SimpleSynthFilterSection m_filterSection;
    SimpleSynthEnvSection m_ampEnvSection;
    SimpleSynthEnvSection m_filterEnvSection;

    // Master
    AutomatableSliderComponent m_levelSlider;
    juce::Label m_levelLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthPluginComponent)
};

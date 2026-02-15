/*
  ==============================================================================

    CompressorPluginComponent.h
    Created: 11 Feb 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

#include <vector>

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "UI/Controls/AutomatableParameter.h"

namespace te = tracktion_engine;

class CompressorTransferGraphComponent : public juce::Component
{
public:
    CompressorTransferGraphComponent(te::AutomatableParameter::Ptr thresholdParam, te::AutomatableParameter::Ptr ratioParam, te::AutomatableParameter::Ptr outputParam, ApplicationViewState &appState, juce::Colour headerColour);

    void paint(juce::Graphics &g) override;

private:
    te::AutomatableParameter::Ptr m_thresholdParam;
    te::AutomatableParameter::Ptr m_ratioParam;
    te::AutomatableParameter::Ptr m_outputParam;
    ApplicationViewState &m_appState;
    juce::Colour m_headerColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorTransferGraphComponent)
};

class CompressorPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    CompressorPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~CompressorPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    int getNeededWidth() override { return 2; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void refreshSidechainSourceList();
    void syncSidechainTriggerButton();

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    te::AutomatableParameter::Ptr m_thresholdParam;
    te::AutomatableParameter::Ptr m_ratioParam;
    te::AutomatableParameter::Ptr m_attackParam;
    te::AutomatableParameter::Ptr m_releaseParam;
    te::AutomatableParameter::Ptr m_outputParam;

    CompressorTransferGraphComponent m_transferGraph;

    std::unique_ptr<AutomatableParameterComponent> m_thresholdComp;
    std::unique_ptr<AutomatableParameterComponent> m_ratioComp;
    std::unique_ptr<AutomatableParameterComponent> m_attackComp;
    std::unique_ptr<AutomatableParameterComponent> m_releaseComp;
    std::unique_ptr<AutomatableParameterComponent> m_outputComp;
    std::unique_ptr<AutomatableParameterComponent> m_sidechainGainComp;

    juce::Label m_sidechainSourceLabel;
    juce::ComboBox m_sidechainSourceCombo;
    juce::ToggleButton m_sidechainTriggerButton;
    std::vector<te::EditItemID> m_sidechainSourceIDs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorPluginComponent)
};

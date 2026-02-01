/*
  ==============================================================================

    EqPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Utilities/EditViewState.h"
#include "UI/Controls/AutomatableParameter.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class EqPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    EqPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~EqPluginComponent() override { m_plugin->state.removeListener(this); }

    void paint(juce::Graphics &) override {}
    void resized() override;
    int getNeededWidth() override { return 4; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_lowFreqComp, m_lowGainComp, m_lowQComp;
    std::unique_ptr<AutomatableParameterComponent> m_midFreq1Comp, m_midGain1Comp, m_midQ1Comp;
    std::unique_ptr<AutomatableParameterComponent> m_midFreq2Comp, m_midGain2Comp, m_midQ2Comp;
    std::unique_ptr<AutomatableParameterComponent> m_hiFreqComp, m_hiGainComp, m_hiQComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqPluginComponent)
};

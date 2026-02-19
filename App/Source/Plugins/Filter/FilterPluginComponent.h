/*
  ==============================================================================

    FilterPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/Filter/NextFilterPlugin.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableParameter.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class FilterPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    FilterPluginComponent(EditViewState &, te::Plugin::Ptr);
    ~FilterPluginComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    int getNeededWidth() override { return 2; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    class FilterTransferGraphComponent;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_freqPar;
    std::unique_ptr<AutomatableParameterComponent> m_resonancePar;
    std::unique_ptr<AutomatableChoiceComponent> m_modePar;
    std::unique_ptr<AutomatableChoiceComponent> m_slopePar;
    std::unique_ptr<FilterTransferGraphComponent> m_graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPluginComponent)
};

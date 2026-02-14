/*
  ==============================================================================

    ReverbPluginComponent.h
    Created: 14 Feb 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableParameter.h"
#include "Utilities/EditViewState.h"

namespace te = tracktion_engine;

class ReverbPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    ReverbPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~ReverbPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    int getNeededWidth() override { return 3; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    class ReverbSpaceGraphComponent;

    void valueTreeChanged() override;
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    std::unique_ptr<AutomatableParameterComponent> m_roomSize, m_damping, m_wet, m_dry, m_width;
    std::unique_ptr<AutomatableChoiceComponent> m_mode;
    std::unique_ptr<ReverbSpaceGraphComponent> m_graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbPluginComponent)
};

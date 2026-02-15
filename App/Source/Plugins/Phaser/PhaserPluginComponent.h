#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/Phaser/NextPhaserPlugin.h"
#include "UI/Controls/AutomatableParameter.h"
#include "Utilities/EditViewState.h"

namespace te = tracktion_engine;

class PhaserPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    PhaserPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~PhaserPluginComponent() override;

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
    class PhaserSweepGraphComponent;

    void valueTreeChanged() override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    std::unique_ptr<AutomatableParameterComponent> m_depth;
    std::unique_ptr<AutomatableParameterComponent> m_rate;
    std::unique_ptr<AutomatableParameterComponent> m_feedback;
    std::unique_ptr<AutomatableParameterComponent> m_mix;
    std::unique_ptr<PhaserSweepGraphComponent> m_graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserPluginComponent)
};

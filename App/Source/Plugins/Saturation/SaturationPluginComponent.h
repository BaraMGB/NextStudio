#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/Saturation/NextSaturationPlugin.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableParameter.h"
#include "UI/LevelMeterComponent.h"
#include "Utilities/EditViewState.h"

namespace te = tracktion_engine;

class SaturationPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    SaturationPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~SaturationPluginComponent() override;

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
    class SaturationTransferGraphComponent;

    void valueTreeChanged() override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    NextSaturationPlugin *m_saturationPlugin = nullptr;

    std::unique_ptr<AutomatableParameterComponent> m_input;
    std::unique_ptr<AutomatableParameterComponent> m_drive;
    std::unique_ptr<AutomatableParameterComponent> m_tone;
    std::unique_ptr<AutomatableParameterComponent> m_mix;
    std::unique_ptr<AutomatableParameterComponent> m_output;
    std::unique_ptr<AutomatableParameterComponent> m_bias;
    std::unique_ptr<AutomatableChoiceComponent> m_mode;
    std::unique_ptr<AutomatableChoiceComponent> m_quality;

    std::unique_ptr<LevelMeterComponent> m_inMeter;
    std::unique_ptr<LevelMeterComponent> m_outMeter;
    juce::Label m_inLabel;
    juce::Label m_outLabel;

    std::unique_ptr<SaturationTransferGraphComponent> m_graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationPluginComponent)
};

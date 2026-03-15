#pragma once

#include <JuceHeader.h>

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/PeakLimiter/PeakLimiterPlugin.h"
#include "UI/Controls/AutomatableParameter.h"
#include "UI/Controls/AutomatableToggle.h"

namespace te = tracktion_engine;

class PeakLimiterPluginComponent
    : public PluginViewComponent
    , private juce::Timer
{
public:
    PeakLimiterPluginComponent(EditViewState &, te::Plugin::Ptr);
    ~PeakLimiterPluginComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    int getNeededWidth() override { return 3; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    class MeterComponent;

    void timerCallback() override;

    PeakLimiterPlugin *m_peakLimiter = nullptr;

    std::unique_ptr<AutomatableParameterComponent> m_inputGainComp;
    std::unique_ptr<AutomatableParameterComponent> m_ceilingComp;
    std::unique_ptr<AutomatableParameterComponent> m_releaseComp;
    std::unique_ptr<AutomatableToggleComponent> m_linkChannelsComp;

    juce::Label m_inputMeterLabel;
    juce::Label m_outputMeterLabel;
    juce::Label m_gainReductionLabel;
    std::unique_ptr<MeterComponent> m_meter;

    float m_inputPeakDb = -100.0f;
    float m_outputPeakDb = -100.0f;
    float m_gainReductionDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakLimiterPluginComponent)
};

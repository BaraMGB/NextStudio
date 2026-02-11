#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPlugin.h"

#include <array>
#include <vector>

class SpectrumAnalyzerPluginComponent
    : public PluginViewComponent
    , private juce::Timer
{
public:
    SpectrumAnalyzerPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~SpectrumAnalyzerPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    int getNeededWidth() override { return 4; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void timerCallback() override;
    void updateTimerState();
    SpectrumAnalyzerPlugin *getAnalyzer() const noexcept;
    float xForFrequency(juce::Rectangle<float> area, double frequency, double sampleRate) const;
    float yForDb(juce::Rectangle<float> area, float db) const;

    std::array<float, SpectrumAnalyzerPlugin::numDisplayBins> m_spectrum{};
    std::vector<float> m_sampledY;
    int m_pointCount = 0;
    juce::Label m_titleLabel;
    bool m_timerRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerPluginComponent)
};

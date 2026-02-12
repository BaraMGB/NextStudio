/*
  ==============================================================================

    EqPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
#include <array>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class EqResponseGraphComponent : public juce::Component
{
public:
    EqResponseGraphComponent(te::Plugin::Ptr plugin, te::AutomatableParameter::Ptr lowFreq, te::AutomatableParameter::Ptr lowGain, te::AutomatableParameter::Ptr lowQ, te::AutomatableParameter::Ptr midFreq1, te::AutomatableParameter::Ptr midGain1, te::AutomatableParameter::Ptr midQ1, te::AutomatableParameter::Ptr midFreq2, te::AutomatableParameter::Ptr midGain2, te::AutomatableParameter::Ptr midQ2, te::AutomatableParameter::Ptr highFreq, te::AutomatableParameter::Ptr highGain, te::AutomatableParameter::Ptr highQ);

    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;

private:
    struct BandHandle
    {
        te::AutomatableParameter::Ptr freq;
        te::AutomatableParameter::Ptr gain;
        te::AutomatableParameter::Ptr q;
    };

    float xForFrequency(const juce::Rectangle<float> &, float frequency) const;
    float yForGainDb(const juce::Rectangle<float> &, float db) const;
    float frequencyForX(const juce::Rectangle<float> &, float x) const;
    float gainDbForY(const juce::Rectangle<float> &, float y) const;
    juce::Rectangle<float> getPlotArea() const;
    int getBandIndexAtPosition(juce::Point<float>) const;

    te::Plugin::Ptr m_plugin;
    std::array<BandHandle, 4> m_bands;
    int m_dragBandIndex = -1;
    int m_hoverBandIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqResponseGraphComponent)
};

class EqPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    EqPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~EqPluginComponent() override { m_plugin->state.removeListener(this); }

    void paint(juce::Graphics &) override {}
    void resized() override;
    int getNeededWidth() override { return 5; }

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

    te::AutomatableParameter::Ptr m_lowFreqParam, m_lowGainParam, m_lowQParam;
    te::AutomatableParameter::Ptr m_midFreq1Param, m_midGain1Param, m_midQ1Param;
    te::AutomatableParameter::Ptr m_midFreq2Param, m_midGain2Param, m_midQ2Param;
    te::AutomatableParameter::Ptr m_hiFreqParam, m_hiGainParam, m_hiQParam;

    EqResponseGraphComponent m_responseGraph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqPluginComponent)
};

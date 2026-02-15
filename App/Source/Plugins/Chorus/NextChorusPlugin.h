#pragma once

#include <JuceHeader.h>

#include <atomic>

namespace te = tracktion_engine;

class NextChorusPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "next_chorus";
    static const char *getPluginName() { return "Chorus"; }
    static constexpr const char *depthMsParamID = "depthMs";
    static constexpr const char *speedHzParamID = "speedHz";
    static constexpr const char *widthParamID = "width";
    static constexpr const char *mixProportionParamID = "mixProportion";

    NextChorusPlugin(te::PluginCreationInfo info);
    ~NextChorusPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Next Chorus Plugin"; }

    bool isSynth() override { return false; }
    bool takesMidiInput() override { return false; }
    bool takesAudioInput() override { return true; }
    bool producesAudioWhenNoAudioInput() override { return false; }

    int getNumOutputChannelsGivenInputs(int numInputChannels) override { return juce::jmin(numInputChannels, 2); }

    void initialise(const te::PluginInitialisationInfo &) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const te::PluginRenderContext &) override;
    void midiPanic() override;
    void restorePluginStateFromValueTree(const juce::ValueTree &v) override;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

private:
    struct AudioParams
    {
        std::atomic<float> depthMs{3.0f};
        std::atomic<float> speedHz{1.0f};
        std::atomic<float> width{0.5f};
        std::atomic<float> mixProportion{0.5f};
    } m_audioParams;

    static constexpr int maxChannels = 2;
    static constexpr float minDepthMs = 0.0f;
    static constexpr float maxDepthMs = 30.0f;
    static constexpr float minSpeedHz = 0.02f;
    static constexpr float maxSpeedHz = 10.0f;
    static constexpr float baseDelayMs = 20.0f;

    void updateAtomics();

    te::AutomatableParameter::Ptr m_depthMsParam;
    te::AutomatableParameter::Ptr m_speedHzParam;
    te::AutomatableParameter::Ptr m_widthParam;
    te::AutomatableParameter::Ptr m_mixProportionParam;

    juce::CachedValue<float> m_depthMsValue;
    juce::CachedValue<float> m_speedHzValue;
    juce::CachedValue<float> m_widthValue;
    juce::CachedValue<float> m_mixProportionValue;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> m_delayLine;
    juce::LinearSmoothedValue<float> m_depthMsSmoothed;
    juce::LinearSmoothedValue<float> m_speedHzSmoothed;
    juce::LinearSmoothedValue<float> m_widthSmoothed;
    juce::LinearSmoothedValue<float> m_mixSmoothed;

    float m_phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextChorusPlugin)
};

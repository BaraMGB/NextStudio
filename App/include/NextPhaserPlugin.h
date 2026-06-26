#pragma once

#include <JuceHeader.h>

#include <atomic>

namespace te = tracktion_engine;

class NextPhaserPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "next_phaser";
    static const char *getPluginName() { return "Phaser"; }

    static constexpr const char *depthParamID = "depth";
    static constexpr const char *rateParamID = "rate";
    static constexpr const char *feedbackParamID = "feedback";
    static constexpr const char *mixParamID = "mixProportion";

    NextPhaserPlugin(te::PluginCreationInfo info);
    ~NextPhaserPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Next Phaser Plugin"; }

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
    void restorePluginStateFromValueTree(const juce::ValueTree &) override;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

private:
    struct AudioParams
    {
        std::atomic<float> depth{0.55f};
        std::atomic<float> rateHz{0.45f};
        std::atomic<float> feedback{0.35f};
        std::atomic<float> mix{0.6f};
    } m_audioParams;

    static constexpr float minDepth = 0.0f;
    static constexpr float maxDepth = 1.0f;
    static constexpr float minRateHz = 0.02f;
    static constexpr float maxRateHz = 10.0f;
    static constexpr float minFeedback = -0.95f;
    static constexpr float maxFeedback = 0.95f;
    static constexpr float minMix = 0.0f;
    static constexpr float maxMix = 1.0f;

    void updateAtomics();

    te::AutomatableParameter::Ptr m_depthParam;
    te::AutomatableParameter::Ptr m_rateParam;
    te::AutomatableParameter::Ptr m_feedbackParam;
    te::AutomatableParameter::Ptr m_mixParam;

    juce::CachedValue<float> m_depthValue;
    juce::CachedValue<float> m_rateValue;
    juce::CachedValue<float> m_feedbackValue;
    juce::CachedValue<float> m_mixValue;

    juce::dsp::Phaser<float> m_phaser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextPhaserPlugin)
};

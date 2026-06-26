#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace te = tracktion_engine;

class PeakLimiterPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "peak_limiter";
    static const char *getPluginName() { return "Peak Limiter"; }

    static constexpr const char *inputGainParamID = "inputGainDb";
    static constexpr const char *ceilingParamID = "ceilingDb";
    static constexpr const char *releaseParamID = "releaseMs";
    static constexpr const char *linkChannelsParamID = "linkChannels";
    static constexpr float fixedLookAheadMs = 3.0f;

    PeakLimiterPlugin(te::PluginCreationInfo);
    ~PeakLimiterPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Peak Limiter Plugin"; }

    bool isSynth() override { return false; }
    bool takesMidiInput() override { return false; }
    bool takesAudioInput() override { return true; }
    bool producesAudioWhenNoAudioInput() override { return false; }
    int getNumOutputChannelsGivenInputs(int numInputChannels) override { return numInputChannels > 0 ? juce::jmin(numInputChannels, 2) : 2; }
    double getLatencySeconds() override;

    void initialise(const te::PluginInitialisationInfo &) override;
    void deinitialise() override;
    void reset() override;
    void midiPanic() override;
    void applyToBuffer(const te::PluginRenderContext &) override;
    void restorePluginStateFromValueTree(const juce::ValueTree &) override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

    float getInputPeakDb() const { return m_inputPeakDb.load(std::memory_order_relaxed); }
    float getOutputPeakDb() const { return m_outputPeakDb.load(std::memory_order_relaxed); }
    float getGainReductionDb() const { return m_gainReductionDb.load(std::memory_order_relaxed); }

private:
    struct AudioParams
    {
        std::atomic<float> inputGainDb{0.0f};
        std::atomic<float> ceilingDb{-0.3f};
        std::atomic<float> releaseMs{80.0f};
        std::atomic<int> linkChannels{1};
    } m_audioParams;

    struct SlidingMaxQueue
    {
        void prepare(int capacity);
        void reset();
        void push(int64_t index, float value);
        void prune(int64_t minIndex);
        float getMax() const;

    private:
        int positionFor(int64_t logicalIndex) const;

        std::vector<float> values;
        std::vector<int64_t> indices;
        int64_t head = 0;
        int64_t tail = 0;
    };

    void updateAtomics();
    void updateDerivedParameters();
    int calculateLookAheadSamples(float lookAheadMs) const;
    void pushDetectionSample(int channel, int64_t sampleIndex, float absolutePeak);
    void updateGainFromDetectors(bool linkedChannels, int numChannels);
    float computeReleaseCoeff(float releaseMs, double sampleRate) const;

    te::AutomatableParameter::Ptr m_inputGainParam;
    te::AutomatableParameter::Ptr m_ceilingParam;
    te::AutomatableParameter::Ptr m_releaseParam;
    te::AutomatableParameter::Ptr m_linkChannelsParam;

    juce::CachedValue<float> m_inputGainValue;
    juce::CachedValue<float> m_ceilingValue;
    juce::CachedValue<float> m_releaseValue;
    juce::CachedValue<float> m_linkChannelsValue;

    double m_sampleRate = 44100.0;
    int m_delayBufferSize = 0;
    int m_writePos = 0;
    int m_lookAheadSamples = 0;
    int64_t m_sampleCounter = 0;
    bool m_wasEnabled = true;
    float m_inputGainLinear = 1.0f;
    float m_ceilingLinear = 1.0f;
    float m_releaseCoeff = 0.0f;

    static constexpr int maxChannels = 2;
    static constexpr int delaySafetySamples = 64;

    std::array<float, maxChannels> m_currentGain{1.0f, 1.0f};
    std::array<std::vector<float>, maxChannels> m_delayBuffers;
    std::array<SlidingMaxQueue, maxChannels> m_peakQueues;

    std::atomic<float> m_inputPeakDb{-100.0f};
    std::atomic<float> m_outputPeakDb{-100.0f};
    std::atomic<float> m_gainReductionDb{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakLimiterPlugin)
};

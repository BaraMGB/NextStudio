#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>

namespace te = tracktion_engine;

class NextFilterPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "next_filter";
    static const char *getPluginName() { return "Filter"; }

    static constexpr const char *frequencyParamID = "frequency";
    static constexpr const char *resonanceParamID = "resonance";
    static constexpr const char *modeParamID = "mode";
    static constexpr const char *slopeParamID = "slope";

    enum Mode
    {
        lowpass = 0,
        highpass = 1
    };

    enum Slope
    {
        slope12 = 0,
        slope24 = 1,
        slope36 = 2,
        slope48 = 3
    };

    NextFilterPlugin(te::PluginCreationInfo);
    ~NextFilterPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Next Filter Plugin"; }

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
        std::atomic<float> frequency{1400.0f};
        std::atomic<float> resonance{0.71f};
        std::atomic<int> mode{(int)lowpass};
        std::atomic<int> slope{(int)slope24};
    } m_audioParams;

    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;
    static constexpr float minResonance = 0.3f;
    static constexpr float maxResonance = 6.0f;

    static juce::String modeToText(int);
    static int textToMode(const juce::String &);
    static juce::String slopeToText(int);
    static int textToSlope(const juce::String &);
    static int slopeToStageCount(int);

    void updateAtomics();

    te::AutomatableParameter::Ptr m_freqParam;
    te::AutomatableParameter::Ptr m_resParam;
    te::AutomatableParameter::Ptr m_modeParam;
    te::AutomatableParameter::Ptr m_slopeParam;

    juce::CachedValue<float> m_freqValue;
    juce::CachedValue<float> m_resValue;
    juce::CachedValue<float> m_modeValue;
    juce::CachedValue<float> m_slopeValue;

    static constexpr int maxChannels = 2;
    static constexpr int maxStages = 4;
    std::array<std::array<juce::dsp::StateVariableTPTFilter<float>, maxStages>, maxChannels> m_filters;

    juce::LinearSmoothedValue<float> m_freqSmoothed;
    juce::LinearSmoothedValue<float> m_resSmoothed;
    int m_lastAppliedMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextFilterPlugin)
};

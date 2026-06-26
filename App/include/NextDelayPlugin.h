#pragma once

#include <JuceHeader.h>

namespace te = tracktion_engine;

class NextDelayPlugin : public te::Plugin
{
public:
    NextDelayPlugin(te::PluginCreationInfo info);
    ~NextDelayPlugin() override;

    static constexpr const char *xmlTypeName = "next_delay";
    static const char *getPluginName() { return "Delay"; }

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Next Delay Plugin"; }

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

    enum class DelayMode : int
    {
        mono = 0,
        stereo,
        pingPong,
        dual
    };

    te::AutomatableParameter::Ptr modeParam;
    te::AutomatableParameter::Ptr syncEnabledParam;
    te::AutomatableParameter::Ptr syncDivisionParam;
    te::AutomatableParameter::Ptr timeMsParam;
    te::AutomatableParameter::Ptr feedbackParam;
    te::AutomatableParameter::Ptr mixParam;
    te::AutomatableParameter::Ptr stereoOffsetParam;
    te::AutomatableParameter::Ptr pingPongAmountParam;
    te::AutomatableParameter::Ptr hpCutoffParam;
    te::AutomatableParameter::Ptr lpCutoffParam;

    juce::CachedValue<float> modeValue;
    juce::CachedValue<float> syncEnabledValue;
    juce::CachedValue<float> syncDivisionValue;
    juce::CachedValue<float> timeMsValue;
    juce::CachedValue<float> feedbackValue;
    juce::CachedValue<float> mixValue;
    juce::CachedValue<float> stereoOffsetValue;
    juce::CachedValue<float> pingPongAmountValue;
    juce::CachedValue<float> hpCutoffValue;
    juce::CachedValue<float> lpCutoffValue;

private:
    struct AudioParams
    {
        std::atomic<float> mode{0.0f};
        std::atomic<float> syncEnabled{0.0f};
        std::atomic<float> syncDivision{3.0f};
        std::atomic<float> timeMs{250.0f};
        std::atomic<float> feedback{0.35f};
        std::atomic<float> mix{0.25f};
        std::atomic<float> stereoOffsetMs{15.0f};
        std::atomic<float> pingPongAmount{1.0f};
        std::atomic<float> hpCutoff{20.0f};
        std::atomic<float> lpCutoff{18000.0f};
    } audioParams;

    static constexpr int maxChannels = 2;
    static constexpr float minDelayMs = 20.0f;
    static constexpr float maxDelayMs = 2000.0f;
    static constexpr float maxStereoOffsetMs = 100.0f;
    static constexpr float maxFeedback = 0.95f;

    void updateAtomics();
    void updateFilterCutoffs(float hpCutoffHz, float lpCutoffHz);
    float getSyncedDelayMs(int divisionIndex, double bpm) const;
    static float getDivisionInQuarterNotes(int divisionIndex);

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> m_delayLine;
    juce::dsp::StateVariableTPTFilter<float> m_hpFilters[maxChannels];
    juce::dsp::StateVariableTPTFilter<float> m_lpFilters[maxChannels];

    juce::LinearSmoothedValue<float> m_leftDelaySamples;
    juce::LinearSmoothedValue<float> m_rightDelaySamples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextDelayPlugin)
};

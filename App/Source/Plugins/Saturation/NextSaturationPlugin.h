#pragma once

#include <JuceHeader.h>

#include <atomic>

namespace te = tracktion_engine;

class NextSaturationPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "next_saturation";
    static const char *getPluginName() { return "Saturation"; }

    static constexpr const char *inputParamID = "input";
    static constexpr const char *driveParamID = "drive";
    static constexpr const char *mixParamID = "mix";
    static constexpr const char *outputParamID = "output";
    static constexpr const char *toneParamID = "tone";
    static constexpr const char *biasParamID = "bias";
    static constexpr const char *modeParamID = "mode";
    static constexpr const char *qualityParamID = "quality";

    enum Mode
    {
        softClip = 0,
        smooth = 1,
        hardClip = 2
    };

    enum Quality
    {
        quality1x = 0,
        quality2x = 1,
        quality4x = 2
    };

    NextSaturationPlugin(te::PluginCreationInfo info);
    ~NextSaturationPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return "Next Saturation Plugin"; }

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

    te::LevelMeasurer *getInputLevelMeasurer() { return &m_inputMeasurer; }
    te::LevelMeasurer *getOutputLevelMeasurer() { return &m_outputMeasurer; }

private:
    enum class TransitionStage
    {
        none,
        fadeOut,
        fadeIn
    };

    struct AudioParams
    {
        std::atomic<float> inputDb{0.0f};
        std::atomic<float> driveDb{12.0f};
        std::atomic<float> mix{1.0f};
        std::atomic<float> outputDb{0.0f};
        std::atomic<float> tone{0.65f};
        std::atomic<float> bias{0.0f};
        std::atomic<int> mode{(int)softClip};
        std::atomic<int> quality{(int)quality2x};
    } m_audioParams;

    static constexpr float minInputDb = -24.0f;
    static constexpr float maxInputDb = 24.0f;
    static constexpr float minDriveDb = 0.0f;
    static constexpr float maxDriveDb = 36.0f;
    static constexpr float minMix = 0.0f;
    static constexpr float maxMix = 1.0f;
    static constexpr float minOutputDb = -24.0f;
    static constexpr float maxOutputDb = 24.0f;
    static constexpr float minTone = 0.0f;
    static constexpr float maxTone = 1.0f;
    static constexpr float minBias = -1.0f;
    static constexpr float maxBias = 1.0f;

    void updateAtomics();
    static juce::String modeToText(int modeValue);
    static juce::String qualityToText(int qualityValue);
    static int textToMode(const juce::String &text);
    static int textToQuality(const juce::String &text);
    static float saturateSample(float x, int modeValue);

    void processSaturationBlock(juce::dsp::AudioBlock<float> block, juce::dsp::StateVariableTPTFilter<float> &toneFilter, float inputGainStart, float inputGainEnd, float driveGainStart, float driveGainEnd, float biasStart, float biasEnd, float toneStart, float toneEnd, int modeValue);

    te::AutomatableParameter::Ptr m_inputParam;
    te::AutomatableParameter::Ptr m_driveParam;
    te::AutomatableParameter::Ptr m_mixParam;
    te::AutomatableParameter::Ptr m_outputParam;
    te::AutomatableParameter::Ptr m_toneParam;
    te::AutomatableParameter::Ptr m_biasParam;
    te::AutomatableParameter::Ptr m_modeParam;
    te::AutomatableParameter::Ptr m_qualityParam;

    juce::CachedValue<float> m_inputValue;
    juce::CachedValue<float> m_driveValue;
    juce::CachedValue<float> m_mixValue;
    juce::CachedValue<float> m_outputValue;
    juce::CachedValue<float> m_toneValue;
    juce::CachedValue<float> m_biasValue;
    juce::CachedValue<float> m_modeValue;
    juce::CachedValue<float> m_qualityValue;

    te::LevelMeasurer m_inputMeasurer;
    te::LevelMeasurer m_outputMeasurer;

    juce::dsp::Oversampling<float> m_oversampling2x;
    juce::dsp::Oversampling<float> m_oversampling4x;
    juce::dsp::StateVariableTPTFilter<float> m_toneFilter1x;
    juce::dsp::StateVariableTPTFilter<float> m_toneFilter2x;
    juce::dsp::StateVariableTPTFilter<float> m_toneFilter4x;

    juce::AudioBuffer<float> m_dryBuffer;

    juce::LinearSmoothedValue<float> m_inputGainSmoothed;
    juce::LinearSmoothedValue<float> m_driveGainSmoothed;
    juce::LinearSmoothedValue<float> m_mixSmoothed;
    juce::LinearSmoothedValue<float> m_outputGainSmoothed;
    juce::LinearSmoothedValue<float> m_toneSmoothed;
    juce::LinearSmoothedValue<float> m_biasSmoothed;

    int m_activeMode = softClip;
    int m_activeQuality = quality2x;
    int m_targetMode = softClip;
    int m_targetQuality = quality2x;
    TransitionStage m_transitionStage = TransitionStage::none;
    int m_transitionLengthSamples = 256;
    int m_transitionSamplesRemaining = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextSaturationPlugin)
};

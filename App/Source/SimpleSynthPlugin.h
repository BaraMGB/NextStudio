#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class SimpleSynthPlugin : public te::Plugin
{
public:
    SimpleSynthPlugin(te::PluginCreationInfo info);
    ~SimpleSynthPlugin() override;

    //==============================================================================
    static constexpr const char* xmlTypeName = "simple_synth";
    static const char* getPluginName() { return "Simple Synth"; }

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return getPluginName(); }
    
    bool isSynth() override { return true; }
    bool takesMidiInput() override { return true; }
    bool takesAudioInput() override { return false; }
    bool producesAudioWhenNoAudioInput() override { return true; }

    void initialise(const te::PluginInitialisationInfo&) override;
    void deinitialise() override;
    void applyToBuffer(const te::PluginRenderContext&) override;

    //==============================================================================
    void restorePluginStateFromValueTree(const juce::ValueTree& v) override;

    enum Waveform
    {
        sine = 0,
        triangle,
        saw,
        square,
        noise,
        numWaveforms
    };

    // Parameters
    te::AutomatableParameter::Ptr levelParam;
    te::AutomatableParameter::Ptr tuneParam;
    te::AutomatableParameter::Ptr waveParam;
    te::AutomatableParameter::Ptr attackParam;
    te::AutomatableParameter::Ptr decayParam;
    te::AutomatableParameter::Ptr sustainParam;
    te::AutomatableParameter::Ptr releaseParam;

    juce::CachedValue<float> levelValue;
    juce::CachedValue<float> tuneValue;
    juce::CachedValue<float> waveValue;
    juce::CachedValue<float> attackValue;
    juce::CachedValue<float> decayValue;
    juce::CachedValue<float> sustainValue;
    juce::CachedValue<float> releaseValue;

private:
    struct Voice
    {
        void start(int note, float velocity, float sampleRate, const juce::ADSR::Parameters& params);
        void stop();
        
        bool active = false;
        bool isKeyDown = false;
        int currentNote = -1;
        float currentVelocity = 0.0f;
        float phase = 0.0f;
        float phaseDelta = 0.0f;
        float targetFrequency = 0.0f;
        float sampleRate = 44100.0f;
        
        juce::ADSR adsr;
        
        // For Noise
        juce::Random random;
    };

    static constexpr int numVoices = 8;
    Voice voices[numVoices];
    
    juce::LinearSmoothedValue<float> masterLevelSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthPlugin)
};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class SimpleSynthPlugin : public te::Plugin
{
public:
    SimpleSynthPlugin(te::PluginCreationInfo info);
    ~SimpleSynthPlugin() override;

    //==============================================================================
    static constexpr const char *xmlTypeName = "simple_synth";
    static const char *getPluginName() { return "Simple Synth"; }

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return getPluginName(); }

    bool isSynth() override { return true; }
    bool takesMidiInput() override { return true; }
    bool takesAudioInput() override { return false; }
    bool producesAudioWhenNoAudioInput() override { return true; }

    void getChannelNames(juce::StringArray *ins, juce::StringArray *outs) override;
    int getNumOutputChannelsGivenInputs(int numInputChannels) override;

    void initialise(const te::PluginInitialisationInfo &) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const te::PluginRenderContext &) override;
    void midiPanic() override;

    //==============================================================================
    void restorePluginStateFromValueTree(const juce::ValueTree &v) override;

    // Constants
    static constexpr float defaultSampleRate = 44100.0f;
    static constexpr float referenceFrequency = 440.0f; // A4 reference frequency
    static constexpr float midiNoteA4 = 69.0f;          // MIDI note number for A4
    static constexpr float maxFilterSweepSemitones = 60.0f;
    static constexpr float svfBaseQ = 0.7071f;
    static constexpr float levelSmoothingTime = 0.02f;
    static constexpr float cutoffSmoothingTime = 0.05f;

    enum FilterType
    {
        ladder = 0,
        svf,
        numFilterTypes
    };

    enum Waveform
    {
        sine = 0,
        triangle,
        saw,
        square,
        noise,
        numWaveforms
    };

    enum MixMode
    {
        mix = 0,
        ringMod,
        fm,
        hardSync,
        numMixModes
    };

    // Parameters (Automatable)
    te::AutomatableParameter::Ptr levelParam;
    te::AutomatableParameter::Ptr coarseTuneParam;
    te::AutomatableParameter::Ptr fineTuneParam;
    te::AutomatableParameter::Ptr waveParam;
    te::AutomatableParameter::Ptr attackParam;
    te::AutomatableParameter::Ptr decayParam;
    te::AutomatableParameter::Ptr sustainParam;
    te::AutomatableParameter::Ptr releaseParam;
    te::AutomatableParameter::Ptr unisonOrderParam;
    te::AutomatableParameter::Ptr unisonDetuneParam;
    te::AutomatableParameter::Ptr unisonSpreadParam;
    te::AutomatableParameter::Ptr retriggerParam;
    te::AutomatableParameter::Ptr filterTypeParam;
    te::AutomatableParameter::Ptr filterCutoffParam;
    te::AutomatableParameter::Ptr filterResParam;
    te::AutomatableParameter::Ptr filterDriveParam;
    te::AutomatableParameter::Ptr filterEnvAmountParam;
    te::AutomatableParameter::Ptr filterAttackParam;
    te::AutomatableParameter::Ptr filterDecayParam;
    te::AutomatableParameter::Ptr filterSustainParam;
    te::AutomatableParameter::Ptr filterReleaseParam;

    te::AutomatableParameter::Ptr osc2WaveParam;
    te::AutomatableParameter::Ptr osc2EnabledParam;
    te::AutomatableParameter::Ptr osc2CoarseParam;
    te::AutomatableParameter::Ptr osc2FineParam;
    te::AutomatableParameter::Ptr osc2LevelParam;
    te::AutomatableParameter::Ptr mixModeParam;
    te::AutomatableParameter::Ptr crossModAmountParam;

    // State Persistence (Message Thread only)
    juce::CachedValue<float> levelValue;
    juce::CachedValue<float> coarseTuneValue;
    juce::CachedValue<float> fineTuneValue;
    juce::CachedValue<float> waveValue;
    juce::CachedValue<float> attackValue;
    juce::CachedValue<float> decayValue;
    juce::CachedValue<float> sustainValue;
    juce::CachedValue<float> releaseValue;
    juce::CachedValue<float> unisonOrderValue;
    juce::CachedValue<float> unisonDetuneValue;
    juce::CachedValue<float> unisonSpreadValue;
    juce::CachedValue<float> retriggerValue;
    juce::CachedValue<float> filterTypeValue;
    juce::CachedValue<float> filterCutoffValue;
    juce::CachedValue<float> filterResValue;
    juce::CachedValue<float> filterDriveValue;
    juce::CachedValue<float> filterEnvAmountValue;
    juce::CachedValue<float> filterAttackValue;
    juce::CachedValue<float> filterDecayValue;
    juce::CachedValue<float> filterSustainValue;
    juce::CachedValue<float> filterReleaseValue;

    juce::CachedValue<float> osc2EnabledValue;
    juce::CachedValue<float> osc2WaveValue;
    juce::CachedValue<float> osc2CoarseValue;
    juce::CachedValue<float> osc2FineValue;
    juce::CachedValue<float> osc2LevelValue;
    juce::CachedValue<float> mixModeValue;
    juce::CachedValue<float> crossModAmountValue;

private:
    struct Voice
    {
        void start(int note, float velocity, float sampleRate, float startCutoff, float drive, const juce::ADSR::Parameters &ampParams, const juce::ADSR::Parameters &filterParams, float unisonBias, bool retrigger, uint32_t timestamp);
        void stop();
        void kill();

        bool active = false;
        bool isKeyDown = false;
        int currentNote = -1;
        uint32_t noteOnTime = 0; // For LRU Voice Stealing
        float currentVelocity = 0.0f;
        float phase = 0.0f;
        float phaseDelta = 0.0f;
        float targetFrequency = 0.0f;
        float phase2 = 0.0f;
        float phaseDelta2 = 0.0f;
        float targetFrequency2 = 0.0f;
        float sampleRate = 44100.0f;

        // Unison Handling
        float unisonBias = 0.0f; // -1.0 (Left/Flat) to +1.0 (Right/Sharp)
        float currentPan = 0.5f;
        float currentDetuneMultiplier = 1.0f;

        juce::ADSR adsr;
        juce::ADSR filterAdsr;
        juce::dsp::LadderFilter<float> filter;
        juce::dsp::StateVariableTPTFilter<float> svfFilter;

        // For Noise
        juce::Random random;
    };

    void processMidiMessages(te::MidiMessageArray *midiMessages, const juce::ADSR::Parameters &ampParams, const juce::ADSR::Parameters &filterParams);
    void triggerNote(int note, float velocity, int unisonOrder, bool retrigger, float startCutoff, float drive, const juce::ADSR::Parameters &ampParams, const juce::ADSR::Parameters &filterParams);
    void updateVoiceParameters(int unisonOrder, float unisonDetuneCents, float unisonSpread, float resonance, float drive, float coarseTune, float fineTuneCents, float osc2Coarse, float osc2FineCents, const juce::ADSR::Parameters &ampAdsr, const juce::ADSR::Parameters &filterAdsr);
    void renderAudio(const te::PluginRenderContext &, float baseCutoff, float filterEnvAmount, int waveShape, int unisonOrder, float drive);

    inline float generateWaveSample(int waveShape, float phase, float phaseDelta, juce::Random &random);

    Voice *findVoiceToSteal();
    uint32_t noteCounter = 0;
    int lastUnisonOrder = 1;

    // Thread-safe parameters for the Audio Thread
    struct AudioParams
    {
        std::atomic<float> level{0.0f}, coarseTune{0.0f}, fineTune{0.0f}, wave{2.0f};
        std::atomic<float> osc2Enabled{0.0f}, osc2Wave{0.0f}, osc2Coarse{0.0f}, osc2Fine{0.0f}, osc2Level{0.0f};
        std::atomic<float> mixMode{0.0f}, crossModAmount{0.0f};
        std::atomic<float> attack{0.005f}, decay{0.005f}, sustain{1.0f}, release{0.005f};
        std::atomic<float> unisonOrder{1.0f}, unisonDetune{0.0f}, unisonSpread{0.0f}, retrigger{0.0f};
        std::atomic<float> filterType{0.0f}, filterCutoff{20000.0f}, filterRes{0.0f}, filterDrive{1.0f}, filterEnvAmount{0.0f};
        std::atomic<float> filterAttack{0.005f}, filterDecay{0.005f}, filterSustain{1.0f}, filterRelease{0.005f};
    } audioParams;

    void updateAtomics();
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

    static constexpr int numVoices = 16;
    Voice voices[numVoices];

    juce::LinearSmoothedValue<float> masterLevelSmoother;
    juce::LinearSmoothedValue<float> cutoffSmoother;
    std::atomic<bool> panicTriggered{false};
    bool lastWasPlaying = false;

    juce::dsp::LookupTable<float> sineTable;
    float sineTableScaler = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthPlugin)
};

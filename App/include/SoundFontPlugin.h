#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <atomic>
#include <memory>

namespace te = tracktion_engine;

extern "C"
{
    typedef struct tsf tsf;
}

class SoundFontPlugin : public te::Plugin
{
public:
    SoundFontPlugin(te::PluginCreationInfo info);
    ~SoundFontPlugin() override;

    static constexpr const char *xmlTypeName = "soundfont_player";
    static const char *getPluginName() { return "SoundFont Player"; }

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
    void restorePluginStateFromValueTree(const juce::ValueTree &v) override;

    bool loadSoundFontFile(const juce::String &filePath);
    bool setCurrentPresetIndex(int presetIndex);
    void setOutputGainDb(float gainDb);

    juce::String getSoundFontFilePath() const;
    juce::String getLastError() const;
    juce::String getCurrentPresetName() const;
    juce::StringArray getPresetNames() const;
    int getCurrentPresetIndex() const;
    float getOutputGainDb() const;
    bool hasLoadedSoundFont() const;

private:
    struct RenderState;
    struct ScratchBuffer;

    std::shared_ptr<RenderState> createRenderState(const juce::String &filePath, int desiredPresetIndex, juce::String &errorMessage, juce::StringArray &presetNames, int &actualPresetIndex) const;
    void publishRenderState(std::shared_ptr<RenderState> renderState, const juce::String &filePath, const juce::StringArray &presetNames, int presetIndex, const juce::String &errorMessage);
    void clearRenderState(const juce::String &filePath, int presetIndex, const juce::String &errorMessage);
    void syncRenderState(RenderState &renderState);
    static void applyCurrentPresetToAllChannels(RenderState &renderState, int presetIndex);
    static void updateSynthOutput(RenderState &renderState, double sampleRate, float gainDb);
    juce::String resolveStoredSoundFontPath(const juce::String &storedPath) const;
    juce::String getStatePathForFile(const juce::String &filePath) const;
    void setLastErrorInternal(const juce::String &message);

    static constexpr int numMidiChannels = 16;
    static constexpr int maxVoices = 256;

    mutable juce::CriticalSection m_metadataLock;
    juce::StringArray m_presetNames;
    juce::String m_soundFontPath;
    juce::String m_lastError;
    std::shared_ptr<RenderState> m_renderState;
    std::shared_ptr<ScratchBuffer> m_scratchBuffer;
    std::atomic<double> m_sampleRate{44100.0};
    std::atomic<int> m_currentPresetIndex{0};
    std::atomic<float> m_outputGainDb{0.0f};
    std::atomic<bool> m_panicRequested{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontPlugin)
};

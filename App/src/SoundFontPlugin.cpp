#include "SoundFontPlugin.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

struct SoundFontPlugin::RenderState
{
    ~RenderState()
    {
        if (synth != nullptr)
            tsf_close(synth);
    }

    juce::MemoryBlock soundFontData;
    tsf *synth = nullptr;
    int presetCount = 0;
    int activePresetIndex = 0;
    double activeSampleRate = 44100.0;
    float activeGainDb = 0.0f;
};

struct SoundFontPlugin::ScratchBuffer
{
    explicit ScratchBuffer(int frameCapacity)
        : interleaved(static_cast<size_t>(juce::jmax(1, frameCapacity)) * 2u)
    {
    }

    int getFrameCapacity() const { return static_cast<int>(interleaved.size() / 2u); }

    std::vector<float> interleaved;
};

namespace
{
constexpr const char *soundFontPathProperty = "soundFontPath";
constexpr const char *presetIndexProperty = "presetIndex";
constexpr const char *gainDbProperty = "gainDb";
constexpr int soundFontMidiChannelCount = 16;

int toSynthChannel(int midiChannel) { return juce::jlimit(0, soundFontMidiChannelCount - 1, midiChannel - 1); }
} // namespace

SoundFontPlugin::SoundFontPlugin(te::PluginCreationInfo info)
    : te::Plugin(info)
{
    auto *undoManager = getUndoManager();

    // Ensure older/newer project states always have the properties this plugin expects.
    if (!state.hasProperty(soundFontPathProperty))
        state.setProperty(soundFontPathProperty, {}, undoManager);

    if (!state.hasProperty(presetIndexProperty))
        state.setProperty(presetIndexProperty, 0, undoManager);

    if (!state.hasProperty(gainDbProperty))
        state.setProperty(gainDbProperty, 0.0f, undoManager);

    const auto restoredPath = resolveStoredSoundFontPath(state.getProperty(soundFontPathProperty).toString());
    const auto restoredPresetIndex = static_cast<int>(state.getProperty(presetIndexProperty, 0));
    const auto restoredGainDb = static_cast<float>(state.getProperty(gainDbProperty, 0.0f));

    m_currentPresetIndex.store(juce::jmax(0, restoredPresetIndex), std::memory_order_relaxed);
    m_outputGainDb.store(juce::jlimit(-60.0f, 12.0f, restoredGainDb), std::memory_order_relaxed);

    if (restoredPath.isNotEmpty())
    {
        juce::String errorMessage;
        juce::StringArray presetNames;
        int actualPresetIndex = restoredPresetIndex;

        if (auto renderState = createRenderState(restoredPath, restoredPresetIndex, errorMessage, presetNames, actualPresetIndex))
            publishRenderState(std::move(renderState), restoredPath, presetNames, actualPresetIndex, {});
        else
            clearRenderState(restoredPath, restoredPresetIndex, errorMessage);
    }
    else
    {
        clearRenderState({}, restoredPresetIndex, {});
    }
}

SoundFontPlugin::~SoundFontPlugin()
{
    std::atomic_store_explicit(&m_renderState, std::shared_ptr<RenderState>{}, std::memory_order_release);
    std::atomic_store_explicit(&m_scratchBuffer, std::shared_ptr<ScratchBuffer>{}, std::memory_order_release);
    notifyListenersOfDeletion();
}

void SoundFontPlugin::getChannelNames(juce::StringArray *ins, juce::StringArray *outs)
{
    if (ins != nullptr)
        ins->clear();

    if (outs != nullptr)
    {
        outs->clear();
        outs->add("Left");
        outs->add("Right");
    }
}

int SoundFontPlugin::getNumOutputChannelsGivenInputs(int) { return 2; }

void SoundFontPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    m_sampleRate.store(info.sampleRate > 0.0 ? info.sampleRate : 44100.0, std::memory_order_relaxed);

    const int requiredFrames = juce::jmax(1, info.blockSizeSamples);
    auto scratchBuffer = std::atomic_load_explicit(&m_scratchBuffer, std::memory_order_acquire);

    if (scratchBuffer == nullptr || scratchBuffer->getFrameCapacity() < requiredFrames)
    {
        scratchBuffer = std::make_shared<ScratchBuffer>(requiredFrames);
        std::atomic_store_explicit(&m_scratchBuffer, std::move(scratchBuffer), std::memory_order_release);
    }
}

void SoundFontPlugin::deinitialise() { m_panicRequested.store(true, std::memory_order_release); }

void SoundFontPlugin::reset() { midiPanic(); }

void SoundFontPlugin::midiPanic() { m_panicRequested.store(true, std::memory_order_release); }

void SoundFontPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    const auto restoredPath = resolveStoredSoundFontPath(v.getProperty(soundFontPathProperty, {}).toString());
    const auto restoredPresetIndex = juce::jmax(0, static_cast<int>(v.getProperty(presetIndexProperty, 0)));
    const auto restoredGainDb = juce::jlimit(-60.0f, 12.0f, static_cast<float>(v.getProperty(gainDbProperty, 0.0f)));

    m_outputGainDb.store(restoredGainDb, std::memory_order_relaxed);
    m_currentPresetIndex.store(restoredPresetIndex, std::memory_order_relaxed);
    m_panicRequested.store(true, std::memory_order_release);

    if (restoredPath.isNotEmpty())
    {
        juce::String errorMessage;
        juce::StringArray presetNames;
        int actualPresetIndex = restoredPresetIndex;

        if (auto renderState = createRenderState(restoredPath, restoredPresetIndex, errorMessage, presetNames, actualPresetIndex))
            publishRenderState(std::move(renderState), restoredPath, presetNames, actualPresetIndex, {});
        else
            clearRenderState(restoredPath, restoredPresetIndex, errorMessage);
    }
    else
    {
        clearRenderState({}, restoredPresetIndex, {});
    }

    state.setProperty(soundFontPathProperty, restoredPath.isNotEmpty() ? getStatePathForFile(restoredPath) : juce::String(), nullptr);
    state.setProperty(presetIndexProperty, getCurrentPresetIndex(), nullptr);
    state.setProperty(gainDbProperty, restoredGainDb, nullptr);
}

bool SoundFontPlugin::loadSoundFontFile(const juce::String &filePath)
{
    const auto normalizedPath = resolveStoredSoundFontPath(filePath);
    juce::String errorMessage;
    juce::StringArray presetNames;
    int actualPresetIndex = m_currentPresetIndex.load(std::memory_order_relaxed);

    auto renderState = createRenderState(normalizedPath, actualPresetIndex, errorMessage, presetNames, actualPresetIndex);
    if (renderState == nullptr)
    {
        const juce::ScopedLock sl(m_metadataLock);
        setLastErrorInternal(errorMessage);
        return false;
    }

    publishRenderState(std::move(renderState), normalizedPath, presetNames, actualPresetIndex, {});
    state.setProperty(soundFontPathProperty, getStatePathForFile(normalizedPath), nullptr);
    state.setProperty(presetIndexProperty, actualPresetIndex, nullptr);
    return true;
}

bool SoundFontPlugin::setCurrentPresetIndex(int presetIndex)
{
    int clampedPresetIndex = juce::jmax(0, presetIndex);
    bool hasLoadedPreset = false;
    {
        const juce::ScopedLock sl(m_metadataLock);
        if (!m_presetNames.isEmpty())
        {
            clampedPresetIndex = juce::jlimit(0, m_presetNames.size() - 1, presetIndex);
            hasLoadedPreset = true;
        }
    }

    m_currentPresetIndex.store(clampedPresetIndex, std::memory_order_relaxed);
    m_panicRequested.store(true, std::memory_order_release);

    if (hasLoadedPreset)
        state.setProperty(presetIndexProperty, clampedPresetIndex, nullptr);

    return hasLoadedPreset;
}

void SoundFontPlugin::setOutputGainDb(float gainDb)
{
    const auto clampedGain = juce::jlimit(-60.0f, 12.0f, gainDb);
    m_outputGainDb.store(clampedGain, std::memory_order_relaxed);
    state.setProperty(gainDbProperty, clampedGain, nullptr);
}

juce::String SoundFontPlugin::getSoundFontFilePath() const
{
    const juce::ScopedLock sl(m_metadataLock);
    return m_soundFontPath;
}

juce::String SoundFontPlugin::getLastError() const
{
    const juce::ScopedLock sl(m_metadataLock);
    return m_lastError;
}

juce::String SoundFontPlugin::getCurrentPresetName() const
{
    const juce::ScopedLock sl(m_metadataLock);
    const int presetIndex = m_currentPresetIndex.load(std::memory_order_relaxed);

    if (!juce::isPositiveAndBelow(presetIndex, m_presetNames.size()))
        return {};

    return m_presetNames[presetIndex];
}

juce::StringArray SoundFontPlugin::getPresetNames() const
{
    const juce::ScopedLock sl(m_metadataLock);
    return m_presetNames;
}

int SoundFontPlugin::getCurrentPresetIndex() const { return m_currentPresetIndex.load(std::memory_order_relaxed); }

float SoundFontPlugin::getOutputGainDb() const { return m_outputGainDb.load(std::memory_order_relaxed); }

bool SoundFontPlugin::hasLoadedSoundFont() const { return std::atomic_load_explicit(&m_renderState, std::memory_order_acquire) != nullptr; }

void SoundFontPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0)
        return;

    const int bufferStart = juce::jlimit(0, fc.destBuffer->getNumSamples(), fc.bufferStartSample);
    const int availableSamples = fc.destBuffer->getNumSamples() - bufferStart;
    const int numSamples = juce::jmin(fc.bufferNumSamples, availableSamples);

    if (numSamples <= 0)
        return;

    // This is an instrument, so each render block starts from silence and is filled only by the synth output.
    for (int channel = 0; channel < fc.destBuffer->getNumChannels(); ++channel)
        fc.destBuffer->clear(channel, bufferStart, numSamples);

    if (!isEnabled())
        return;

    auto renderState = std::atomic_load_explicit(&m_renderState, std::memory_order_acquire);
    if (renderState == nullptr || renderState->synth == nullptr)
        return;

    auto scratchBuffer = std::atomic_load_explicit(&m_scratchBuffer, std::memory_order_acquire);
    if (scratchBuffer == nullptr || scratchBuffer->getFrameCapacity() <= 0)
    {
        jassertfalse;
        return;
    }

    syncRenderState(*renderState);

    const int scratchFrames = scratchBuffer->getFrameCapacity();
    const double currentSampleRate = renderState->activeSampleRate > 0.0 ? renderState->activeSampleRate : m_sampleRate.load(std::memory_order_relaxed);

    auto *left = fc.destBuffer->getWritePointer(0, bufferStart);
    auto *right = fc.destBuffer->getNumChannels() > 1 ? fc.destBuffer->getWritePointer(1, bufferStart) : nullptr;

    auto renderSamples = [&](int startSample, int sampleCount)
    {
        for (int rendered = 0; rendered < sampleCount; rendered += scratchFrames)
        {
            const int chunkSamples = juce::jmin(scratchFrames, sampleCount - rendered);
            tsf_render_float(renderState->synth, scratchBuffer->interleaved.data(), chunkSamples, 0);

            for (int sample = 0; sample < chunkSamples; ++sample)
            {
                const auto interleavedIndex = static_cast<size_t>(sample) * 2u;
                const float leftSample = scratchBuffer->interleaved[interleavedIndex];
                const float rightSample = scratchBuffer->interleaved[interleavedIndex + 1u];
                const int destSample = startSample + rendered + sample;

                left[destSample] = leftSample;

                if (right != nullptr)
                    right[destSample] = rightSample;
                else
                    left[destSample] = 0.5f * (leftSample + rightSample);
            }
        }
    };

    auto applyMidiMessage = [&](const auto &message)
    {
        const int channel = toSynthChannel(message.getChannel());

        if (message.isNoteOn())
        {
            const int note = juce::jlimit(0, 127, message.getNoteNumber());
            const float velocity = juce::jlimit(0.0f, 1.0f, message.getFloatVelocity());

            if (velocity > 0.0f)
                tsf_channel_note_on(renderState->synth, channel, note, velocity);
            else
                tsf_channel_note_off(renderState->synth, channel, note);
        }
        else if (message.isNoteOff())
        {
            tsf_channel_note_off(renderState->synth, channel, juce::jlimit(0, 127, message.getNoteNumber()));
        }
        else if (message.isAllNotesOff())
        {
            tsf_channel_note_off_all(renderState->synth, channel);
        }
        else if (message.isAllSoundOff())
        {
            tsf_channel_sounds_off_all(renderState->synth, channel);
        }
        else if (message.isPitchWheel())
        {
            tsf_channel_set_pitchwheel(renderState->synth, channel, message.getPitchWheelValue());
        }
        else if (message.isController())
        {
            tsf_channel_midi_control(renderState->synth, channel, message.getControllerNumber(), message.getControllerValue());
        }
    };

    int renderedSamples = 0;

    if (fc.bufferForMidiMessages != nullptr)
    {
        if (fc.bufferForMidiMessages->isAllNotesOff)
            tsf_note_off_all(renderState->synth);

        auto getEventSample = [&](int messageIndex)
        {
            const auto eventTimeSeconds = (*fc.bufferForMidiMessages)[messageIndex].getTimeStamp() - fc.midiBufferOffset;
            return juce::jlimit(0, numSamples, juce::roundToInt(eventTimeSeconds * currentSampleRate));
        };

        for (int messageIndex = 0; messageIndex < fc.bufferForMidiMessages->size();)
        {
            const int eventSample = getEventSample(messageIndex);

            if (eventSample > renderedSamples)
            {
                renderSamples(renderedSamples, eventSample - renderedSamples);
                renderedSamples = eventSample;
            }

            do
            {
                applyMidiMessage((*fc.bufferForMidiMessages)[messageIndex]);
                ++messageIndex;
            } while (messageIndex < fc.bufferForMidiMessages->size() && getEventSample(messageIndex) == eventSample);
        }
    }

    if (renderedSamples < numSamples)
        renderSamples(renderedSamples, numSamples - renderedSamples);
}

std::shared_ptr<SoundFontPlugin::RenderState> SoundFontPlugin::createRenderState(const juce::String &filePath, int desiredPresetIndex, juce::String &errorMessage, juce::StringArray &presetNames, int &actualPresetIndex) const
{
    actualPresetIndex = juce::jmax(0, desiredPresetIndex);

    if (filePath.isEmpty())
    {
        errorMessage = {};
        return {};
    }

    const juce::File file(filePath);

    if (!file.existsAsFile())
    {
        errorMessage = "SoundFont file not found.";
        return {};
    }

    auto renderState = std::make_shared<RenderState>();
    if (!file.loadFileAsData(renderState->soundFontData) || renderState->soundFontData.getSize() == 0)
    {
        errorMessage = "Failed to read SoundFont file.";
        return {};
    }

    // TinySoundFont exposes memory-based loading, so we fully own the file contents for the synth lifetime.
    renderState->synth = tsf_load_memory(renderState->soundFontData.getData(), static_cast<int>(renderState->soundFontData.getSize()));

    if (renderState->synth == nullptr)
    {
        errorMessage = "Failed to parse SoundFont file.";
        return {};
    }

    tsf_set_max_voices(renderState->synth, maxVoices);
    updateSynthOutput(*renderState, m_sampleRate.load(std::memory_order_relaxed), m_outputGainDb.load(std::memory_order_relaxed));

    renderState->presetCount = tsf_get_presetcount(renderState->synth);
    presetNames.clear();
    presetNames.ensureStorageAllocated(renderState->presetCount);

    for (int presetIndex = 0; presetIndex < renderState->presetCount; ++presetIndex)
    {
        const auto *presetName = tsf_get_presetname(renderState->synth, presetIndex);
        presetNames.add(juce::String(presetIndex + 1) + ": " + juce::String(presetName != nullptr ? presetName : "Preset"));
    }

    if (presetNames.isEmpty())
    {
        errorMessage = "This SoundFont has no presets.";
        return {};
    }

    actualPresetIndex = juce::jlimit(0, presetNames.size() - 1, desiredPresetIndex);
    applyCurrentPresetToAllChannels(*renderState, actualPresetIndex);
    errorMessage = {};
    return renderState;
}

void SoundFontPlugin::publishRenderState(std::shared_ptr<RenderState> renderState, const juce::String &filePath, const juce::StringArray &presetNames, int presetIndex, const juce::String &errorMessage)
{
    std::atomic_store_explicit(&m_renderState, std::move(renderState), std::memory_order_release);
    m_currentPresetIndex.store(presetIndex, std::memory_order_relaxed);
    m_panicRequested.store(false, std::memory_order_release);

    const juce::ScopedLock sl(m_metadataLock);
    m_soundFontPath = filePath;
    m_presetNames = presetNames;
    setLastErrorInternal(errorMessage);
}

void SoundFontPlugin::clearRenderState(const juce::String &filePath, int presetIndex, const juce::String &errorMessage)
{
    std::atomic_store_explicit(&m_renderState, std::shared_ptr<RenderState>{}, std::memory_order_release);
    m_currentPresetIndex.store(juce::jmax(0, presetIndex), std::memory_order_relaxed);
    m_panicRequested.store(false, std::memory_order_release);

    {
        const juce::ScopedLock sl(m_metadataLock);
        m_soundFontPath = filePath;
        m_presetNames.clear();
        setLastErrorInternal(errorMessage);
    }
}

void SoundFontPlugin::syncRenderState(RenderState &renderState)
{
    const auto desiredSampleRate = m_sampleRate.load(std::memory_order_relaxed);
    const auto desiredGainDb = m_outputGainDb.load(std::memory_order_relaxed);
    const auto desiredPresetIndex = juce::jlimit(0, juce::jmax(0, renderState.presetCount - 1), m_currentPresetIndex.load(std::memory_order_relaxed));
    const bool shouldResetVoices = m_panicRequested.exchange(false, std::memory_order_acq_rel);

    if (renderState.activeSampleRate != desiredSampleRate || renderState.activeGainDb != desiredGainDb)
        updateSynthOutput(renderState, desiredSampleRate, desiredGainDb);

    if (shouldResetVoices)
    {
        tsf_reset(renderState.synth);
        updateSynthOutput(renderState, desiredSampleRate, desiredGainDb);
    }

    if (shouldResetVoices || renderState.activePresetIndex != desiredPresetIndex)
    {
        tsf_note_off_all(renderState.synth);
        applyCurrentPresetToAllChannels(renderState, desiredPresetIndex);
    }
}

void SoundFontPlugin::applyCurrentPresetToAllChannels(RenderState &renderState, int presetIndex)
{
    // The MVP exposes one selected preset for the whole plugin, so every MIDI channel uses the same preset.
    for (int channel = 0; channel < numMidiChannels; ++channel)
        tsf_channel_set_presetindex(renderState.synth, channel, presetIndex);

    renderState.activePresetIndex = presetIndex;
}

void SoundFontPlugin::updateSynthOutput(RenderState &renderState, double sampleRate, float gainDb)
{
    tsf_set_output(renderState.synth, TSF_STEREO_INTERLEAVED, juce::roundToInt(sampleRate), gainDb);
    renderState.activeSampleRate = sampleRate;
    renderState.activeGainDb = gainDb;
}

juce::String SoundFontPlugin::resolveStoredSoundFontPath(const juce::String &storedPath) const
{
    if (storedPath.isEmpty())
        return {};

    if (edit.filePathResolver != nullptr)
        return edit.filePathResolver(storedPath).getFullPathName();

    return juce::File(storedPath).getFullPathName();
}

juce::String SoundFontPlugin::getStatePathForFile(const juce::String &filePath) const
{
    const juce::File absoluteFile(filePath);

    if (!juce::File::isAbsolutePath(filePath))
        return absoluteFile.getFullPathName();

    if (edit.editFileRetriever != nullptr)
    {
        const auto editFile = edit.editFileRetriever();
        if (editFile.existsAsFile())
            return absoluteFile.getRelativePathFrom(editFile.getParentDirectory());
    }

    return absoluteFile.getFullPathName();
}

void SoundFontPlugin::setLastErrorInternal(const juce::String &message) { m_lastError = message; }

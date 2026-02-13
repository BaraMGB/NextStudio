#include "Plugins/Delay/NextDelayPlugin.h"

#include "Utilities/Utilities.h"

namespace
{
float skewedCutoffRange = 0.3f;
} // namespace

NextDelayPlugin::NextDelayPlugin(te::PluginCreationInfo info)
    : te::Plugin(info),
      m_delayLine((int)std::ceil((NextDelayPlugin::maxDelayMs + NextDelayPlugin::maxStereoOffsetMs) * 44100.0f / 1000.0f) + 4)
{
    auto um = getUndoManager();

    modeValue.referTo(state, "mode", um, 0.0f);
    modeParam = addParam(
        "mode", "Mode", {0.0f, 3.0f, 1.0f},
        [](float v)
        {
            const int mode = juce::roundToInt(v);
            if (mode == (int)NextDelayPlugin::DelayMode::mono)
                return juce::String("Mono");
            if (mode == (int)NextDelayPlugin::DelayMode::stereo)
                return juce::String("Stereo");
            if (mode == (int)NextDelayPlugin::DelayMode::pingPong)
                return juce::String("PingPong");
            if (mode == (int)NextDelayPlugin::DelayMode::dual)
                return juce::String("Dual");
            return juce::String("Mono");
        },
        [](const juce::String &s)
        {
            if (s == "Mono")
                return (float)NextDelayPlugin::DelayMode::mono;
            if (s == "Stereo")
                return (float)NextDelayPlugin::DelayMode::stereo;
            if (s == "PingPong")
                return (float)NextDelayPlugin::DelayMode::pingPong;
            if (s == "Dual")
                return (float)NextDelayPlugin::DelayMode::dual;
            return (float)NextDelayPlugin::DelayMode::mono;
        });
    modeParam->attachToCurrentValue(modeValue);

    syncEnabledValue.referTo(state, "syncEnabled", um, 0.0f);
    syncEnabledParam = addParam("syncEnabled", "Sync", {0.0f, 1.0f, 1.0f}, [](float v) { return v >= 0.5f ? juce::String("On") : juce::String("Off"); }, [](const juce::String &s) { return s == "On" ? 1.0f : 0.0f; });
    syncEnabledParam->attachToCurrentValue(syncEnabledValue);

    syncDivisionValue.referTo(state, "syncDivision", um, 3.0f);
    syncDivisionParam = addParam(
        "syncDivision", "Division", {0.0f, 11.0f, 1.0f},
        [](float v)
        {
            static const char *labels[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4D", "1/8D", "1/16D", "1/4T", "1/8T", "1/16T"};
            return juce::String(labels[juce::jlimit(0, 11, juce::roundToInt(v))]);
        },
        [](const juce::String &s)
        {
            static const char *labels[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4D", "1/8D", "1/16D", "1/4T", "1/8T", "1/16T"};
            for (int i = 0; i < 12; ++i)
                if (s == labels[i])
                    return (float)i;
            return 3.0f;
        });
    syncDivisionParam->attachToCurrentValue(syncDivisionValue);

    timeMsValue.referTo(state, "timeMs", um, 250.0f);
    timeMsParam = addParam("timeMs", "Time", {minDelayMs, maxDelayMs, 0.0f, 0.35f}, [](float v) { return juce::String(juce::roundToInt(v)) + " ms"; }, [](const juce::String &s) { return juce::jlimit(minDelayMs, maxDelayMs, s.getFloatValue()); });
    timeMsParam->attachToCurrentValue(timeMsValue);

    feedbackValue.referTo(state, "feedback", um, 0.35f);
    feedbackParam = addParam("feedback", "Feedback", {0.0f, maxFeedback}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(0.0f, maxFeedback, s.getFloatValue() / 100.0f); });
    feedbackParam->attachToCurrentValue(feedbackValue);

    mixValue.referTo(state, "mix", um, 0.25f);
    mixParam = addParam("mix", "Mix", {0.0f, 1.0f}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(0.0f, 1.0f, s.getFloatValue() / 100.0f); });
    mixParam->attachToCurrentValue(mixValue);

    stereoOffsetValue.referTo(state, "stereoOffsetMs", um, 15.0f);
    stereoOffsetParam = addParam("stereoOffsetMs", "Stereo Offset", {-maxStereoOffsetMs, maxStereoOffsetMs}, [](float v) { return juce::String(v, 1) + " ms"; }, [](const juce::String &s) { return juce::jlimit(-maxStereoOffsetMs, maxStereoOffsetMs, s.getFloatValue()); });
    stereoOffsetParam->attachToCurrentValue(stereoOffsetValue);

    pingPongAmountValue.referTo(state, "pingPongAmount", um, 1.0f);
    pingPongAmountParam = addParam("pingPongAmount", "PingPong", {0.0f, 1.0f}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(0.0f, 1.0f, s.getFloatValue() / 100.0f); });
    pingPongAmountParam->attachToCurrentValue(pingPongAmountValue);

    hpCutoffValue.referTo(state, "hpCutoff", um, 20.0f);
    hpCutoffParam = addParam("hpCutoff", "HP", {20.0f, 20000.0f, 0.0f, skewedCutoffRange}, [](float v) { return juce::String(juce::roundToInt(v)) + " Hz"; }, [](const juce::String &s) { return juce::jlimit(20.0f, 20000.0f, s.getFloatValue()); });
    hpCutoffParam->attachToCurrentValue(hpCutoffValue);

    lpCutoffValue.referTo(state, "lpCutoff", um, 18000.0f);
    lpCutoffParam = addParam("lpCutoff", "LP", {20.0f, 20000.0f, 0.0f, skewedCutoffRange}, [](float v) { return juce::String(juce::roundToInt(v)) + " Hz"; }, [](const juce::String &s) { return juce::jlimit(20.0f, 20000.0f, s.getFloatValue()); });
    lpCutoffParam->attachToCurrentValue(lpCutoffValue);

    state.addListener(this);
    updateAtomics();
}

NextDelayPlugin::~NextDelayPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    modeParam->detachFromCurrentValue();
    syncEnabledParam->detachFromCurrentValue();
    syncDivisionParam->detachFromCurrentValue();
    timeMsParam->detachFromCurrentValue();
    feedbackParam->detachFromCurrentValue();
    mixParam->detachFromCurrentValue();
    stereoOffsetParam->detachFromCurrentValue();
    pingPongAmountParam->detachFromCurrentValue();
    hpCutoffParam->detachFromCurrentValue();
    lpCutoffParam->detachFromCurrentValue();
}

void NextDelayPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    const auto sr = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = juce::jmax((juce::uint32)64, (juce::uint32)juce::jmax(1, info.blockSizeSamples));
    spec.numChannels = maxChannels;

    const int maxDelaySamples = (int)std::ceil((maxDelayMs + maxStereoOffsetMs) * (float)sr / 1000.0f) + 4;
    m_delayLine.setMaximumDelayInSamples(maxDelaySamples);
    m_delayLine.prepare(spec);
    m_delayLine.reset();

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        m_hpFilters[ch].prepare(spec);
        m_hpFilters[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        m_lpFilters[ch].prepare(spec);
        m_lpFilters[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    m_leftDelaySamples.reset(sr, 0.03);
    m_rightDelaySamples.reset(sr, 0.03);

    updateFilterCutoffs(audioParams.hpCutoff.load(std::memory_order_relaxed), audioParams.lpCutoff.load(std::memory_order_relaxed));
    reset();
}

void NextDelayPlugin::deinitialise() { m_delayLine.reset(); }

void NextDelayPlugin::reset()
{
    m_delayLine.reset();
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        m_hpFilters[ch].reset();
        m_lpFilters[ch].reset();
    }

    const auto sr = juce::jmax(1.0f, (float)sampleRate);
    const auto currentDelaySamples = juce::jlimit(minDelayMs * sr / 1000.0f, (float)m_delayLine.getMaximumDelayInSamples(), audioParams.timeMs.load(std::memory_order_relaxed) * sr / 1000.0f);
    m_leftDelaySamples.setCurrentAndTargetValue(currentDelaySamples);
    m_rightDelaySamples.setCurrentAndTargetValue(currentDelaySamples);
}

void NextDelayPlugin::midiPanic() { reset(); }

void NextDelayPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

void NextDelayPlugin::updateAtomics()
{
    audioParams.mode.store(modeValue.get(), std::memory_order_relaxed);
    audioParams.syncEnabled.store(syncEnabledValue.get(), std::memory_order_relaxed);
    audioParams.syncDivision.store(syncDivisionValue.get(), std::memory_order_relaxed);
    audioParams.timeMs.store(timeMsValue.get(), std::memory_order_relaxed);
    audioParams.feedback.store(feedbackValue.get(), std::memory_order_relaxed);
    audioParams.mix.store(mixValue.get(), std::memory_order_relaxed);
    audioParams.stereoOffsetMs.store(stereoOffsetValue.get(), std::memory_order_relaxed);
    audioParams.pingPongAmount.store(pingPongAmountValue.get(), std::memory_order_relaxed);
    audioParams.hpCutoff.store(hpCutoffValue.get(), std::memory_order_relaxed);
    audioParams.lpCutoff.store(lpCutoffValue.get(), std::memory_order_relaxed);
}

void NextDelayPlugin::updateFilterCutoffs(float hpCutoffHz, float lpCutoffHz)
{
    const auto hp = juce::jlimit(20.0f, 20000.0f, hpCutoffHz);
    const auto lp = juce::jlimit(20.0f, 20000.0f, lpCutoffHz);
    const auto saneLp = juce::jmax(lp, hp + 10.0f);

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        m_hpFilters[ch].setCutoffFrequency(hp);
        m_lpFilters[ch].setCutoffFrequency(saneLp);
    }
}

float NextDelayPlugin::getDivisionInQuarterNotes(int divisionIndex)
{
    switch (juce::jlimit(0, 11, divisionIndex))
    {
    case 0:
        return 4.0f;
    case 1:
        return 2.0f;
    case 2:
        return 1.0f;
    case 3:
        return 0.5f;
    case 4:
        return 0.25f;
    case 5:
        return 0.125f;
    case 6:
        return 1.5f;
    case 7:
        return 0.75f;
    case 8:
        return 0.375f;
    case 9:
        return 2.0f / 3.0f;
    case 10:
        return 1.0f / 3.0f;
    case 11:
        return 1.0f / 6.0f;
    default:
        return 0.5f;
    }
}

float NextDelayPlugin::getSyncedDelayMs(int divisionIndex, double bpm) const
{
    const auto clampedBpm = (float)juce::jlimit(10.0, 400.0, bpm);
    const auto quarterNoteMs = 60000.0f / clampedBpm;
    return quarterNoteMs * getDivisionInQuarterNotes(divisionIndex);
}

void NextDelayPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0)
        return;

    if (!isEnabled())
        return;

    const int numChannels = juce::jmin(fc.destBuffer->getNumChannels(), maxChannels);
    if (numChannels <= 0)
        return;

    const int startSample = juce::jlimit(0, fc.destBuffer->getNumSamples(), fc.bufferStartSample);
    const int availableSamples = fc.destBuffer->getNumSamples() - startSample;
    const int numSamples = juce::jmin(fc.bufferNumSamples, availableSamples);

    if (numSamples <= 0)
        return;

    for (int ch = numChannels; ch < fc.destBuffer->getNumChannels(); ++ch)
        fc.destBuffer->clear(ch, startSample, numSamples);

    const auto mode = (DelayMode)juce::jlimit(0, 3, juce::roundToInt(audioParams.mode.load(std::memory_order_relaxed)));
    const auto syncEnabled = audioParams.syncEnabled.load(std::memory_order_relaxed) >= 0.5f;
    const auto syncDivision = juce::roundToInt(audioParams.syncDivision.load(std::memory_order_relaxed));

    const auto bpm = edit.tempoSequence.getTempoAt(fc.editTime.getStart()).getBpm();
    const auto baseDelayMs = syncEnabled ? getSyncedDelayMs(syncDivision, bpm) : audioParams.timeMs.load(std::memory_order_relaxed);
    const auto clampedBaseDelayMs = juce::jlimit(minDelayMs, maxDelayMs, baseDelayMs);
    const auto stereoOffsetMs = juce::jlimit(-maxStereoOffsetMs, maxStereoOffsetMs, audioParams.stereoOffsetMs.load(std::memory_order_relaxed));

    float leftDelayMs = clampedBaseDelayMs;
    float rightDelayMs = clampedBaseDelayMs;

    switch (mode)
    {
    case DelayMode::stereo:
    {
        leftDelayMs = clampedBaseDelayMs + (stereoOffsetMs * 0.5f);
        rightDelayMs = clampedBaseDelayMs - (stereoOffsetMs * 0.5f);
        break;
    }
    case DelayMode::dual:
    {
        const float spread = std::abs(stereoOffsetMs);
        leftDelayMs = clampedBaseDelayMs - spread;
        rightDelayMs = clampedBaseDelayMs + spread;
        break;
    }
    case DelayMode::pingPong:
    case DelayMode::mono:
    default:
        break;
    }

    const auto sr = juce::jmax(1.0f, (float)sampleRate);
    const auto maxDelaySamples = (float)m_delayLine.getMaximumDelayInSamples();
    const auto leftTargetSamples = juce::jlimit(1.0f, maxDelaySamples, leftDelayMs * sr / 1000.0f);
    const auto rightTargetSamples = juce::jlimit(1.0f, maxDelaySamples, rightDelayMs * sr / 1000.0f);

    m_leftDelaySamples.setTargetValue(leftTargetSamples);
    m_rightDelaySamples.setTargetValue(rightTargetSamples);

    const auto feedback = juce::jlimit(0.0f, maxFeedback, audioParams.feedback.load(std::memory_order_relaxed));
    const auto mix = juce::jlimit(0.0f, 1.0f, audioParams.mix.load(std::memory_order_relaxed));
    const auto dry = 1.0f - mix;
    const auto pingPongAmount = juce::jlimit(0.0f, 1.0f, audioParams.pingPongAmount.load(std::memory_order_relaxed));

    updateFilterCutoffs(audioParams.hpCutoff.load(std::memory_order_relaxed), audioParams.lpCutoff.load(std::memory_order_relaxed));

    auto *left = fc.destBuffer->getWritePointer(0, startSample);
    auto *right = numChannels > 1 ? fc.destBuffer->getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto delaySamplesL = m_leftDelaySamples.getNextValue();
        const auto delaySamplesR = m_rightDelaySamples.getNextValue();

        const float inL = left[i];
        const float inR = right != nullptr ? right[i] : inL;

        const float delayedL = m_delayLine.popSample(0, delaySamplesL);
        const float delayedR = m_delayLine.popSample(1, delaySamplesR);

        const float fbL = m_lpFilters[0].processSample(0, m_hpFilters[0].processSample(0, delayedL));
        const float fbR = m_lpFilters[1].processSample(0, m_hpFilters[1].processSample(0, delayedR));

        float writeL = inL + (fbL * feedback);
        float writeR = inR + (fbR * feedback);

        float wetL = delayedL;
        float wetR = delayedR;

        if (mode == DelayMode::mono)
        {
            const float monoIn = 0.5f * (inL + inR);
            const float monoFb = 0.5f * (fbL + fbR);
            const float monoWet = 0.5f * (delayedL + delayedR);
            writeL = monoIn + (monoFb * feedback);
            writeR = writeL;
            wetL = monoWet;
            wetR = monoWet;
        }
        else if (mode == DelayMode::pingPong)
        {
            const float monoIn = 0.5f * (inL + inR);
            const float fbToL = juce::jmap(pingPongAmount, fbL, fbR);
            const float fbToR = juce::jmap(pingPongAmount, fbR, fbL);

            writeL = monoIn + (fbToL * feedback);
            writeR = (fbToR * feedback);
            wetL = delayedL;
            wetR = delayedR;
        }
        else if (mode == DelayMode::dual)
        {
            const float monoIn = 0.5f * (inL + inR);
            writeL = monoIn + (fbL * feedback);
            writeR = monoIn + (fbR * feedback);
        }

        m_delayLine.pushSample(0, writeL);
        m_delayLine.pushSample(1, writeR);

        left[i] = (inL * dry) + (wetL * mix);
        if (right != nullptr)
            right[i] = (inR * dry) + (wetR * mix);
    }

    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void NextDelayPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, modeValue, syncEnabledValue, syncDivisionValue, timeMsValue, feedbackValue, mixValue, stereoOffsetValue, pingPongAmountValue, hpCutoffValue, lpCutoffValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();

    updateAtomics();
}

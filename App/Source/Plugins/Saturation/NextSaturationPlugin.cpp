#include "Plugins/Saturation/NextSaturationPlugin.h"

#include <cmath>

namespace
{
constexpr float minToneCutoffHz = 1200.0f;
constexpr float maxToneCutoffHz = 18000.0f;
constexpr float parameterSmoothingTimeSeconds = 0.03f;

float shapeNormalized(float x, float exponent) { return std::pow(juce::jlimit(0.0f, 1.0f, x), exponent); }

float shapeSigned(float x, float exponent)
{
    const float magnitude = std::pow(std::abs(x), exponent);
    return x < 0.0f ? -magnitude : magnitude;
}

float mapToneToCutoff(float tone)
{
    const float shaped = shapeNormalized(tone, 1.45f);
    const float ratio = maxToneCutoffHz / minToneCutoffHz;
    return minToneCutoffHz * std::pow(ratio, shaped);
}

float mapDriveDbToEffectiveDb(float driveDb, float minDb, float maxDb)
{
    const float normal = (driveDb - minDb) / (maxDb - minDb);
    return minDb + (maxDb - minDb) * shapeNormalized(normal, 1.25f);
}
} // namespace

NextSaturationPlugin::NextSaturationPlugin(te::PluginCreationInfo info)
    : te::Plugin(info),
      m_oversampling2x(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true),
      m_oversampling4x(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
{
    auto *um = getUndoManager();

    m_inputValue.referTo(state, NextSaturationPlugin::inputParamID, um, 0.0f);
    m_inputParam = addParam(NextSaturationPlugin::inputParamID, "Input", {minInputDb, maxInputDb}, [](float v) { return juce::String(v, 1) + " dB"; }, [](const juce::String &s) { return juce::jlimit(minInputDb, maxInputDb, s.getFloatValue()); });
    m_inputParam->attachToCurrentValue(m_inputValue);

    m_driveValue.referTo(state, NextSaturationPlugin::driveParamID, um, 12.0f);
    m_driveParam = addParam(NextSaturationPlugin::driveParamID, "Drive", {minDriveDb, maxDriveDb}, [](float v) { return juce::String(v, 1) + " dB"; }, [](const juce::String &s) { return juce::jlimit(minDriveDb, maxDriveDb, s.getFloatValue()); });
    m_driveParam->attachToCurrentValue(m_driveValue);

    m_mixValue.referTo(state, NextSaturationPlugin::mixParamID, um, 1.0f);
    m_mixParam = addParam(NextSaturationPlugin::mixParamID, "Mix", {minMix, maxMix}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minMix, maxMix, s.getFloatValue() / 100.0f); });
    m_mixParam->attachToCurrentValue(m_mixValue);

    m_outputValue.referTo(state, NextSaturationPlugin::outputParamID, um, 0.0f);
    m_outputParam = addParam(NextSaturationPlugin::outputParamID, "Output", {minOutputDb, maxOutputDb}, [](float v) { return juce::String(v, 1) + " dB"; }, [](const juce::String &s) { return juce::jlimit(minOutputDb, maxOutputDb, s.getFloatValue()); });
    m_outputParam->attachToCurrentValue(m_outputValue);

    m_toneValue.referTo(state, NextSaturationPlugin::toneParamID, um, 0.65f);
    m_toneParam = addParam(NextSaturationPlugin::toneParamID, "Tone", {minTone, maxTone}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minTone, maxTone, s.getFloatValue() / 100.0f); });
    m_toneParam->attachToCurrentValue(m_toneValue);

    m_biasValue.referTo(state, NextSaturationPlugin::biasParamID, um, 0.0f);
    m_biasParam = addParam(NextSaturationPlugin::biasParamID, "Bias", {minBias, maxBias}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minBias, maxBias, s.getFloatValue() / 100.0f); });
    m_biasParam->attachToCurrentValue(m_biasValue);

    m_modeValue.referTo(state, NextSaturationPlugin::modeParamID, um, (float)NextSaturationPlugin::softClip);
    m_modeParam = addParam(NextSaturationPlugin::modeParamID, "Mode", {0.0f, 2.0f, 1.0f}, [](float v) { return NextSaturationPlugin::modeToText((int)std::round(v)); }, [](const juce::String &s) { return (float)NextSaturationPlugin::textToMode(s); });
    m_modeParam->attachToCurrentValue(m_modeValue);

    m_qualityValue.referTo(state, NextSaturationPlugin::qualityParamID, um, (float)NextSaturationPlugin::quality2x);
    m_qualityParam = addParam(NextSaturationPlugin::qualityParamID, "Quality", {0.0f, 2.0f, 1.0f}, [](float v) { return NextSaturationPlugin::qualityToText((int)std::round(v)); }, [](const juce::String &s) { return (float)NextSaturationPlugin::textToQuality(s); });
    m_qualityParam->attachToCurrentValue(m_qualityValue);

    m_inputMeasurer.setMode(te::LevelMeasurer::peakMode);
    m_outputMeasurer.setMode(te::LevelMeasurer::peakMode);

    m_toneFilter1x.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    m_toneFilter2x.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    m_toneFilter4x.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    state.addListener(this);
    updateAtomics();
}

NextSaturationPlugin::~NextSaturationPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    m_inputParam->detachFromCurrentValue();
    m_driveParam->detachFromCurrentValue();
    m_mixParam->detachFromCurrentValue();
    m_outputParam->detachFromCurrentValue();
    m_toneParam->detachFromCurrentValue();
    m_biasParam->detachFromCurrentValue();
    m_modeParam->detachFromCurrentValue();
    m_qualityParam->detachFromCurrentValue();
}

void NextSaturationPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    const double sampleRateHz = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
    const auto blockSize = (juce::uint32)juce::jmax(1, info.blockSizeSamples);

    juce::dsp::ProcessSpec spec1x;
    spec1x.sampleRate = sampleRateHz;
    spec1x.maximumBlockSize = juce::jmax((juce::uint32)64, blockSize);
    spec1x.numChannels = 2;

    juce::dsp::ProcessSpec spec2x = spec1x;
    spec2x.sampleRate = sampleRateHz * 2.0;
    spec2x.maximumBlockSize = spec1x.maximumBlockSize * 2;

    juce::dsp::ProcessSpec spec4x = spec1x;
    spec4x.sampleRate = sampleRateHz * 4.0;
    spec4x.maximumBlockSize = spec1x.maximumBlockSize * 4;

    m_oversampling2x.reset();
    m_oversampling4x.reset();
    m_oversampling2x.initProcessing(spec1x.maximumBlockSize);
    m_oversampling4x.initProcessing(spec1x.maximumBlockSize);

    m_toneFilter1x.prepare(spec1x);
    m_toneFilter2x.prepare(spec2x);
    m_toneFilter4x.prepare(spec4x);

    m_dryBuffer.setSize(2, (int)spec1x.maximumBlockSize, false, false, true);

    m_inputGainSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);
    m_driveGainSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);
    m_mixSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);
    m_outputGainSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);
    m_toneSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);
    m_biasSmoothed.reset(sampleRateHz, parameterSmoothingTimeSeconds);

    m_transitionLengthSamples = juce::jmax(32, (int)std::round(sampleRateHz * 0.01));
    m_transitionSamplesRemaining = 0;
    m_transitionStage = TransitionStage::none;

    reset();
}

void NextSaturationPlugin::deinitialise()
{
    m_oversampling2x.reset();
    m_oversampling4x.reset();
    m_toneFilter1x.reset();
    m_toneFilter2x.reset();
    m_toneFilter4x.reset();
    m_inputMeasurer.clear();
    m_outputMeasurer.clear();
}

void NextSaturationPlugin::reset()
{
    m_oversampling2x.reset();
    m_oversampling4x.reset();
    m_toneFilter1x.reset();
    m_toneFilter2x.reset();
    m_toneFilter4x.reset();
    m_inputMeasurer.clear();
    m_outputMeasurer.clear();

    const float inputDb = juce::jlimit(minInputDb, maxInputDb, m_audioParams.inputDb.load(std::memory_order_relaxed));
    const float driveDb = juce::jlimit(minDriveDb, maxDriveDb, m_audioParams.driveDb.load(std::memory_order_relaxed));
    const float shapedDriveDb = mapDriveDbToEffectiveDb(driveDb, minDriveDb, maxDriveDb);

    m_inputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputDb));
    m_driveGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(shapedDriveDb));
    m_mixSmoothed.setCurrentAndTargetValue(juce::jlimit(minMix, maxMix, m_audioParams.mix.load(std::memory_order_relaxed)));
    m_outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(minOutputDb, maxOutputDb, m_audioParams.outputDb.load(std::memory_order_relaxed))));
    m_toneSmoothed.setCurrentAndTargetValue(juce::jlimit(minTone, maxTone, m_audioParams.tone.load(std::memory_order_relaxed)));
    m_biasSmoothed.setCurrentAndTargetValue(shapeSigned(juce::jlimit(minBias, maxBias, m_audioParams.bias.load(std::memory_order_relaxed)), 1.6f));

    m_activeMode = juce::jlimit(0, 2, m_audioParams.mode.load(std::memory_order_relaxed));
    m_activeQuality = juce::jlimit(0, 2, m_audioParams.quality.load(std::memory_order_relaxed));
    m_targetMode = m_activeMode;
    m_targetQuality = m_activeQuality;
    m_transitionStage = TransitionStage::none;
    m_transitionSamplesRemaining = 0;
}

void NextSaturationPlugin::midiPanic() { reset(); }

void NextSaturationPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

juce::String NextSaturationPlugin::modeToText(int modeValue)
{
    switch (modeValue)
    {
    case NextSaturationPlugin::softClip:
        return "Soft";
    case NextSaturationPlugin::smooth:
        return "Smooth";
    case NextSaturationPlugin::hardClip:
        return "Hard";
    default:
        return "Soft";
    }
}

juce::String NextSaturationPlugin::qualityToText(int qualityValue)
{
    switch (qualityValue)
    {
    case NextSaturationPlugin::quality1x:
        return "1x";
    case NextSaturationPlugin::quality2x:
        return "2x";
    case NextSaturationPlugin::quality4x:
        return "4x";
    default:
        return "2x";
    }
}

int NextSaturationPlugin::textToMode(const juce::String &text)
{
    const auto t = text.trim().toLowerCase();
    if (t.startsWith("hard") || t == "2")
        return NextSaturationPlugin::hardClip;
    if (t.startsWith("smooth") || t == "1")
        return NextSaturationPlugin::smooth;
    return NextSaturationPlugin::softClip;
}

int NextSaturationPlugin::textToQuality(const juce::String &text)
{
    const auto t = text.trim().toLowerCase();
    if (t.startsWith("4") || t == "2")
        return NextSaturationPlugin::quality4x;
    if (t.startsWith("2") || t == "1")
        return NextSaturationPlugin::quality2x;
    return NextSaturationPlugin::quality1x;
}

void NextSaturationPlugin::updateAtomics()
{
    m_audioParams.inputDb.store(m_inputValue.get(), std::memory_order_relaxed);
    m_audioParams.driveDb.store(m_driveValue.get(), std::memory_order_relaxed);
    m_audioParams.mix.store(m_mixValue.get(), std::memory_order_relaxed);
    m_audioParams.outputDb.store(m_outputValue.get(), std::memory_order_relaxed);
    m_audioParams.tone.store(m_toneValue.get(), std::memory_order_relaxed);
    m_audioParams.bias.store(m_biasValue.get(), std::memory_order_relaxed);
    m_audioParams.mode.store(juce::jlimit(0, 2, (int)std::round(m_modeValue.get())), std::memory_order_relaxed);
    m_audioParams.quality.store(juce::jlimit(0, 2, (int)std::round(m_qualityValue.get())), std::memory_order_relaxed);
}

float NextSaturationPlugin::saturateSample(float x, int modeValue)
{
    switch (modeValue)
    {
    case NextSaturationPlugin::softClip:
        return std::tanh(x);
    case NextSaturationPlugin::smooth:
        return (2.0f / juce::MathConstants<float>::pi) * std::atan(x);
    case NextSaturationPlugin::hardClip:
        return juce::jlimit(-1.0f, 1.0f, x);
    default:
        return std::tanh(x);
    }
}

void NextSaturationPlugin::processSaturationBlock(juce::dsp::AudioBlock<float> block, juce::dsp::StateVariableTPTFilter<float> &toneFilter, float inputGainStart, float inputGainEnd, float driveGainStart, float driveGainEnd, float biasStart, float biasEnd, float toneStart, float toneEnd, int modeValue)
{
    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();

    const float inputGainDelta = numSamples > 1 ? (inputGainEnd - inputGainStart) / (float)(numSamples - 1) : 0.0f;
    const float driveGainDelta = numSamples > 1 ? (driveGainEnd - driveGainStart) / (float)(numSamples - 1) : 0.0f;
    const float biasDelta = numSamples > 1 ? (biasEnd - biasStart) / (float)(numSamples - 1) : 0.0f;

    float inputGain = inputGainStart;
    float driveGain = driveGainStart;
    float bias = biasStart;

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto *data = block.getChannelPointer(ch);
        for (size_t i = 0; i < numSamples; ++i)
        {
            const float sampleBias = bias * 0.35f;
            data[i] = saturateSample(data[i] * inputGain * driveGain + sampleBias, modeValue);
            inputGain += inputGainDelta;
            driveGain += driveGainDelta;
            bias += biasDelta;
        }

        inputGain = inputGainStart;
        driveGain = driveGainStart;
        bias = biasStart;
    }

    const float toneMid = 0.5f * (toneStart + toneEnd);
    const float toneCutoff = mapToneToCutoff(juce::jlimit(minTone, maxTone, toneMid));
    toneFilter.setCutoffFrequency(toneCutoff);
    juce::dsp::ProcessContextReplacing<float> context(block);
    toneFilter.process(context);
}

void NextSaturationPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0 || !isEnabled())
        return;

    const int numChannels = juce::jmin(fc.destBuffer->getNumChannels(), 2);
    if (numChannels <= 0)
        return;

    const int startSample = juce::jlimit(0, fc.destBuffer->getNumSamples(), fc.bufferStartSample);
    const int availableSamples = fc.destBuffer->getNumSamples() - startSample;
    const int numSamples = juce::jmin(fc.bufferNumSamples, availableSamples);
    if (numSamples <= 0)
        return;

    for (int ch = numChannels; ch < fc.destBuffer->getNumChannels(); ++ch)
        fc.destBuffer->clear(ch, startSample, numSamples);

    m_inputMeasurer.processBuffer(*fc.destBuffer, startSample, numSamples);

    const float inputDb = juce::jlimit(minInputDb, maxInputDb, m_audioParams.inputDb.load(std::memory_order_relaxed));
    const float driveDb = juce::jlimit(minDriveDb, maxDriveDb, m_audioParams.driveDb.load(std::memory_order_relaxed));
    const float shapedDriveDb = mapDriveDbToEffectiveDb(driveDb, minDriveDb, maxDriveDb);

    m_inputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(inputDb));
    m_driveGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(shapedDriveDb));
    m_mixSmoothed.setTargetValue(juce::jlimit(minMix, maxMix, m_audioParams.mix.load(std::memory_order_relaxed)));
    m_outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(minOutputDb, maxOutputDb, m_audioParams.outputDb.load(std::memory_order_relaxed))));
    m_toneSmoothed.setTargetValue(juce::jlimit(minTone, maxTone, m_audioParams.tone.load(std::memory_order_relaxed)));
    m_biasSmoothed.setTargetValue(shapeSigned(juce::jlimit(minBias, maxBias, m_audioParams.bias.load(std::memory_order_relaxed)), 1.6f));

    const float inputGainStart = m_inputGainSmoothed.getCurrentValue();
    const float inputGainEnd = m_inputGainSmoothed.skip(numSamples);
    const float driveGainStart = m_driveGainSmoothed.getCurrentValue();
    const float driveGainEnd = m_driveGainSmoothed.skip(numSamples);
    const float mixStart = m_mixSmoothed.getCurrentValue();
    const float mixEnd = m_mixSmoothed.skip(numSamples);
    const float outputGainStart = m_outputGainSmoothed.getCurrentValue();
    const float outputGainEnd = m_outputGainSmoothed.skip(numSamples);
    const float toneStart = m_toneSmoothed.getCurrentValue();
    const float toneEnd = m_toneSmoothed.skip(numSamples);
    const float biasStart = m_biasSmoothed.getCurrentValue();
    const float biasEnd = m_biasSmoothed.skip(numSamples);

    const int requestedMode = juce::jlimit(0, 2, m_audioParams.mode.load(std::memory_order_relaxed));
    const int requestedQuality = juce::jlimit(0, 2, m_audioParams.quality.load(std::memory_order_relaxed));

    if (requestedMode != m_targetMode || requestedQuality != m_targetQuality)
    {
        m_targetMode = requestedMode;
        m_targetQuality = requestedQuality;

        if (m_transitionStage == TransitionStage::none)
        {
            m_transitionStage = TransitionStage::fadeOut;
            m_transitionSamplesRemaining = m_transitionLengthSamples;
        }
    }

    const int localBlockCapacity = m_dryBuffer.getNumSamples();
    if (localBlockCapacity <= 0)
        return;

    int processed = 0;
    while (processed < numSamples)
    {
        const int chunkStart = startSample + processed;
        const int chunkSize = juce::jmin(numSamples - processed, localBlockCapacity);

        for (int ch = 0; ch < numChannels; ++ch)
            m_dryBuffer.copyFrom(ch, 0, *fc.destBuffer, ch, chunkStart, chunkSize);

        juce::dsp::AudioBlock<float> block(*fc.destBuffer);
        auto active = block.getSubsetChannelBlock(0, (size_t)numChannels).getSubBlock((size_t)chunkStart, (size_t)chunkSize);

        const float chunkPosStart = (float)processed / (float)numSamples;
        const float chunkPosEnd = (float)(processed + chunkSize) / (float)numSamples;

        const auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

        const float chunkInputGainStart = lerp(inputGainStart, inputGainEnd, chunkPosStart);
        const float chunkInputGainEnd = lerp(inputGainStart, inputGainEnd, chunkPosEnd);
        const float chunkDriveGainStart = lerp(driveGainStart, driveGainEnd, chunkPosStart);
        const float chunkDriveGainEnd = lerp(driveGainStart, driveGainEnd, chunkPosEnd);
        const float chunkToneStart = lerp(toneStart, toneEnd, chunkPosStart);
        const float chunkToneEnd = lerp(toneStart, toneEnd, chunkPosEnd);
        const float chunkBiasStart = lerp(biasStart, biasEnd, chunkPosStart);
        const float chunkBiasEnd = lerp(biasStart, biasEnd, chunkPosEnd);

        if (m_activeQuality == NextSaturationPlugin::quality1x)
        {
            processSaturationBlock(active, m_toneFilter1x, chunkInputGainStart, chunkInputGainEnd, chunkDriveGainStart, chunkDriveGainEnd, chunkBiasStart, chunkBiasEnd, chunkToneStart, chunkToneEnd, m_activeMode);
        }
        else if (m_activeQuality == NextSaturationPlugin::quality2x)
        {
            auto up = m_oversampling2x.processSamplesUp(active);
            processSaturationBlock(up, m_toneFilter2x, chunkInputGainStart, chunkInputGainEnd, chunkDriveGainStart, chunkDriveGainEnd, chunkBiasStart, chunkBiasEnd, chunkToneStart, chunkToneEnd, m_activeMode);
            m_oversampling2x.processSamplesDown(active);
        }
        else
        {
            auto up = m_oversampling4x.processSamplesUp(active);
            processSaturationBlock(up, m_toneFilter4x, chunkInputGainStart, chunkInputGainEnd, chunkDriveGainStart, chunkDriveGainEnd, chunkBiasStart, chunkBiasEnd, chunkToneStart, chunkToneEnd, m_activeMode);
            m_oversampling4x.processSamplesDown(active);
        }

        const float mixDelta = numSamples > 1 ? (mixEnd - mixStart) / (float)(numSamples - 1) : 0.0f;
        const float outputGainDelta = numSamples > 1 ? (outputGainEnd - outputGainStart) / (float)(numSamples - 1) : 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto *wet = fc.destBuffer->getWritePointer(ch, chunkStart);
            const auto *dry = m_dryBuffer.getReadPointer(ch, 0);
            float mix = mixStart + mixDelta * (float)processed;
            float outputGain = outputGainStart + outputGainDelta * (float)processed;

            for (int i = 0; i < chunkSize; ++i)
            {
                float transitionWet = 1.0f;

                if (m_transitionStage == TransitionStage::fadeOut)
                {
                    transitionWet = (float)m_transitionSamplesRemaining / (float)juce::jmax(1, m_transitionLengthSamples);
                    if (--m_transitionSamplesRemaining <= 0)
                    {
                        m_activeMode = m_targetMode;
                        m_activeQuality = m_targetQuality;
                        m_transitionStage = TransitionStage::fadeIn;
                        m_transitionSamplesRemaining = m_transitionLengthSamples;

                        m_oversampling2x.reset();
                        m_oversampling4x.reset();
                        m_toneFilter1x.reset();
                        m_toneFilter2x.reset();
                        m_toneFilter4x.reset();
                    }
                }
                else if (m_transitionStage == TransitionStage::fadeIn)
                {
                    transitionWet = 1.0f - ((float)m_transitionSamplesRemaining / (float)juce::jmax(1, m_transitionLengthSamples));
                    if (--m_transitionSamplesRemaining <= 0)
                    {
                        m_transitionStage = TransitionStage::none;
                        m_transitionSamplesRemaining = 0;
                        transitionWet = 1.0f;
                    }
                }

                const float effectiveMix = juce::jlimit(0.0f, 1.0f, mix * transitionWet);
                const float dryGain = std::cos(0.5f * juce::MathConstants<float>::pi * effectiveMix);
                const float wetGain = std::sin(0.5f * juce::MathConstants<float>::pi * effectiveMix);
                wet[i] = (dry[i] * dryGain + wet[i] * wetGain) * outputGain;

                mix += mixDelta;
                outputGain += outputGainDelta;
            }
        }

        processed += chunkSize;
    }

    m_outputMeasurer.processBuffer(*fc.destBuffer, startSample, numSamples);
    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void NextSaturationPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, m_inputValue, m_driveValue, m_mixValue, m_outputValue, m_toneValue, m_biasValue, m_modeValue, m_qualityValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();

    updateAtomics();
}

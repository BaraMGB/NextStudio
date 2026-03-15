#include "Plugins/PeakLimiter/PeakLimiterPlugin.h"

#include <cmath>

namespace
{
constexpr float minInputGainDb = -24.0f;
constexpr float maxInputGainDb = 24.0f;
constexpr float minCeilingDb = -12.0f;
constexpr float maxCeilingDb = -0.01f;
constexpr float minReleaseMs = 5.0f;
constexpr float maxReleaseMs = 500.0f;

float gainToDb(float gain) { return juce::Decibels::gainToDecibels(juce::jmax(gain, 0.0000001f), -100.0f); }
} // namespace

void PeakLimiterPlugin::SlidingMaxQueue::prepare(int capacity)
{
    values.assign((size_t)juce::jmax(1, capacity), 0.0f);
    indices.assign(values.size(), 0);
    reset();
}

void PeakLimiterPlugin::SlidingMaxQueue::reset()
{
    head = 0;
    tail = 0;
}

int PeakLimiterPlugin::SlidingMaxQueue::positionFor(int64_t logicalIndex) const
{
    const auto size = (int)values.size();
    return size > 0 ? (int)(logicalIndex % size) : 0;
}

void PeakLimiterPlugin::SlidingMaxQueue::push(int64_t index, float value)
{
    if (values.empty())
        return;

    while (tail > head)
    {
        const auto lastPosition = positionFor(tail - 1);
        if (values[(size_t)lastPosition] >= value)
            break;

        --tail;
    }

    if (tail - head >= (int)values.size())
        ++head;

    const auto insertPosition = positionFor(tail);
    values[(size_t)insertPosition] = value;
    indices[(size_t)insertPosition] = index;
    ++tail;
}

void PeakLimiterPlugin::SlidingMaxQueue::prune(int64_t minIndex)
{
    while (tail > head)
    {
        const auto headPosition = positionFor(head);
        if (indices[(size_t)headPosition] >= minIndex)
            break;

        ++head;
    }
}

float PeakLimiterPlugin::SlidingMaxQueue::getMax() const
{
    if (tail <= head || values.empty())
        return 0.0f;

    return values[(size_t)positionFor(head)];
}

PeakLimiterPlugin::PeakLimiterPlugin(te::PluginCreationInfo info)
    : te::Plugin(info)
{
    auto *undoManager = getUndoManager();

    m_inputGainValue.referTo(state, inputGainParamID, undoManager, 0.0f);
    m_inputGainParam = addParam(inputGainParamID, "Input", {minInputGainDb, maxInputGainDb}, [](float value) { return juce::String(value, 1) + " dB"; }, [](const juce::String &text) { return juce::jlimit(minInputGainDb, maxInputGainDb, text.getFloatValue()); });
    m_inputGainParam->attachToCurrentValue(m_inputGainValue);

    m_ceilingValue.referTo(state, ceilingParamID, undoManager, -0.3f);
    m_ceilingParam = addParam(ceilingParamID, "Ceiling", {minCeilingDb, maxCeilingDb}, [](float value) { return juce::String(value, 2) + " dB"; }, [](const juce::String &text) { return juce::jlimit(minCeilingDb, maxCeilingDb, text.getFloatValue()); });
    m_ceilingParam->attachToCurrentValue(m_ceilingValue);

    m_releaseValue.referTo(state, releaseParamID, undoManager, 80.0f);
    m_releaseParam = addParam(releaseParamID, "Release", {minReleaseMs, maxReleaseMs, 0.0f, 0.35f}, [](float value) { return juce::String(value, 1) + " ms"; }, [](const juce::String &text) { return juce::jlimit(minReleaseMs, maxReleaseMs, text.getFloatValue()); });
    m_releaseParam->attachToCurrentValue(m_releaseValue);

    m_linkChannelsValue.referTo(state, linkChannelsParamID, undoManager, 1.0f);
    m_linkChannelsParam = addParam(
        linkChannelsParamID, "Stereo Link", {0.0f, 1.0f, 1.0f}, [](float value) { return value >= 0.5f ? juce::String("On") : juce::String("Off"); },
        [](const juce::String &text)
        {
            const auto normalized = text.trim().toLowerCase();
            return normalized == "off" || normalized == "0" ? 0.0f : 1.0f;
        });
    m_linkChannelsParam->attachToCurrentValue(m_linkChannelsValue);

    state.addListener(this);
    updateAtomics();
    updateDerivedParameters();
}

PeakLimiterPlugin::~PeakLimiterPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    m_inputGainParam->detachFromCurrentValue();
    m_ceilingParam->detachFromCurrentValue();
    m_releaseParam->detachFromCurrentValue();
    m_linkChannelsParam->detachFromCurrentValue();
}

double PeakLimiterPlugin::getLatencySeconds() { return m_sampleRate > 0.0 ? (double)m_lookAheadSamples / m_sampleRate : 0.0; }

int PeakLimiterPlugin::calculateLookAheadSamples(float lookAheadMs) const { return juce::jlimit(0, juce::jmax(0, m_delayBufferSize - 1), (int)std::round(lookAheadMs * m_sampleRate / 1000.0)); }

void PeakLimiterPlugin::pushDetectionSample(int channel, int64_t sampleIndex, float absolutePeak)
{
    auto &queue = m_peakQueues[(size_t)channel];
    queue.push(sampleIndex, absolutePeak);
    queue.prune(sampleIndex - m_lookAheadSamples);
}

void PeakLimiterPlugin::updateGainFromDetectors(bool linkedChannels, int numChannels)
{
    if (linkedChannels && numChannels > 1)
    {
        const auto futurePeak = juce::jmax(m_peakQueues[0].getMax(), m_peakQueues[1].getMax());
        const auto requiredGain = futurePeak > m_ceilingLinear && futurePeak > 0.0f ? (m_ceilingLinear / futurePeak) : 1.0f;

        if (requiredGain < m_currentGain[0])
            m_currentGain[0] = requiredGain;
        else
            m_currentGain[0] = requiredGain + (m_currentGain[0] - requiredGain) * m_releaseCoeff;

        m_currentGain[1] = m_currentGain[0];
        return;
    }

    const auto activeChannels = juce::jlimit(1, maxChannels, numChannels);
    for (int ch = 0; ch < activeChannels; ++ch)
    {
        const auto futurePeak = m_peakQueues[(size_t)ch].getMax();
        const auto requiredGain = futurePeak > m_ceilingLinear && futurePeak > 0.0f ? (m_ceilingLinear / futurePeak) : 1.0f;

        if (requiredGain < m_currentGain[(size_t)ch])
            m_currentGain[(size_t)ch] = requiredGain;
        else
            m_currentGain[(size_t)ch] = requiredGain + (m_currentGain[(size_t)ch] - requiredGain) * m_releaseCoeff;
    }
}

void PeakLimiterPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    m_sampleRate = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
    const auto maxBlockSize = juce::jmax(1, info.blockSizeSamples);

    const auto maxLookAheadSamples = (int)std::ceil(PeakLimiterPlugin::fixedLookAheadMs * m_sampleRate / 1000.0);
    m_delayBufferSize = juce::jmax(maxLookAheadSamples + maxBlockSize + delaySafetySamples, 512);

    for (auto &buffer : m_delayBuffers)
        buffer.assign((size_t)m_delayBufferSize, 0.0f);

    const auto queueCapacity = juce::jmax(maxLookAheadSamples + maxBlockSize + delaySafetySamples, 128);
    for (auto &queue : m_peakQueues)
        queue.prepare(queueCapacity);

    m_lookAheadSamples = calculateLookAheadSamples(PeakLimiterPlugin::fixedLookAheadMs);
    reset();
}

void PeakLimiterPlugin::deinitialise() { reset(); }

void PeakLimiterPlugin::reset()
{
    for (auto &buffer : m_delayBuffers)
        std::fill(buffer.begin(), buffer.end(), 0.0f);

    for (auto &queue : m_peakQueues)
        queue.reset();

    m_currentGain.fill(1.0f);
    m_writePos = 0;
    m_sampleCounter = 0;
    m_wasEnabled = isEnabled();
    m_inputPeakDb.store(-100.0f, std::memory_order_relaxed);
    m_outputPeakDb.store(-100.0f, std::memory_order_relaxed);
    m_gainReductionDb.store(0.0f, std::memory_order_relaxed);
}

void PeakLimiterPlugin::midiPanic() { reset(); }

void PeakLimiterPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

void PeakLimiterPlugin::updateAtomics()
{
    m_audioParams.inputGainDb.store(juce::jlimit(minInputGainDb, maxInputGainDb, m_inputGainValue.get()), std::memory_order_relaxed);
    m_audioParams.ceilingDb.store(juce::jlimit(minCeilingDb, maxCeilingDb, m_ceilingValue.get()), std::memory_order_relaxed);
    m_audioParams.releaseMs.store(juce::jlimit(minReleaseMs, maxReleaseMs, m_releaseValue.get()), std::memory_order_relaxed);
    m_audioParams.linkChannels.store(m_linkChannelsValue.get() >= 0.5f ? 1 : 0, std::memory_order_relaxed);
}

void PeakLimiterPlugin::updateDerivedParameters()
{
    m_inputGainLinear = juce::Decibels::decibelsToGain(m_audioParams.inputGainDb.load(std::memory_order_relaxed));
    m_ceilingLinear = juce::Decibels::decibelsToGain(m_audioParams.ceilingDb.load(std::memory_order_relaxed));
    m_releaseCoeff = computeReleaseCoeff(m_audioParams.releaseMs.load(std::memory_order_relaxed), m_sampleRate);
}

float PeakLimiterPlugin::computeReleaseCoeff(float releaseMs, double sampleRate) const
{
    const auto effectiveReleaseMs = juce::jmax(minReleaseMs, releaseMs);
    const auto sr = juce::jmax(1.0, sampleRate);
    return std::exp(-1.0f / (0.001f * effectiveReleaseMs * (float)sr));
}

void PeakLimiterPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0)
        return;

    if (fc.destBuffer->getNumChannels() <= 0)
        return;

    const int startSample = juce::jlimit(0, fc.destBuffer->getNumSamples(), fc.bufferStartSample);
    const int availableSamples = fc.destBuffer->getNumSamples() - startSample;
    const int numSamples = juce::jmin(fc.bufferNumSamples, availableSamples);
    if (numSamples <= 0)
        return;

    updateDerivedParameters();

    const int numChannels = fc.destBuffer->getNumChannels();
    const bool isBypassed = !isEnabled();

    if (numChannels > maxChannels)
    {
        // NextStudio is stereo-only for now. Internal plugins are not expected
        // to process more than two channels.
        jassertfalse;
        return;
    }

    std::array<float *, maxChannels> channelData{nullptr, nullptr};
    for (int ch = 0; ch < numChannels; ++ch)
        channelData[(size_t)ch] = fc.destBuffer->getWritePointer(ch, startSample);

    if (isBypassed)
    {
        float blockInputPeak = 0.0f;
        float blockOutputPeak = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto in = channelData[(size_t)ch][sample];
                const auto metered = std::abs(in * m_inputGainLinear);
                blockInputPeak = juce::jmax(blockInputPeak, metered);
                blockOutputPeak = juce::jmax(blockOutputPeak, std::abs(in));
            }
        }

        m_inputPeakDb.store(gainToDb(blockInputPeak), std::memory_order_relaxed);
        m_outputPeakDb.store(gainToDb(blockOutputPeak), std::memory_order_relaxed);
        m_gainReductionDb.store(0.0f, std::memory_order_relaxed);
        m_wasEnabled = false;
        return;
    }

    if (!m_wasEnabled)
    {
        // Enabling is expected to be a manual UI action (not automated). We reset
        // state for deterministic behavior instead of smoothing a rare transition.
        reset();
    }

    m_wasEnabled = true;

    const bool linkedChannels = m_audioParams.linkChannels.load(std::memory_order_relaxed) != 0;
    float blockInputPeak = 0.0f;
    float blockOutputPeak = 0.0f;
    float blockGainReductionDb = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto in = channelData[(size_t)ch][sample] * m_inputGainLinear;
            const auto detectedPeak = std::abs(in);
            blockInputPeak = juce::jmax(blockInputPeak, detectedPeak);
            pushDetectionSample(ch, m_sampleCounter, detectedPeak);
            m_delayBuffers[(size_t)ch][(size_t)m_writePos] = in;
        }

        updateGainFromDetectors(linkedChannels, numChannels);

        const auto readPos = (m_writePos - m_lookAheadSamples + m_delayBufferSize) % m_delayBufferSize;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto out = m_delayBuffers[(size_t)ch][(size_t)readPos] * m_currentGain[(size_t)ch];
            channelData[(size_t)ch][sample] = out;

            blockOutputPeak = juce::jmax(blockOutputPeak, std::abs(out));
            blockGainReductionDb = juce::jmax(blockGainReductionDb, -gainToDb(m_currentGain[(size_t)ch]));
        }

        m_writePos = (m_writePos + 1) % m_delayBufferSize;
        ++m_sampleCounter;
    }

    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
    m_inputPeakDb.store(gainToDb(blockInputPeak), std::memory_order_relaxed);
    m_outputPeakDb.store(gainToDb(blockOutputPeak), std::memory_order_relaxed);
    m_gainReductionDb.store(blockGainReductionDb, std::memory_order_relaxed);
}

void PeakLimiterPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, m_inputGainValue, m_ceilingValue, m_releaseValue, m_linkChannelsValue);

    for (auto parameter : getAutomatableParameters())
        parameter->updateFromAttachedValue();

    updateAtomics();
    reset();
}

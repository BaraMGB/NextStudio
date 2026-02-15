#include "Plugins/Chorus/NextChorusPlugin.h"

#include <cmath>

namespace
{
// Keep parameter changes smooth to avoid zipper noise during automation.
constexpr float smoothingTimeSeconds = 0.03f;
} // namespace

NextChorusPlugin::NextChorusPlugin(te::PluginCreationInfo info)
    : te::Plugin(info),
      m_delayLine((int)std::ceil((NextChorusPlugin::baseDelayMs + NextChorusPlugin::maxDepthMs) * 44100.0f / 1000.0f) + 8)
{
    auto *um = getUndoManager();

    // Keep the same state keys as the classic chorus where it makes sense,
    // so old presets map naturally when copied over.
    m_depthMsValue.referTo(state, NextChorusPlugin::depthMsParamID, um, 3.0f);
    m_depthMsParam = addParam(NextChorusPlugin::depthMsParamID, "Depth", {minDepthMs, maxDepthMs}, [](float v) { return juce::String(v, 1) + " ms"; }, [](const juce::String &s) { return juce::jlimit(minDepthMs, maxDepthMs, s.getFloatValue()); });
    m_depthMsParam->attachToCurrentValue(m_depthMsValue);

    m_speedHzValue.referTo(state, NextChorusPlugin::speedHzParamID, um, 1.0f);
    m_speedHzParam = addParam(NextChorusPlugin::speedHzParamID, "Rate", {minSpeedHz, maxSpeedHz, 0.0f, 0.35f}, [](float v) { return juce::String(v, 2) + " Hz"; }, [](const juce::String &s) { return juce::jlimit(minSpeedHz, maxSpeedHz, s.getFloatValue()); });
    m_speedHzParam->attachToCurrentValue(m_speedHzValue);

    m_widthValue.referTo(state, NextChorusPlugin::widthParamID, um, 0.5f);
    m_widthParam = addParam(NextChorusPlugin::widthParamID, "Width", {0.0f, 1.0f}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(0.0f, 1.0f, s.getFloatValue() / 100.0f); });
    m_widthParam->attachToCurrentValue(m_widthValue);

    m_mixProportionValue.referTo(state, NextChorusPlugin::mixProportionParamID, um, 0.5f);
    m_mixProportionParam = addParam(NextChorusPlugin::mixProportionParamID, "Mix", {0.0f, 1.0f}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(0.0f, 1.0f, s.getFloatValue() / 100.0f); });
    m_mixProportionParam->attachToCurrentValue(m_mixProportionValue);

    state.addListener(this);
    updateAtomics();
}

NextChorusPlugin::~NextChorusPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    m_depthMsParam->detachFromCurrentValue();
    m_speedHzParam->detachFromCurrentValue();
    m_widthParam->detachFromCurrentValue();
    m_mixProportionParam->detachFromCurrentValue();
}

void NextChorusPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    const auto sr = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = juce::jmax((juce::uint32)64, (juce::uint32)juce::jmax(1, info.blockSizeSamples));
    spec.numChannels = maxChannels;

    const int maxDelaySamples = (int)std::ceil((baseDelayMs + maxDepthMs) * (float)sr / 1000.0f) + 8;
    m_delayLine.setMaximumDelayInSamples(maxDelaySamples);
    m_delayLine.prepare(spec);

    m_depthMsSmoothed.reset(sr, smoothingTimeSeconds);
    m_speedHzSmoothed.reset(sr, smoothingTimeSeconds);
    m_widthSmoothed.reset(sr, smoothingTimeSeconds);
    m_mixSmoothed.reset(sr, smoothingTimeSeconds);

    reset();
}

void NextChorusPlugin::deinitialise() { m_delayLine.reset(); }

void NextChorusPlugin::reset()
{
    m_delayLine.reset();

    m_depthMsSmoothed.setCurrentAndTargetValue(juce::jlimit(minDepthMs, maxDepthMs, m_audioParams.depthMs.load(std::memory_order_relaxed)));
    m_speedHzSmoothed.setCurrentAndTargetValue(juce::jlimit(minSpeedHz, maxSpeedHz, m_audioParams.speedHz.load(std::memory_order_relaxed)));
    m_widthSmoothed.setCurrentAndTargetValue(juce::jlimit(0.0f, 1.0f, m_audioParams.width.load(std::memory_order_relaxed)));
    m_mixSmoothed.setCurrentAndTargetValue(juce::jlimit(0.0f, 1.0f, m_audioParams.mixProportion.load(std::memory_order_relaxed)));
    m_phase = 0.0f;
}

void NextChorusPlugin::midiPanic() { reset(); }

void NextChorusPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

void NextChorusPlugin::updateAtomics()
{
    m_audioParams.depthMs.store(m_depthMsValue.get(), std::memory_order_relaxed);
    m_audioParams.speedHz.store(m_speedHzValue.get(), std::memory_order_relaxed);
    m_audioParams.width.store(m_widthValue.get(), std::memory_order_relaxed);
    m_audioParams.mixProportion.store(m_mixProportionValue.get(), std::memory_order_relaxed);
}

void NextChorusPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0 || !isEnabled())
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

    m_depthMsSmoothed.setTargetValue(juce::jlimit(minDepthMs, maxDepthMs, m_audioParams.depthMs.load(std::memory_order_relaxed)));
    m_speedHzSmoothed.setTargetValue(juce::jlimit(minSpeedHz, maxSpeedHz, m_audioParams.speedHz.load(std::memory_order_relaxed)));
    m_widthSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, m_audioParams.width.load(std::memory_order_relaxed)));
    m_mixSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, m_audioParams.mixProportion.load(std::memory_order_relaxed)));

    const float sr = juce::jmax(1.0f, (float)sampleRate);
    const float maxDelaySamples = (float)m_delayLine.getMaximumDelayInSamples();
    const float baseDelaySamples = baseDelayMs * sr / 1000.0f;

    auto *left = fc.destBuffer->getWritePointer(0, startSample);
    auto *right = numChannels > 1 ? fc.destBuffer->getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float depthMs = m_depthMsSmoothed.getNextValue();
        const float speedHz = m_speedHzSmoothed.getNextValue();
        const float width = m_widthSmoothed.getNextValue();
        const float mix = m_mixSmoothed.getNextValue();
        const float dry = 1.0f - mix;

        const float depthSamples = depthMs * sr / 1000.0f;
        const float lfoScale = 0.5f * depthSamples;
        // Width controls phase offset between L and R LFOs (0..pi).
        const float phaseOffset = juce::MathConstants<float>::pi * width;

        // Delay is base + bipolar LFO mapped into a positive depth range.
        const float delaySamplesL = juce::jlimit(1.0f, maxDelaySamples, baseDelaySamples + lfoScale * (1.0f + std::sin(m_phase)));
        const float delaySamplesR = juce::jlimit(1.0f, maxDelaySamples, baseDelaySamples + lfoScale * (1.0f + std::sin(m_phase + phaseOffset)));

        const float inL = left[i];
        // Mono inputs are mirrored so the chorus path still runs stereo-safe.
        const float inR = right != nullptr ? right[i] : inL;

        const float wetL = m_delayLine.popSample(0, delaySamplesL);
        const float wetR = right != nullptr ? m_delayLine.popSample(1, delaySamplesR) : wetL;

        m_delayLine.pushSample(0, inL);
        if (right != nullptr)
            m_delayLine.pushSample(1, inR);

        left[i] = inL * dry + wetL * mix;
        if (right != nullptr)
            right[i] = inR * dry + wetR * mix;

        m_phase += juce::MathConstants<float>::twoPi * speedHz / sr;
        if (m_phase >= juce::MathConstants<float>::twoPi)
            m_phase -= juce::MathConstants<float>::twoPi;
    }

    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void NextChorusPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, m_depthMsValue, m_speedHzValue, m_widthValue, m_mixProportionValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();

    updateAtomics();
}

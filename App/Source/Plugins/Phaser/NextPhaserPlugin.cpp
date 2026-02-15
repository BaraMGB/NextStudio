#include "Plugins/Phaser/NextPhaserPlugin.h"

NextPhaserPlugin::NextPhaserPlugin(te::PluginCreationInfo info)
    : te::Plugin(info)
{
    auto *um = getUndoManager();

    m_depthValue.referTo(state, NextPhaserPlugin::depthParamID, um, 0.55f);
    m_depthParam = addParam(NextPhaserPlugin::depthParamID, "Depth", {minDepth, maxDepth}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minDepth, maxDepth, s.getFloatValue() / 100.0f); });
    m_depthParam->attachToCurrentValue(m_depthValue);

    m_rateValue.referTo(state, NextPhaserPlugin::rateParamID, um, 0.45f);
    m_rateParam = addParam(NextPhaserPlugin::rateParamID, "Rate", {minRateHz, maxRateHz, 0.0f, 0.35f}, [](float v) { return juce::String(v, 2) + " Hz"; }, [](const juce::String &s) { return juce::jlimit(minRateHz, maxRateHz, s.getFloatValue()); });
    m_rateParam->attachToCurrentValue(m_rateValue);

    m_feedbackValue.referTo(state, NextPhaserPlugin::feedbackParamID, um, 0.35f);
    m_feedbackParam = addParam(NextPhaserPlugin::feedbackParamID, "Feedback", {minFeedback, maxFeedback}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minFeedback, maxFeedback, s.getFloatValue() / 100.0f); });
    m_feedbackParam->attachToCurrentValue(m_feedbackValue);

    m_mixValue.referTo(state, NextPhaserPlugin::mixParamID, um, 0.6f);
    m_mixParam = addParam(NextPhaserPlugin::mixParamID, "Mix", {minMix, maxMix}, [](float v) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }, [](const juce::String &s) { return juce::jlimit(minMix, maxMix, s.getFloatValue() / 100.0f); });
    m_mixParam->attachToCurrentValue(m_mixValue);

    state.addListener(this);
    updateAtomics();
}

NextPhaserPlugin::~NextPhaserPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    m_depthParam->detachFromCurrentValue();
    m_rateParam->detachFromCurrentValue();
    m_feedbackParam->detachFromCurrentValue();
    m_mixParam->detachFromCurrentValue();
}

void NextPhaserPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
    spec.maximumBlockSize = juce::jmax((juce::uint32)64, (juce::uint32)juce::jmax(1, info.blockSizeSamples));
    spec.numChannels = 2;

    m_phaser.prepare(spec);
    reset();
}

void NextPhaserPlugin::deinitialise() { m_phaser.reset(); }

void NextPhaserPlugin::reset() { m_phaser.reset(); }

void NextPhaserPlugin::midiPanic() { reset(); }

void NextPhaserPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

void NextPhaserPlugin::updateAtomics()
{
    m_audioParams.depth.store(m_depthValue.get(), std::memory_order_relaxed);
    m_audioParams.rateHz.store(m_rateValue.get(), std::memory_order_relaxed);
    m_audioParams.feedback.store(m_feedbackValue.get(), std::memory_order_relaxed);
    m_audioParams.mix.store(m_mixValue.get(), std::memory_order_relaxed);
}

void NextPhaserPlugin::applyToBuffer(const te::PluginRenderContext &fc)
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

    m_phaser.setCentreFrequency(1200.0f);
    m_phaser.setRate(juce::jlimit(minRateHz, maxRateHz, m_audioParams.rateHz.load(std::memory_order_relaxed)));
    m_phaser.setDepth(juce::jlimit(minDepth, maxDepth, m_audioParams.depth.load(std::memory_order_relaxed)));
    m_phaser.setFeedback(juce::jlimit(minFeedback, maxFeedback, m_audioParams.feedback.load(std::memory_order_relaxed)));
    m_phaser.setMix(juce::jlimit(minMix, maxMix, m_audioParams.mix.load(std::memory_order_relaxed)));

    juce::dsp::AudioBlock<float> block(*fc.destBuffer);
    auto active = block.getSubsetChannelBlock(0, (size_t)numChannels).getSubBlock((size_t)startSample, (size_t)numSamples);
    juce::dsp::ProcessContextReplacing<float> context(active);
    m_phaser.process(context);

    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void NextPhaserPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, m_depthValue, m_rateValue, m_feedbackValue, m_mixValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();

    updateAtomics();
}

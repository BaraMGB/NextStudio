#include "Plugins/Filter/NextFilterPlugin.h"

#include <cmath>

namespace
{
constexpr float smoothingTimeSeconds = 0.03f;
}

NextFilterPlugin::NextFilterPlugin(te::PluginCreationInfo info)
    : te::Plugin(info)
{
    auto *um = getUndoManager();

    m_freqValue.referTo(state, NextFilterPlugin::frequencyParamID, um, 1400.0f);
    m_freqParam = addParam(NextFilterPlugin::frequencyParamID, "Frequency", {minFrequency, maxFrequency, 0.0f, 0.22f}, [](float v) { return juce::String(juce::roundToInt(v)) + " Hz"; }, [](const juce::String &s) { return s.getFloatValue(); });
    m_freqParam->attachToCurrentValue(m_freqValue);

    m_resValue.referTo(state, NextFilterPlugin::resonanceParamID, um, 0.71f);
    m_resParam = addParam(NextFilterPlugin::resonanceParamID, "Resonance", {minResonance, maxResonance}, [](float v) { return juce::String(v, 2); }, [](const juce::String &s) { return s.getFloatValue(); });
    m_resParam->attachToCurrentValue(m_resValue);

    m_modeValue.referTo(state, NextFilterPlugin::modeParamID, um, (float)NextFilterPlugin::lowpass);
    m_modeParam = addParam(NextFilterPlugin::modeParamID, "Mode", {0.0f, 1.0f, 1.0f}, [](float v) { return NextFilterPlugin::modeToText((int)std::round(v)); }, [](const juce::String &s) { return (float)NextFilterPlugin::textToMode(s); });
    m_modeParam->attachToCurrentValue(m_modeValue);

    m_slopeValue.referTo(state, NextFilterPlugin::slopeParamID, um, (float)NextFilterPlugin::slope24);
    m_slopeParam = addParam(NextFilterPlugin::slopeParamID, "Slope", {0.0f, 3.0f, 1.0f}, [](float v) { return NextFilterPlugin::slopeToText((int)std::round(v)); }, [](const juce::String &s) { return (float)NextFilterPlugin::textToSlope(s); });
    m_slopeParam->attachToCurrentValue(m_slopeValue);

    for (auto &channelFilters : m_filters)
        for (auto &filter : channelFilters)
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    state.addListener(this);
    updateAtomics();
}

NextFilterPlugin::~NextFilterPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    m_freqParam->detachFromCurrentValue();
    m_resParam->detachFromCurrentValue();
    m_modeParam->detachFromCurrentValue();
    m_slopeParam->detachFromCurrentValue();
}

juce::String NextFilterPlugin::modeToText(int mode) { return mode == NextFilterPlugin::highpass ? "Highpass" : "Lowpass"; }

int NextFilterPlugin::textToMode(const juce::String &text)
{
    const auto t = text.trim().toLowerCase();
    return t.startsWith("high") || t == "1" ? NextFilterPlugin::highpass : NextFilterPlugin::lowpass;
}

juce::String NextFilterPlugin::slopeToText(int slope)
{
    switch (juce::jlimit(0, 3, slope))
    {
    case NextFilterPlugin::slope12:
        return "12 dB";
    case NextFilterPlugin::slope24:
        return "24 dB";
    case NextFilterPlugin::slope36:
        return "36 dB";
    case NextFilterPlugin::slope48:
        return "48 dB";
    default:
        return "24 dB";
    }
}

int NextFilterPlugin::textToSlope(const juce::String &text)
{
    const auto t = text.trim().toLowerCase();
    if (t.startsWith("48") || t == "3")
        return NextFilterPlugin::slope48;
    if (t.startsWith("36") || t == "2")
        return NextFilterPlugin::slope36;
    if (t.startsWith("24") || t == "1")
        return NextFilterPlugin::slope24;
    return NextFilterPlugin::slope12;
}

int NextFilterPlugin::slopeToStageCount(int slope) { return juce::jlimit(1, maxStages, slope + 1); }

void NextFilterPlugin::updateAtomics()
{
    m_audioParams.frequency.store(juce::jlimit(minFrequency, maxFrequency, m_freqValue.get()), std::memory_order_relaxed);
    m_audioParams.resonance.store(juce::jlimit(minResonance, maxResonance, m_resValue.get()), std::memory_order_relaxed);
    m_audioParams.mode.store(juce::jlimit(0, 1, (int)std::round(m_modeValue.get())), std::memory_order_relaxed);
    m_audioParams.slope.store(juce::jlimit(0, 3, (int)std::round(m_slopeValue.get())), std::memory_order_relaxed);
}

void NextFilterPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    const double sr = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = juce::jmax((juce::uint32)64, (juce::uint32)juce::jmax(1, info.blockSizeSamples));
    spec.numChannels = maxChannels;

    for (auto &channelFilters : m_filters)
        for (auto &filter : channelFilters)
            filter.prepare(spec);

    m_freqSmoothed.reset(sr, smoothingTimeSeconds);
    m_resSmoothed.reset(sr, smoothingTimeSeconds);
    reset();
}

void NextFilterPlugin::deinitialise()
{
    for (auto &channelFilters : m_filters)
        for (auto &filter : channelFilters)
            filter.reset();
}

void NextFilterPlugin::reset()
{
    for (auto &channelFilters : m_filters)
        for (auto &filter : channelFilters)
            filter.reset();

    m_freqSmoothed.setCurrentAndTargetValue(juce::jlimit(minFrequency, maxFrequency, m_audioParams.frequency.load(std::memory_order_relaxed)));
    m_resSmoothed.setCurrentAndTargetValue(juce::jlimit(minResonance, maxResonance, m_audioParams.resonance.load(std::memory_order_relaxed)));
    m_lastAppliedMode = -1;
}

void NextFilterPlugin::midiPanic() { reset(); }

void NextFilterPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &)
{
    if (v == state)
        updateAtomics();
}

void NextFilterPlugin::applyToBuffer(const te::PluginRenderContext &fc)
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

    m_freqSmoothed.setTargetValue(juce::jlimit(minFrequency, maxFrequency, m_audioParams.frequency.load(std::memory_order_relaxed)));
    m_resSmoothed.setTargetValue(juce::jlimit(minResonance, maxResonance, m_audioParams.resonance.load(std::memory_order_relaxed)));

    const int mode = juce::jlimit(0, 1, m_audioParams.mode.load(std::memory_order_relaxed));
    const int stages = slopeToStageCount(juce::jlimit(0, 3, m_audioParams.slope.load(std::memory_order_relaxed)));
    const auto type = mode == NextFilterPlugin::highpass ? juce::dsp::StateVariableTPTFilterType::highpass : juce::dsp::StateVariableTPTFilterType::lowpass;

    if (mode != m_lastAppliedMode)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            for (int stage = 0; stage < maxStages; ++stage)
                m_filters[(size_t)ch][(size_t)stage].setType(type);

        m_lastAppliedMode = mode;
    }

    const bool smoothParamsPerSample = m_freqSmoothed.isSmoothing() || m_resSmoothed.isSmoothing();

    if (!smoothParamsPerSample)
    {
        const float freq = m_freqSmoothed.getCurrentValue();
        const float resonance = m_resSmoothed.getCurrentValue();

        for (int ch = 0; ch < numChannels; ++ch)
            for (int stage = 0; stage < stages; ++stage)
            {
                auto &filter = m_filters[(size_t)ch][(size_t)stage];
                filter.setCutoffFrequency(freq);
                filter.setResonance(resonance);
            }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto value = fc.destBuffer->getSample(ch, startSample + sample);

                for (int stage = 0; stage < stages; ++stage)
                    value = m_filters[(size_t)ch][(size_t)stage].processSample(ch, value);

                fc.destBuffer->setSample(ch, startSample + sample, value);
            }
        }

        te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
        return;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float freq = m_freqSmoothed.getNextValue();
        const float resonance = m_resSmoothed.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto value = fc.destBuffer->getSample(ch, startSample + sample);

            for (int stage = 0; stage < stages; ++stage)
            {
                auto &filter = m_filters[(size_t)ch][(size_t)stage];
                filter.setCutoffFrequency(freq);
                filter.setResonance(resonance);
                value = filter.processSample(ch, value);
            }

            fc.destBuffer->setSample(ch, startSample + sample, value);
        }
    }

    te::zeroDenormalisedValuesIfNeeded(*fc.destBuffer);
}

void NextFilterPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    te::copyPropertiesToCachedValues(v, m_freqValue, m_resValue, m_modeValue, m_slopeValue);

    for (auto p : getAutomatableParameters())
        p->updateFromAttachedValue();

    updateAtomics();
}

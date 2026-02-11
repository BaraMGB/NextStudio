#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPlugin.h"

#include <algorithm>
#include <cmath>
#include <numeric>

SpectrumAnalyzerPlugin::SpectrumAnalyzerPlugin(te::PluginCreationInfo info)
    : te::Plugin(info),
      m_fft(fftOrder),
      m_window(fftSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    std::array<float, fftSize> windowProbe{};
    windowProbe.fill(1.0f);
    m_window.multiplyWithWindowingTable(windowProbe.data(), fftSize);

    const float coherentGain = std::accumulate(windowProbe.begin(), windowProbe.end(), 0.0f) / (float)fftSize;
    m_fftMagnitudeToLinear = coherentGain > 0.0f ? 2.0f / ((float)fftSize * coherentGain) : 0.0f;

    for (auto &value : m_displayDb)
        value.store(minDb, std::memory_order_relaxed);

    rebuildBinMapping(m_currentSampleRate.load(std::memory_order_relaxed));
}

SpectrumAnalyzerPlugin::~SpectrumAnalyzerPlugin() { notifyListenersOfDeletion(); }

void SpectrumAnalyzerPlugin::initialise(const te::PluginInitialisationInfo &info)
{
    const double sr = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
    m_currentSampleRate.store(sr, std::memory_order_relaxed);
    rebuildBinMapping(sr);
    m_clearAnalysisRequested.store(true, std::memory_order_release);
    clearDisplaySpectrum();
}

void SpectrumAnalyzerPlugin::deinitialise() {}

void SpectrumAnalyzerPlugin::reset()
{
    m_clearAnalysisRequested.store(true, std::memory_order_release);
    clearDisplaySpectrum();
}

void SpectrumAnalyzerPlugin::midiPanic()
{
    m_clearAnalysisRequested.store(true, std::memory_order_release);
    clearDisplaySpectrum();
}

void SpectrumAnalyzerPlugin::restorePluginStateFromValueTree(const juce::ValueTree &)
{
    m_clearAnalysisRequested.store(true, std::memory_order_release);
    clearDisplaySpectrum();
    rebuildBinMapping(m_currentSampleRate.load(std::memory_order_relaxed));
}

void SpectrumAnalyzerPlugin::applyToBuffer(const te::PluginRenderContext &fc)
{
    if (!isEnabled())
        return;

    if (fc.destBuffer == nullptr || fc.bufferNumSamples <= 0)
        return;

    const int numChannels = fc.destBuffer->getNumChannels();
    if (numChannels <= 0)
        return;

    const int startSample = juce::jlimit(0, fc.destBuffer->getNumSamples(), fc.bufferStartSample);
    const int availableSamples = fc.destBuffer->getNumSamples() - startSample;
    const int numSamples = juce::jmin(fc.bufferNumSamples, availableSamples);
    if (numSamples <= 0)
        return;

    if (m_clearAnalysisRequested.exchange(false, std::memory_order_acq_rel))
        clearAnalysisState();

    const auto *const *channelData = fc.destBuffer->getArrayOfReadPointers();
    const float channelScale = 1.0f / (float)numChannels;

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        const int sampleIndex = startSample + i;

        for (int ch = 0; ch < numChannels; ++ch)
            mono += channelData[ch][sampleIndex];

        mono *= channelScale;
        pushNextSample(mono);
    }
}

void SpectrumAnalyzerPlugin::copySpectrum(std::array<float, numDisplayBins> &destination) const
{
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const uint32_t beginVersion = m_displayVersion.load(std::memory_order_acquire);
        if ((beginVersion & 1u) != 0u)
            continue;

        for (int i = 0; i < numDisplayBins; ++i)
            destination[(size_t)i] = m_displayDb[(size_t)i].load(std::memory_order_relaxed);

        const uint32_t endVersion = m_displayVersion.load(std::memory_order_acquire);
        if (beginVersion == endVersion)
            return;
    }

    for (int i = 0; i < numDisplayBins; ++i)
        destination[(size_t)i] = m_displayDb[(size_t)i].load(std::memory_order_relaxed);
}

double SpectrumAnalyzerPlugin::getCurrentSampleRate() const { return m_currentSampleRate.load(std::memory_order_relaxed); }

void SpectrumAnalyzerPlugin::pushNextSample(float sample) noexcept
{
    m_fifo[(size_t)m_fifoIndex++] = sample;

    if (m_fifoIndex < fftSize)
        return;

    std::copy(m_fifo.begin(), m_fifo.end(), m_fftData.begin());
    std::fill(m_fftData.begin() + fftSize, m_fftData.end(), 0.0f);
    processFftBlock();

    std::move(m_fifo.begin() + fftHopSize, m_fifo.end(), m_fifo.begin());
    m_fifoIndex = fftSize - fftHopSize;
}

void SpectrumAnalyzerPlugin::processFftBlock() noexcept
{
    const BinMapping *mapping = m_activeMapping.load(std::memory_order_acquire);
    const double sr = m_currentSampleRate.load(std::memory_order_relaxed);
    const double dt = sr > 0.0 ? (double)fftHopSize / sr : 0.0;
    const double attackTau = attackTimeMs * 0.001;
    const double releaseTau = releaseTimeMs * 0.001;
    const float attackAlpha = (attackTau > 0.0 && dt > 0.0) ? (float)(1.0 - std::exp(-dt / attackTau)) : 1.0f;
    const float releaseAlpha = (releaseTau > 0.0 && dt > 0.0) ? (float)(1.0 - std::exp(-dt / releaseTau)) : 1.0f;

    m_window.multiplyWithWindowingTable(m_fftData.data(), fftSize);
    m_fft.performFrequencyOnlyForwardTransform(m_fftData.data());

    m_displayVersion.fetch_add(1u, std::memory_order_release);

    for (int i = 0; i < numDisplayBins; ++i)
    {
        const int startBin = mapping->start[(size_t)i];
        const int endBin = mapping->end[(size_t)i];
        float peak = 0.0f;

        for (int bin = startBin; bin <= endBin; ++bin)
        {
            const float mag = m_fftData[(size_t)bin] * m_fftMagnitudeToLinear;
            if (mag > peak)
                peak = mag;
        }

        const float targetDb = juce::Decibels::gainToDecibels(peak, minDb);

        const float currentDb = m_displayDb[(size_t)i].load(std::memory_order_relaxed);
        const float alpha = targetDb > currentDb ? attackAlpha : releaseAlpha;
        const float smoothedDb = currentDb + (targetDb - currentDb) * alpha;

        m_displayDb[(size_t)i].store(smoothedDb, std::memory_order_relaxed);
    }

    m_displayVersion.fetch_add(1u, std::memory_order_release);
}

void SpectrumAnalyzerPlugin::clearAnalysisState() noexcept
{
    std::fill(m_fifo.begin(), m_fifo.end(), 0.0f);
    std::fill(m_fftData.begin(), m_fftData.end(), 0.0f);
    m_fifoIndex = 0;

    clearDisplaySpectrum();
}

void SpectrumAnalyzerPlugin::clearDisplaySpectrum() noexcept
{
    m_displayVersion.fetch_add(1u, std::memory_order_release);

    for (auto &value : m_displayDb)
        value.store(minDb, std::memory_order_relaxed);

    m_displayVersion.fetch_add(1u, std::memory_order_release);
}

void SpectrumAnalyzerPlugin::rebuildBinMapping(double sr)
{
    const double sampleRate = juce::jmax(1.0, sr);
    const double nyquist = sampleRate * 0.5;
    const double minFrequency = minDisplayFrequency;
    const double maxFrequency = juce::jmax(minFrequency, nyquist);
    const double binFrequency = sampleRate / (double)fftSize;

    BinMapping *const current = m_activeMapping.load(std::memory_order_acquire);
    BinMapping *const target = current == &m_mappingA ? &m_mappingB : &m_mappingA;

    for (int i = 0; i < numDisplayBins; ++i)
    {
        const double t0 = (double)i / (double)numDisplayBins;
        const double t1 = (double)(i + 1) / (double)numDisplayBins;

        const double f0 = minFrequency * std::pow(maxFrequency / minFrequency, t0);
        const double f1 = minFrequency * std::pow(maxFrequency / minFrequency, t1);

        int start = juce::jlimit(1, (fftSize / 2) - 1, (int)std::floor(f0 / binFrequency));
        int end = juce::jlimit(1, (fftSize / 2) - 1, (int)std::ceil(f1 / binFrequency));
        if (end < start)
            end = start;

        target->start[(size_t)i] = start;
        target->end[(size_t)i] = end;
    }

    m_activeMapping.store(target, std::memory_order_release);
}

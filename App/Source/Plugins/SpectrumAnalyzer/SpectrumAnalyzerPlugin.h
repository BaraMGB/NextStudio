#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>

namespace te = tracktion_engine;

class SpectrumAnalyzerPlugin : public te::Plugin
{
public:
    static constexpr const char *xmlTypeName = "spectrum_analyzer";
    static const char *getPluginName() { return "Spectrum Analyzer"; }

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int fftHopSize = fftSize / 4;
    static constexpr int numDisplayBins = 256;
    static constexpr float minDb = -96.0f;
    static constexpr double minDisplayFrequency = 10.0;
    static constexpr float attackTimeMs = 60.0f;
    static constexpr float releaseTimeMs = 200.0f;

    SpectrumAnalyzerPlugin(te::PluginCreationInfo info);
    ~SpectrumAnalyzerPlugin() override;

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return getPluginName(); }

    bool isSynth() override { return false; }
    bool takesMidiInput() override { return false; }
    bool takesAudioInput() override { return true; }
    bool producesAudioWhenNoAudioInput() override { return false; }

    void initialise(const te::PluginInitialisationInfo &) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const te::PluginRenderContext &) override;
    void midiPanic() override;
    void restorePluginStateFromValueTree(const juce::ValueTree &v) override;

    void copySpectrum(std::array<float, numDisplayBins> &destination) const;
    double getCurrentSampleRate() const;

private:
    void pushNextSample(float sample) noexcept;
    void processFftBlock() noexcept;
    void clearAnalysisState() noexcept;
    void clearDisplaySpectrum() noexcept;
    struct BinMapping
    {
        std::array<int, numDisplayBins> start{};
        std::array<int, numDisplayBins> end{};
    };

    void rebuildBinMapping(double sr);

    juce::dsp::FFT m_fft;
    juce::dsp::WindowingFunction<float> m_window;

    std::array<float, fftSize> m_fifo{};
    std::array<float, fftSize * 2> m_fftData{};
    int m_fifoIndex = 0;

    std::array<std::atomic<float>, numDisplayBins> m_displayDb{};
    std::atomic<uint32_t> m_displayVersion{0};
    BinMapping m_mappingA{};
    BinMapping m_mappingB{};
    std::atomic<BinMapping *> m_activeMapping{&m_mappingA};
    float m_fftMagnitudeToLinear = 0.0f;
    std::atomic<double> m_currentSampleRate{44100.0};
    std::atomic<bool> m_clearAnalysisRequested{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerPlugin)
};

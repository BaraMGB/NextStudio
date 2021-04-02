#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"

namespace te = tracktion_engine;

class AudioClipComponent : public ClipComponent

{
public:
    AudioClipComponent (EditViewState&, te::Clip::Ptr);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent &) override;

    te::WaveAudioClip* getWaveAudioClip();

private:

    void updateThumbnail();
    void drawWaveform(juce::Graphics& g,
                      te::AudioClipBase& c,
                      te::SmartThumbnail& thumb,
                      juce::Colour colour,
                       const int left, const int right, int y, int h, int xOffset);
    void drawChannels(juce::Graphics& g,
                      te::SmartThumbnail& thumb,
                      juce::Rectangle<int> area,
                      bool useHighRes,
                       te::EditTimeRange time, bool useLeft, bool useRight,
                       float leftGain, float rightGain);

    std::unique_ptr<te::SmartThumbnail> thumbnail;

    int m_mouseDownX {0};
    int m_clipWidthMouseDown;
    double m_lastOffset{0.0};
    double m_oldDistTime{0.0};
    tracktion_engine::ClipPosition m_posAtMouseDown;
};

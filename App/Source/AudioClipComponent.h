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


    te::WaveAudioClip* getWaveAudioClip();

private:

    void updateThumbnail();

    void drawWaveform(juce::Graphics& g,
                      te::AudioClipBase& c,
                      te::SmartThumbnail& thumb,
                      juce::Colour colour,
                      int left, int right, int y, int h, int xOffset);

    static void drawChannels(juce::Graphics& g,
                      te::SmartThumbnail& thumb,
                      juce::Rectangle<int> area,
                      bool useHighRes,
                      te::EditTimeRange time, bool useLeft, bool useRight,
                      float leftGain, float rightGain);

    std::unique_ptr<te::SmartThumbnail> thumbnail;

    //used for drawing thumbnail
    int getViewportOffset();
    int getViewportEnd();
    int getDrawingStartX();
    int getDrawingEndX();
    juce::Range<int> getDrawingRange();
    juce::Rectangle<int> getDrawingRect();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioClipComponent)
};

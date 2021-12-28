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
                      int left, int right, int y, int h, int xOffset);

    static void drawChannels(juce::Graphics& g,
                      te::SmartThumbnail& thumb,
                      juce::Rectangle<int> area,
                      bool useHighRes,
                      te::EditTimeRange time, bool useLeft, bool useRight,
                      float leftGain, float rightGain);

    std::unique_ptr<te::SmartThumbnail> thumbnail;

    //used for shrinking and expanding
    te::TimecodeSnapType getCurrentSnapType();
    double getDistanceInTime (int distanceInPixel);
    double getDistanceInTime(int distanceInPixel, bool snap);
    double clipEndSnaped();

    //used for drawing thumbnail
    int getViewportOffset();
    int getViewportEnd();
    int getDrawingStartX();
    int getDrawingEndX();
    juce::Range<int> getDrawingRange();
    juce::Rectangle<int> getDrawingRect();

    void setNewTimeAndOffset(double newTime, double newOffset);

    //helper
    static int invert(int value);
    int m_mouseDownX {0};
    int m_cachedClipWidth{};
    double m_oldDistanceTime{0.0};
    tracktion_engine::ClipPosition m_clipPosCached;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioClipComponent)
};

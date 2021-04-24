#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"

namespace te = tracktion_engine;


class RecordingClipComponent : public juce::Component,
                               private juce::Timer
{
public:
    RecordingClipComponent (te::Track::Ptr t, EditViewState&);

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    void updatePosition();
    void initialiseThumbnailAndPunchTime();
    void drawThumbnail (juce::Graphics& g, juce::Colour waveformColour) const;
    bool getBoundsAndTime (
            juce::Rectangle<int>& bounds, juce::Range<double>& times) const;

    int m_clipHeaderHight {10};

    te::Track::Ptr m_track;
    EditViewState& m_editViewState;

    te::RecordingThumbnailManager::Thumbnail::Ptr m_thumbnail;

    double m_punchInTime = -1.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RecordingClipComponent)
};

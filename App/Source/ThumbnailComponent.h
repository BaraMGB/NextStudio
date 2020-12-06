#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class ThumbnailComponent : public juce::Component
        , public juce::ChangeListener
{
public:

    ThumbnailComponent (EditViewState& evs);
    ~ThumbnailComponent ();

    void changeListenerCallback (juce::ChangeBroadcaster *source) override;

    void paint (juce::Graphics &g) override;
    void setFile (const juce::File& f);
    void resized () override;

    void thumbnailChanged ();
    void setDrawOffset (double drawOffset);
    void setDrawOffsetRight (double drawOffsetRight);

private:

    EditViewState & m_editViewState;
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> m_readerSource;
    juce::AudioThumbnailCache m_thumbnailCache;
    juce::AudioThumbnail m_thumbnail;
    double m_drawOffset, m_drawRightOffset;
};

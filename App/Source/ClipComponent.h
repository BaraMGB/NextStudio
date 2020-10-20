#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class ClipComponent : public Component
{
public:
    ClipComponent (EditViewState&, te::Clip::Ptr);

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent&) override;
    void mouseDrag(const MouseEvent &) override;
    void mouseUp(const MouseEvent &) override;

    te::Clip& getClip() { return *clip; }

protected:
    EditViewState& editViewState;
    te::Clip::Ptr clip;
private:
    double m_clipPosAtMouseDown;
    bool m_isDragging;
};




class ThumbnailComponent : public Component
                         , public juce::ChangeListener

{
public:
    ThumbnailComponent(EditViewState& evs)
        : editViewState(evs)
        , thumbnailCache(5)
        , thumbnail (256, formatManager, thumbnailCache)
    {
        setInterceptsMouseClicks (false, false);
        thumbnail.addChangeListener (this);
        formatManager.registerBasicFormats ();
    }
    ~ThumbnailComponent()
    {

    }
    void changeListenerCallback(ChangeBroadcaster *source) override
    {
        if (source == &thumbnail) { thumbnailChanged(); }
    }

    void thumbnailChanged()
    {
        repaint ();
    }

    void paint(Graphics &g) override
    {

        juce::Rectangle<int> thumbnailBounds( 0,10,getWidth (), getHeight () - 10);

        auto leftX = getBoundsInParent ().getX();
        auto rightX = getParentWidth () - (getBoundsInParent ().getX () + getBoundsInParent ().getWidth ());


        auto leftT = thumbnail.getTotalLength () * leftX / getParentWidth ();
        auto rightT = thumbnail.getTotalLength () * rightX / getParentWidth ();
        g.setColour (juce::Colours::black.withAlpha (0.7f));

        thumbnail.drawChannel (g,
                                thumbnailBounds,
                                0.0 + leftT,
                                thumbnail.getTotalLength() - rightT,
                                0,
                                1.0f);
    }

    void setFile(const juce::File& f)
    {
        auto* reader = formatManager.createReaderFor (f);

        if (reader != nullptr)
        {

            std::unique_ptr<juce::AudioFormatReaderSource> newSource (new juce::AudioFormatReaderSource (reader, true));
            thumbnail.setSource (new juce::FileInputSource (f));
            readerSource.reset (newSource.release());
        }
    }
    void resized() override
    {
        repaint ();
    }

private:




    EditViewState & editViewState;
    juce::AudioFormatManager formatManager;                    // [3]
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioThumbnailCache thumbnailCache;                  // [1]
    juce::AudioThumbnail thumbnail;
};



//==============================================================================
class AudioClipComponent : public ClipComponent


{
public:
    AudioClipComponent (EditViewState&, te::Clip::Ptr);

    te::WaveAudioClip* getWaveAudioClip() { return dynamic_cast<te::WaveAudioClip*> (clip.get()); }

    void paint (Graphics& g) override;
    void resized() override;

private:

    ThumbnailComponent thumbnailComponent;
};

//==============================================================================
class MidiClipComponent : public ClipComponent
{
public:
    MidiClipComponent (EditViewState&, te::Clip::Ptr);

    te::MidiClip* getMidiClip() { return dynamic_cast<te::MidiClip*> (clip.get()); }

    void paint (Graphics& g) override;
};

//==============================================================================
class RecordingClipComponent : public Component,
                               private Timer
{
public:
    RecordingClipComponent (te::Track::Ptr t, EditViewState&);

    void paint (Graphics& g) override;

private:
    void timerCallback() override;
    void updatePosition();
    void initialiseThumbnailAndPunchTime();
    void drawThumbnail (Graphics& g, Colour waveformColour) const;
    bool getBoundsAndTime (Rectangle<int>& bounds, Range<double>& times) const;

    int clipHeaderHight {10};

    te::Track::Ptr track;
    EditViewState& editViewState;

    te::RecordingThumbnailManager::Thumbnail::Ptr thumbnail;


    double punchInTime = -1.0;
};


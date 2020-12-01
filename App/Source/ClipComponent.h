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

    bool isCopying() const;
    void setIsCopying(bool isCopying);

    double getClickPosTime() const;
    void setClickPosTime(double clickPosTime);

    bool isShiftDown() const;

protected:
    EditViewState& editViewState;
    te::Clip::Ptr clip;
private:
    double m_clipPosAtMouseDown;
    double m_clickPosTime{0.0};
    bool m_isCopying{false};
    bool m_isDragging;
    bool m_isShiftDown{false};
};




class ThumbnailComponent : public Component
                         , public juce::ChangeListener

{
public:
    ThumbnailComponent(EditViewState& evs)
        : editViewState(evs)
        , thumbnailCache(5)
        , thumbnail (256, formatManager, thumbnailCache)
        , m_drawOffset(0.0)
        , m_drawRightOffset(0.0)
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

        juce::Rectangle<int> thumbnailBounds (0, 0, getWidth (), getHeight ());

        auto leftX = getBoundsInParent ().getX();
        auto rightX = getParentWidth () - (getBoundsInParent ().getX () + getBoundsInParent ().getWidth ());


        auto leftT = (thumbnail.getTotalLength () - m_drawOffset - m_drawRightOffset ) * leftX / getParentWidth ();
        auto rightT = (thumbnail.getTotalLength () - m_drawOffset - m_drawRightOffset) * rightX / getParentWidth ();
        g.setColour (juce::Colours::black.withAlpha (0.7f));

        thumbnail.drawChannel (g,
                                thumbnailBounds,
                                m_drawOffset + leftT,
                                thumbnail.getTotalLength() - rightT - m_drawRightOffset,
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

    void setDrawOffset(double drawOffset)
    {
        m_drawOffset = drawOffset;
    }

    void setDrawOffsetRight(double drawOffsetRight)
    {
        m_drawRightOffset = drawOffsetRight;
    }
private:




    EditViewState & editViewState;
    juce::AudioFormatManager formatManager;                    // [3]
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioThumbnailCache thumbnailCache;                  // [1]
    juce::AudioThumbnail thumbnail;
    double m_drawOffset, m_drawRightOffset;
};



//==============================================================================
class AudioClipComponent : public ClipComponent


{
public:
    AudioClipComponent (EditViewState&, te::Clip::Ptr);

    te::WaveAudioClip* getWaveAudioClip() { return dynamic_cast<te::WaveAudioClip*> (clip.get()); }

    void paint (Graphics& g) override;
    void resized() override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown (const MouseEvent&) override;
    void mouseDrag(const MouseEvent &) override;


private:

    int m_mouseDownX {0};
    int m_clipWidthMouseDown;
    double m_lastOffset{0.0};
    double m_oldDistTime{0.0};
    tracktion_engine::ClipPosition m_posAtMouseDown;

    ThumbnailComponent thumbnailComponent;
};

//==============================================================================
class MidiClipComponent : public ClipComponent
{
public:
    MidiClipComponent (EditViewState&, te::Clip::Ptr);

    te::MidiClip* getMidiClip() { return dynamic_cast<te::MidiClip*> (clip.get()); }

    void paint (Graphics& g) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown (const MouseEvent&) override;
    void mouseDrag(const MouseEvent &) override;

private:
    int m_mouseDownX {0};
    int m_clipWidthMouseDown;
    double m_lastOffset{0.0};
    double m_oldDistTime{0.0};
    tracktion_engine::ClipPosition m_posAtMouseDown;
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


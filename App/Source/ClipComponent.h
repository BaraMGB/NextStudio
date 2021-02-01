#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class SelectableClipClass : public tracktion_engine::SelectableClass
{
public:
    SelectableClipClass() {}
};


class ClipComponent : public juce::Component
{
public:
    ClipComponent (EditViewState&, te::Clip::Ptr);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &e) override;

    te::Clip& getClip () { return *m_clip; }

    bool isCopying () const;
    void setIsCopying (bool isCopying);

    double getClickPosTime () const;
    void setClickPosTime (double clickPosTime);

    bool isShiftDown () const;

protected:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;

    void showContextMenu();
private:
    double m_clipPosAtMouseDown{};
    double m_clickPosTime{0.0};
    bool m_isCopying{false};
    bool m_isDragging{};
    bool m_isShiftDown{false};
};

//==============================================================================

class AudioClipComponent : public ClipComponent


{
public:
    AudioClipComponent (EditViewState&, te::Clip::Ptr);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
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

//==============================================================================
class MidiClipComponent : public ClipComponent
                        , public juce::ChangeBroadcaster
{
public:
    MidiClipComponent (EditViewState&, te::Clip::Ptr);
    ~MidiClipComponent();

    te::MidiClip* getMidiClip()
    {
        return dynamic_cast<te::MidiClip*> (m_clip.get());
    }

    void paint (juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent &) override;

private:
    int m_mouseDownX {0};
    int m_clipWidthMouseDown;
    double m_oldDistTime{0.0};
    tracktion_engine::ClipPosition m_posAtMouseDown;
};

//==============================================================================
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
};

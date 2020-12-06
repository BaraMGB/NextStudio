#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "LevelMeterComponent.h"
#include "PluginRackComponent.h"

namespace te = tracktion_engine;

class TrackHeaderComponent : public juce::Component,
                             private te::ValueTreeAllEventListener
{
public:
    TrackHeaderComponent (EditViewState&, te::Track::Ptr);
    ~TrackHeaderComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    void resized() override;
    juce::Colour getTrackColour();

private:
    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    void showPopupMenu(te::AudioTrack* at);
    void deleteTrackFromEdit();

    EditViewState& editViewState;
    te::Track::Ptr m_track;
    int m_trackHeightATMouseDown;
    int m_yPosAtMouseDown;

    juce::ValueTree inputsState;
    juce::Label m_trackName;
    juce::ToggleButton m_armButton,
                 m_muteButton,
                 m_soloButton;

    juce::Slider       m_volumeKnob;
    std::unique_ptr<LevelMeterComponent> levelMeterComp;

    bool drawOverlayTrackColour {false},
         m_isResizing {false},
         m_isAboutToResizing {false};
};

//==============================================================================

class TrackOverlayComponent : public juce::Component
{
public:
    void setImagePos(int x)
    {
        m_xPos = x;
    }

    void setImage(juce::Image image)
    {
        m_dragImage = image;
    }

    void paint(juce::Graphics& g) override
    {
        if (m_dragImage.isValid())
        {
            g.setColour(juce::Colour(0x66666666));
            g.drawImageAt(m_dragImage, m_xPos, 0);
            g.setColour(juce::Colour(0xffffffff));
            g.drawRect(m_xPos, 0, m_dragImage.getWidth(), getHeight());
        }
    }
private:
    int m_xPos;
    juce::Image m_dragImage;
};


//==============================================================================
class TrackComponent : public juce::Component,
                       private te::ValueTreeAllEventListener,
                       private FlaggedAsyncUpdater,
                       private juce::ChangeListener,
                       public juce::DragAndDropTarget
{
public:
    TrackComponent (EditViewState&, te::Track::Ptr);
    ~TrackComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void resized() override;

    inline bool isInterestedInDragSource(const SourceDetails& /*dragSourceDetails*/) override { return true; }
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;

    te::Track::Ptr getTrack() const;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void valueTreeChanged() override {}

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;

    void handleAsyncUpdate() override;

    void buildClips();
    void buildRecordClips();

    EditViewState& editViewState;
    te::Track::Ptr track;
    te::Clipboard m_clipBoard;
    juce::OwnedArray<ClipComponent> clips;
    juce::Image m_dragImage;
    int m_posInClip{0};
    std::unique_ptr<RecordingClipComponent> recordingClip;

    TrackOverlayComponent m_trackOverlay;

    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         m_dragging = false;
};


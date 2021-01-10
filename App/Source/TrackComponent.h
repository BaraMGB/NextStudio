#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "PluginRackComponent.h"

namespace te = tracktion_engine;


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

    void setIsValid(bool isValid)
    {
        m_isValid = isValid;
    }

    bool isValid()
    {
        return m_isValid;
    }

    void paint(juce::Graphics& g) override
    {
        if (m_dragImage.isValid())
        {
            if (m_isValid)
            {
                g.setColour(juce::Colour(0x66666666));
                g.drawImageAt(m_dragImage, m_xPos, 0);
                g.setColour(juce::Colour(0xffffffff));
                g.drawRect(m_xPos, 0, m_dragImage.getWidth(), getHeight());
            }
            else
            {
                g.setColour (juce::Colour(0xff444444));
                g.fillRect (m_xPos, 0, m_dragImage.getWidth(), getHeight());
                g.setColour (juce::Colours::black);
                g.drawText ("not allowed", m_xPos, 0,m_dragImage.getWidth (), getHeight (),juce::Justification::centred,false);
            }
        }
    }
private:
    int m_xPos;
    juce::Image m_dragImage;
    bool m_isValid {false};
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

    inline bool isInterestedInDragSource(const SourceDetails&) override { return true; }
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails&) override;

    bool keyPressed(const juce::KeyPress &key) override;


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
    te::MidiClip::Ptr createNewMidiClip(double beatPos);

    bool isMidiTrack() { return m_track->state.getProperty(IDs::isMidiTrack); }

    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    te::Clipboard m_clipBoard;
    juce::OwnedArray<ClipComponent> m_clips;

    juce::Image m_dragImage;
    int m_posInClip{0};
    std::unique_ptr<RecordingClipComponent> recordingClip;

    TrackOverlayComponent m_trackOverlay;

    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         m_dragging = false;
};


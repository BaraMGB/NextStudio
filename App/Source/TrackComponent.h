#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "RecordingClipComponent.h"
#include "AudioClipComponent.h"
#include "PluginRackComponent.h"
#include "TrackOverlayComponent.h"

namespace te = tracktion_engine;


class TrackComponent : public juce::Component,
                       private te::ValueTreeAllEventListener,
                       private FlaggedAsyncUpdater,
                       private juce::ChangeListener,
                       public juce::DragAndDropTarget
{
public:
    TrackComponent (EditViewState&, te::Track::Ptr);
    ~TrackComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress &key) override;

    te::Track::Ptr getTrack() const;
    void inserWave(juce::File f, double time);
    juce::OwnedArray<ClipComponent>& getClipComponents();
    TrackOverlayComponent& getTrackOverlay();

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginListEntry")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

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
    std::unique_ptr<RecordingClipComponent> recordingClip;

    TrackOverlayComponent m_trackOverlay;

    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         isOver = false;
};

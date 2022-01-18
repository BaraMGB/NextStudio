#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "RecordingClipComponent.h"
#include "AudioClipComponent.h"
#include "PluginRackComponent.h"
#include "TrackOverlayComponent.h"
#include "AutomationLaneComponent.h"
#include "LowerRangeComponent.h"


class TrackComponent : public juce::Component,
                       private te::ValueTreeAllEventListener,
                       private FlaggedAsyncUpdater,
                       private juce::ChangeListener,
                       public juce::DragAndDropTarget
{
public:
    TrackComponent (EditViewState&, LowerRangeComponent& lr, te::Track::Ptr);
    ~TrackComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void resized() override;

    [[nodiscard]] te::Track::Ptr getTrack() const;
    void insertWave(const juce::File& f, double time);
    juce::OwnedArray<ClipComponent>& getClipComponents();
    TrackOverlayComponent& getTrackOverlay();

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
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
    void buildAutomationLanes();
    void buildRecordClips();
    te::MidiClip::Ptr createNewMidiClip(double beatPos);
    bool isMidiTrack() {return m_track->state.getProperty (IDs::isMidiTrack, false);}
    bool isSelected();

    double xToBeats(int x)
    {
        return m_editViewState.xToBeats (
                    x
                    , getWidth ()
                    , m_editViewState.m_viewX1
                    , m_editViewState.m_viewX2));
    }

    EditViewState& m_editViewState;
    LowerRangeComponent& m_lowerRange;
    te::Track::Ptr m_track;
    [[maybe_unused]] te::Clipboard m_clipBoard;
    juce::OwnedArray<ClipComponent> m_clipComponents;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;

    [[maybe_unused]] juce::Image m_dragImage;
    std::unique_ptr<RecordingClipComponent> recordingClip;

    TrackOverlayComponent m_trackOverlay;
    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         isOver = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackComponent)
};

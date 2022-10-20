#pragma once


#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "RecordingClipComponent.h"
#include "AudioClipComponent.h"
#include "PluginRackComponent.h"
#include "AutomationLaneComponent.h"
#include "LowerRangeComponent.h"

class OverlayClip
{
public:
    OverlayClip(juce::Rectangle<int> clipBounds, bool isValid, bool isResizing) :
        m_clipBounds(clipBounds),
        m_isValid(isValid),
        m_isResizing(isResizing)
    {
    }

    juce::Rectangle<int> getClipBounds(){ return m_clipBounds; }
    bool isValid() {return m_isValid;}
    bool isResizing() {return m_isResizing;}
private:
    juce::Rectangle<int> m_clipBounds;
    bool                 m_isValid;
    bool                 m_isResizing;

};
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
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

    [[nodiscard]] te::Track::Ptr getTrack() const;
    void insertWave(const juce::File& f, double time);
    juce::OwnedArray<ClipComponent>& getClipComponents();

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void addDraggedClip(bool isValid, int startX, int width, bool isResizing)
    {
       m_overlayClips.add(std::make_unique<OverlayClip>(juce::Rectangle<int> (startX, 0, width, getHeight()), isValid, isResizing));
    }

    void clearDraggedClips()
    {
        m_overlayClips.clear();
        repaint();
    }

    const juce::OwnedArray<AutomationLaneComponent> &getAutomationLanes() const;

    void setSelectedTimeRange(tracktion::TimeRange timerange, bool snap=true);
    tracktion::TimeRange getSelectedTimeRange();
    void clearSelectedTimeRange();
                
private:
    void drawDraggingOverlays(juce::Graphics& g);
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

    int getClipHeight();
	double getSnapedTime(double t, bool roundDown);

    double xToBeats(int x)
    {
        return m_editViewState.xToBeats (
                    x
                    , getWidth ()
                    , m_editViewState.m_viewX1
                    , m_editViewState.m_viewX2);
    }

    int timeToX (double time);

    EditViewState& m_editViewState;
    LowerRangeComponent& m_lowerRange;
    te::Track::Ptr m_track;
    [[maybe_unused]] te::Clipboard m_clipBoard;
    juce::OwnedArray<ClipComponent> m_clipComponents;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;

    [[maybe_unused]] juce::Image m_dragImage;
    std::unique_ptr<RecordingClipComponent> recordingClip;

    juce::OwnedArray<OverlayClip> m_overlayClips;
    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         isOver = false;
    bool isFolderTrack();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackComponent)
};

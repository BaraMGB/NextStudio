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

class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(te::AutomationCurve& curve, EditViewState& evs);
    void paint (juce::Graphics& g) override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    te::AutomationCurve &getCurve() const;
private:

    double getTime (int x);
    int getXPos (double time);
    double getValue(int y);
    int getYPos (double value);
    double xToYRatio();

    juce::Point<int> getPoint(const te::AutomationCurve::AutomationPoint& ap);
    int getLaneHeight();
    bool isBeyondLastPoint(double time, float value);
    int getPointWidth();

    double getNewTime(const juce::MouseEvent &e);
    int getIndexOfHoveredPoint(const juce::MouseEvent &e);

    te::AutomationCurve&        m_curve;
    int                         m_hoveredPoint;
    int                         m_hoveredCurve = -1;
    double                      m_hoveredTime;
    juce::Rectangle<int>        m_hoveredRect;
    double                      m_curveAtMousedown;
    double                      m_timeAtMousedown;
    EditViewState&              m_editViewState;
    bool                        m_isVertical {false};
};

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
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void resized() override;

    te::Track::Ptr getTrack() const;
    void inserWave(juce::File f, double time);
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
    bool isSelected()
    {
        return m_editViewState.m_selectionManager.getItemsOfType<te::Track>().contains (m_track);
    }
    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    te::Clipboard m_clipBoard;
    juce::OwnedArray<ClipComponent> m_clips;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;

    juce::Image m_dragImage;
    std::unique_ptr<RecordingClipComponent> recordingClip;

    TrackOverlayComponent m_trackOverlay;

    bool updateClips = false,
         updatePositions = false,
         updateRecordClips = false,
         isOver = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackComponent)
};

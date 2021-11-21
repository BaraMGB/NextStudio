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
    void mouseUp (const juce::MouseEvent& e) override;

    te::AutomationCurve &getCurve() const;

private:
    double getTime (int x)
    {
        return m_editViewState.xToTime(x, getWidth());
    }

    int getXPos (double time)
    {
        return m_editViewState.timeToX(time, getWidth());
    }

    double getValue(int y)
    {
        return 1.0 - static_cast<double>(y + (c_pointThickness/2+1) +1)
                        / static_cast<double>(getLaneHeight());
    }

    int getYPos (double value)
    {
        return (getLaneHeight() - value * getLaneHeight()) + (c_pointThickness/2) +1;
    }

    juce::Point<int> getPoint(const te::AutomationCurve::AutomationPoint& ap)
    {
        return {getXPos (ap.time), getYPos (ap.value)};
    }

    double xToYRatio()
    {
        //1 screen unit in value / 1 screen unit in time
        double screenUnitInValue = 1.0/getLaneHeight();
        double screenUnitInTime =
                (m_editViewState.beatToTime(m_editViewState.m_viewX2)
               - m_editViewState.beatToTime(m_editViewState.m_viewX1))
                                 / getWidth();
        return screenUnitInValue / screenUnitInTime;
    }

    int getLaneHeight()
    {
        return getHeight() - c_pointThickness - 3;
    }

    bool isBeyondLastPoint(double time, float value)
    {
        auto nearestPoint = m_curve.getNearestPoint (time, value , xToYRatio ());
        if (nearestPoint > m_curve.getNumPoints () - 1)
        {
            return true;
        }
        return false;
    }

    te::AutomationCurve&        m_curve;
    int                         m_hoveredPoint;
    int                         m_hoveredCurve = -1;
    double                      m_hoveredTime;
    juce::Rectangle<int>        m_hoveredRect;
    double                      m_curveAtMousedown;
    double                      m_timeAtMousedown;
    EditViewState&              m_editViewState;
    bool                        m_isVertical {false};
    //------------- const
    const int                   c_pointThickness = 8;
    const int                   c_halvePoint = c_pointThickness/2;
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

    bool keyPressed(const juce::KeyPress &key) override;

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

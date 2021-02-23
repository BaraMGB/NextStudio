#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "TimeLineComponent.h"
#include "TrackComponent.h"
#include "TrackHeadComponent.h"
#include "PluginRackComponent.h"
#include "PlayHeadComponent.h"
#include "LowerRangeComponent.h"

namespace te = tracktion_engine;

class ToolBarComponent : public juce::Component
{
public:
    ToolBarComponent ();

    void paint (juce::Graphics& g) override;

private:

};

//==============================================================================

class EditComponent : public  juce::Component
                    , private te::ValueTreeAllEventListener
                    , private FlaggedAsyncUpdater
                    , private juce::ChangeListener
                    , private juce::ScrollBar::Listener
                    , public juce::DragAndDropTarget
{
public:
    EditComponent (te::Edit&, te::SelectionManager&);
    ~EditComponent();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g);
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &event
                        , const juce::MouseWheelDetails &wheel) override;
    void scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved
                        , double newRangeStart) override;
    bool keyPressed(const juce::KeyPress &key) override;

    inline bool isInterestedInDragSource(const SourceDetails&) override { return true; }
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails&) override;

    LowerRangeComponent& lowerRange();
    juce::OwnedArray<TrackComponent>& getTrackComps();
    TrackComponent * getTrackComp(int y);
    TrackComponent * getTrackCompForTrack(te::Track::Ptr track);
    ClipComponent *getClipComponentForClip(te::Clip::Ptr clip);

    te::AudioTrack::Ptr addAudioTrack(bool isMidi, juce::Colour);

    void turnoffAllTrackOverlays();

private:

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;
    void handleAsyncUpdate() override;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void buildTracks();
    
    juce::OwnedArray<TrackComponent> m_trackComps;
    juce::OwnedArray<TrackHeaderComponent> m_headers;
    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;

    te::Edit& m_edit;
    EditViewState m_editViewState;

    TimeLineComponent m_timeLine {m_editViewState,
                  m_editViewState.m_viewX1
                , m_editViewState.m_viewX2 };
    juce::ScrollBar m_scrollbar_v, m_scrollbar_h;
    ToolBarComponent m_toolBar;
    PlayheadComponent m_playhead { m_edit
                                 , m_editViewState
                                 , m_editViewState.m_viewX1
                                 , m_editViewState.m_viewX2};
    LowerRangeComponent m_lowerRange { m_editViewState };

    juce::Rectangle<float> m_songeditorRect;
    bool m_updateTracks = false, m_updateZoom = false;
};

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
#include "LassoSelectionTool.h"
#include "SongEditorView.h"
#include "TrackListView.h"

//------------------------------------------------------------------------------

class FooterBarComponent : public juce::Component
{
public:
    explicit FooterBarComponent ()
    = default;
    void paint (juce::Graphics &g) override
    {
        g.setColour (juce::Colour (0xff181818));
        g.fillRect (
                    0
                  , 0
                  , getWidth ()
                  , getHeight ());
        g.setColour (juce::Colour (0xffffffff));
        g.drawText (m_snapTypeDesc
                  , getWidth () - 100
                  , 0
                  , 90
                  , getHeight ()
                  , juce::Justification::centredRight);
        g.setColour (juce::Colour (0xff555555));
    }
    juce::String m_snapTypeDesc;
};

//------------------------------------------------------------------------------

class EditComponent : public  juce::Component
                    , private te::ValueTreeAllEventListener
                    , private FlaggedAsyncUpdater
                    , private juce::ScrollBar::Listener
{
public:
    EditComponent (te::Edit&
                 , te::SelectionManager&
                 , juce::Array<juce::Colour> tc);
    ~EditComponent() override;

    void paint (juce::Graphics &g) override;
    void paintOverChildren (juce::Graphics &g) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseWheelMove (const juce::MouseEvent &event
                        , const juce::MouseWheelDetails &wheel) override;
    void scrollBarMoved (juce::ScrollBar *scrollBarThatHasMoved
                        , double newRangeStart) override;


    LowerRangeComponent& lowerRange ();

    TrackComponent * getTrackComponent (int y);

    te::AudioTrack::Ptr addAudioTrack (bool isMidi, juce::Colour);

    EditViewState& getEditViewState () { return m_editViewState; }

    void loopAroundSelection();

private:

    void valueTreeChanged () override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;
    void handleAsyncUpdate () override;

    void buildTracks ();

    void refreshSnapTypeDesc();

    tracktion_engine::EditTimeRange getSelectedClipRange();

    void updateHorizontalScrollBar();

    juce::Rectangle<int> getEditorHeaderRect();
    juce::Rectangle<int> getTimeLineRect();
    juce::Rectangle<int> getTrackListToolsRect();
    juce::Rectangle<int> getTrackListRect();
    juce::Rectangle<int> getSongEditorRect();
    juce::Rectangle<int> getFooterRect();
    juce::Rectangle<int> getPlayHeadRect();
    int                  getSongHeight();

    te::Edit&                               m_edit;
    EditViewState                           m_editViewState;

    SongEditorView                          m_songEditor;
    TrackListView                           m_trackListView;
    TimeLineComponent                       m_timeLine {
                                                m_editViewState
                                              , m_editViewState.m_viewX1
                                              , m_editViewState.m_viewX2
                                              };
    FooterBarComponent                      m_footerbar;
    juce::ScrollBar                         m_scrollbar_v
                                          , m_scrollbar_h;
    PlayheadComponent                       m_playhead {
                                                m_edit
                                              , m_editViewState
                                              , m_editViewState.m_viewX1
                                              , m_editViewState.m_viewX2 };
    LowerRangeComponent                     m_lowerRange { m_editViewState };

    juce::Array<juce::Colour>               m_trackColours;

    bool m_updateTracks = false, m_updateZoom = false, m_updateSongEditor = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditComponent)
};

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


class LassoSelectionComponent : public juce::Component
{
public:
    LassoSelectionComponent(EditViewState& evs)
        : m_editViewState(evs) {}
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

    void updateSelection(bool add);
private:
    struct LassoRect
    {
        LassoRect (){}
        LassoRect (te::EditTimeRange timeRange, double top, double bottom)
            : timeRange (timeRange)
            , verticalRange (top, bottom)
            , startTime (timeRange.getStart ())
            , endTime (timeRange.getEnd ())
            , top (top)
            , bottom (bottom){}
        juce::Rectangle<int> getRect (EditViewState& evs, int width)
        {
            int x = evs.timeToX (startTime, width);
            int y = top;
            int w = evs.timeToX (endTime, width) - x;
            int h = bottom - top;

            return  juce::Rectangle<int> (x, y, w, h);
        }
        te::EditTimeRange timeRange { 0,0 };
        juce::Range<double> verticalRange { 0,0 };
        double startTime { 0 };
        double endTime { 0 };
        double top { 0 };
        double bottom { 0 };
    };
    bool                           m_isLassoSelecting = false;
    EditViewState&                 m_editViewState;
    double                         m_clickedTime;
    double                         m_cachedY, m_cachedX;
    LassoRect                      m_lassoRect;


};


//------------------------------------------------------------------------------

class FooterBarComponent : public juce::Component
{
public:
    FooterBarComponent (EditViewState& evs)
        :m_editViewState (evs)
    {
    }
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
        g.drawLine (0,0,getWidth (), 1);
    }
    juce::String m_snapTypeDesc;
private:
    EditViewState& m_editViewState;
};

//------------------------------------------------------------------------------

class EditComponent : public  juce::Component
                    , private te::ValueTreeAllEventListener
                    , private FlaggedAsyncUpdater
                    , private juce::ScrollBar::Listener
                    , public juce::DragAndDropTarget
{
public:
    EditComponent (te::Edit&
                 , te::SelectionManager&
                 , juce::Array<juce::Colour> tc);
    ~EditComponent();

    void paint (juce::Graphics &g) override;
    void paintOverChildren (juce::Graphics &g);
    void resized () override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;
    void mouseWheelMove (const juce::MouseEvent &event
                        , const juce::MouseWheelDetails &wheel) override;
    void scrollBarMoved (juce::ScrollBar *scrollBarThatHasMoved
                        , double newRangeStart) override;
    bool keyPressed (const juce::KeyPress &key) override;

    inline bool isInterestedInDragSource (
            const SourceDetails&) override { return true; }
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails&) override;

    LowerRangeComponent& lowerRange ();
    juce::OwnedArray<TrackComponent>& getTrackComps ();
    TrackComponent * getTrackComp (int y);
    TrackComponent * getTrackCompForTrack (te::Track::Ptr track);
    ClipComponent *getClipComponentForClip (te::Clip::Ptr clip);

    te::AudioTrack::Ptr addAudioTrack (bool isMidi, juce::Colour);

    void turnoffAllTrackOverlays ();
    EditViewState& getEditViewState () { return m_editViewState; }
    LassoSelectionComponent* getLasso ();

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
    void refreshSnaptypeDesc ();
    
    juce::OwnedArray<TrackComponent>        m_trackComps;
    juce::OwnedArray<TrackHeaderComponent>  m_headers;
    juce::OwnedArray<PluginRackComponent>   m_pluginRackComps;

    te::Edit&                               m_edit;
    EditViewState                           m_editViewState;

    TimeLineComponent                       m_timeLine {
                                                m_editViewState
                                              , m_editViewState.m_viewX1
                                              , m_editViewState.m_viewX2
                                              , m_editViewState
                                                        .m_trackHeaderWidth };
    FooterBarComponent                      m_footerbar { m_editViewState };
    juce::ScrollBar                         m_scrollbar_v
                                          , m_scrollbar_h;
    PlayheadComponent                       m_playhead {
                                                m_edit
                                              , m_editViewState
                                              , m_editViewState.m_viewX1
                                              , m_editViewState.m_viewX2 };
    LowerRangeComponent                     m_lowerRange { m_editViewState };
    juce::Rectangle<float>                  m_songeditorRect;
    LassoSelectionComponent                          m_lassoComponent;
    juce::Array<juce::Colour>               m_trackColours;

    bool m_updateTracks = false, m_updateZoom = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditComponent)
};

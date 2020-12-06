
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "TimeLineComponent.h"
#include "TrackComponent.h"
#include "PluginRackComponent.h"


namespace te = tracktion_engine;

class PlayheadComponent : public juce::Component,
                          private juce::Timer
{
public:
    PlayheadComponent (te::Edit&, EditViewState&);
    
    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    
    te::Edit& edit;
    EditViewState& editViewState;
    
    int xPosition = 0;
    bool firstTimer = true;
};
//==============================================================================

class ToolBarComponent : public juce::Component

{
public:
    ToolBarComponent ();

    void paint (juce::Graphics& g) override;

private:


};

//==============================================================================
class EditComponent : public  juce::Component,
                      private te::ValueTreeAllEventListener,
                      private FlaggedAsyncUpdater,
                      private juce::ChangeListener,
                      private juce::ScrollBar::Listener

{
public:
    EditComponent (te::Edit&, te::SelectionManager&);
    ~EditComponent();
    
    EditViewState& getEditViewState()   { return editViewState; }
        
    juce::OwnedArray<TrackComponent> & getTrackComps();

private:
    void valueTreeChanged() override {}
    
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;
    
    void mouseDown(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;
    void paint(juce::Graphics &g) override;
    void handleAsyncUpdate() override;
    void resized() override;
    
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;

    
    void buildTracks();
    
    te::Edit& edit;
    
    EditViewState editViewState;
    TimeLineComponent timeLine {editViewState};
    juce::ScrollBar m_scrollbar;
    ToolBarComponent toolBar;
    PlayheadComponent playhead {edit, editViewState};
    LowerRangeComponent pluginRack {editViewState};
    juce::OwnedArray<TrackComponent> tracks;
    juce::OwnedArray<TrackHeaderComponent> headers;
    juce::OwnedArray<PluginRackComponent> footers;

    
    bool updateTracks = false, updateZoom = false;
};

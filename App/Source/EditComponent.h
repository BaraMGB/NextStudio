
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "TimeLineComponent.h"
#include "TrackComponent.h"
#include "PluginRackComponent.h"


namespace te = tracktion_engine;

class PlayheadComponent : public Component,
                          private Timer
{
public:
    PlayheadComponent (te::Edit&, EditViewState&);
    
    void paint (Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDrag (const MouseEvent&) override;
    void mouseDown (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;

private:
    void timerCallback() override;
    
    te::Edit& edit;
    EditViewState& editViewState;
    
    int xPosition = 0;
    bool firstTimer = true;
};
//==============================================================================

class TollBarComponent : public Component

{
public:
    TollBarComponent ();

    void paint (Graphics& g) override;

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
        
    OwnedArray<TrackComponent> & getTrackComps();

private:
    void valueTreeChanged() override {}
    
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;
    
    void mouseDown(const MouseEvent &) override;
    void paint(Graphics &g) override;
    void handleAsyncUpdate() override;
    void resized() override;
    
    void changeListenerCallback (ChangeBroadcaster*) override { repaint(); }
    void scrollBarMoved(ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;

    
    void buildTracks();
    
    te::Edit& edit;
    
    EditViewState editViewState;
    TimeLineComponent timeLine {editViewState};
    juce::ScrollBar m_scrollbar;
    TollBarComponent toolBar;
    PlayheadComponent playhead {edit, editViewState};
    LowerRangeComponent pluginRack {editViewState};
    OwnedArray<TrackComponent> tracks;
    OwnedArray<TrackHeaderComponent> headers;
    OwnedArray<PluginRackComponent> footers;

    
    bool updateTracks = false, updateZoom = false;
};

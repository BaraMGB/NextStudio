
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "TimeLineComponent.h"
#include "TrackComponent.h"

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

class PluginRackComponent : public Component
                          , public juce::ChangeListener
{
public:
    PluginRackComponent (EditViewState& evs) : editViewState(evs)
    {
        editViewState.selectionManager.addChangeListener(this);

    }
     ~PluginRackComponent()
     {
         editViewState.selectionManager.removeChangeListener(this);
     }
    
    void changeListenerCallback (ChangeBroadcaster*) override 
    {
        auto lastClickedTrack = editViewState.selectionManager.getItemsOfType<tracktion_engine::Track>().getLast();
        if (lastClickedTrack &&  !(lastClickedTrack->isArrangerTrack() 
                            || lastClickedTrack->isTempoTrack()
                            || lastClickedTrack->isMarkerTrack()
                            || lastClickedTrack->isChordTrack())) 
        {
            m_pointedTrack = lastClickedTrack;
            m_footers.clear();
            auto tfc = new TrackFooterComponent(editViewState, m_pointedTrack);
            m_footers.add(tfc);
            addAndMakeVisible(tfc);
            tfc->setAlwaysOnTop(true);
            resized();
        }
        else
        {
             m_footers.clear();
             resized();
        }
    }

    void paint (Graphics& g) override
    {

        auto rect = getLocalBounds();
        g.setColour(Colour(0xff181818));
        g.fillRect(rect);
        g.setColour(juce::Colours::yellow);
        g.drawRect(rect);

    }
    void resized () override
    {
        if (m_footers.getFirst())
        {
            m_footers.getFirst()->setBounds(getLocalBounds());
        }
    }

private:
      
    EditViewState& editViewState;
    OwnedArray<TrackFooterComponent> m_footers;

    tracktion_engine::Track * m_pointedTrack;
    
};

//==============================================================================
class EditComponent : public Component,
                      private te::ValueTreeAllEventListener,
                      private FlaggedAsyncUpdater,
                      private ChangeListener

{
public:
    EditComponent (te::Edit&, te::SelectionManager&);
    ~EditComponent();
    
    EditViewState& getEditViewState()   { return editViewState; }
        
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

    
    void buildTracks();
    
    te::Edit& edit;
    
    EditViewState editViewState;
    te::Clipboard clipBoard;
    TimeLineComponent timeLine {editViewState};
    PlayheadComponent playhead {edit, editViewState};
    PluginRackComponent pluginRack {editViewState};
    OwnedArray<TrackComponent> tracks;
    OwnedArray<TrackHeaderComponent> headers;
    OwnedArray<TrackFooterComponent> footers;
    
    bool updateTracks = false, updateZoom = false;
};

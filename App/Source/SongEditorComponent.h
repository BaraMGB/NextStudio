/*
  ==============================================================================

    SongEditorComponent.h
    Created: 7 Jan 2020 8:31:49pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"
#include "ArrangerComponent.h"
#include "TimeLineComponent.h"
#include "SongEditorState.h"


class ScrollArea : public Viewport
                 , public ChangeBroadcaster
{
public:
    ScrollArea() {}
    ~ScrollArea() {}

    void visibleAreaChanged(const Rectangle<int>& newVisableArea) override
    {
        sendChangeMessage();
    }
};

//==============================================================================
/*
*/
class SongEditorComponent    : public Component
                             , public ChangeListener
                             , public ValueTree::Listener
{
public:
    SongEditorComponent(tracktion_engine::Edit&, tracktion_engine::SelectionManager&);
    ~SongEditorComponent();

    void paint (Graphics&) override;
    void resized() override;
    void addTrack(File& f);
    void addTrack();
    void buildTracks();

    void mouseDown(const MouseEvent& event) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;
    void valueTreePropertyChanged(ValueTree& tree, const Identifier& id) override
    {
        if (id.toString() == "position")
        {
            m_arranger.updatePositionLine();
        }
        else if(id.toString() == "start")
        {
            m_arranger.ClipsMoved();
        }
        else if (id.toString() == "bpm")
        {
            m_arranger.resized();
        }
        else
        {
        }
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& child) override 
    {
        buildTracks();
        m_arranger.resized();
        m_arranger.repaint();
    }
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override
    {
        buildTracks();
        m_arranger.resized();
        m_arranger.repaint();
    }
    void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override
    {
    }
    void valueTreeParentChanged(ValueTree&) override {}
private:
    tracktion_engine::AudioTrack* getOrInsertAudioTrackAt(int index);
    void removeAllClips(tracktion_engine::AudioTrack& track)
    {

        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked(i)->removeFromParentTrack();
    }

    void trackHeadsresized()
    {
        for (auto& trackComp : m_tracks)
        {
            trackComp->resized();
        }
    }

    OwnedArray<TrackHeaderComponent> m_tracks;
    ScrollArea m_arrangeViewport;
    tracktion_engine::Edit& m_edit;
    SongEditorViewState m_songEditorState;
    ArrangerComponent m_arranger;
    TimeLineComponent m_timeLineComp;
    TextButton m_toolBox;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorComponent)
};

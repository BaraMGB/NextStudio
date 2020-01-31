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
{
public:
    SongEditorComponent(tracktion_engine::Edit &edit);
    ~SongEditorComponent();

    void paint (Graphics&) override;
    void resized() override;
    void addTrack(File& f);

    void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    tracktion_engine::AudioTrack* getOrInsertAudioTrackAt(int index);
    void removeAllClips(tracktion_engine::AudioTrack& track)
    {
        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked(i)->removeFromParentTrack();
    }

    OwnedArray<TrackHeaderComponent> m_tracks;
    ScrollArea m_arrangeViewport;
    ArrangerComponent m_arranger;
    tracktion_engine::Edit& m_edit;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorComponent)
};

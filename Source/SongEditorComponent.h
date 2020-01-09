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
    SongEditorComponent();
    ~SongEditorComponent();

    void paint (Graphics&) override;
    void resized() override;
    void addTrack();

    void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    Array<TrackHeaderComponent*> m_tracks;
    ScrollArea m_arrangeViewport;
    ArrangerComponent * m_arranger;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorComponent)
};

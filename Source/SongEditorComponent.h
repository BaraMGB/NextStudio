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

//==============================================================================
/*
*/
class SongEditorComponent    : public Component
{
public:
    SongEditorComponent();
    ~SongEditorComponent();

    void paint (Graphics&) override;
    void resized() override;
    void addTrack();

private:
    Array<TrackHeaderComponent*> m_tracks;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorComponent)
};

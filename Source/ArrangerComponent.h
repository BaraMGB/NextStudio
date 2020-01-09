/*
  ==============================================================================

    ArrangerComponent.h
    Created: 7 Jan 2020 8:29:38pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"
#include "ClipComponent.h"



//==============================================================================
/*
*/
class ArrangerComponent    : public Component
                           , public ChangeListener
{
public:
    ArrangerComponent(Array<TrackHeaderComponent*> *tracks);
    ~ArrangerComponent();

    void paint (Graphics&) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        //position the Clips
        resized();
    }

private:

    Array<TrackHeaderComponent*> *m_trackComponents;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangerComponent)
};

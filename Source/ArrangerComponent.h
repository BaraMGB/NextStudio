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

class PositionLine : public Component
{
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::white);
    }
};

//==============================================================================
/*
*/
class ArrangerComponent    : public Component
                           , public ChangeBroadcaster
{
public:
    ArrangerComponent(OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit& edit, int& pixelPerBeat);
    ~ArrangerComponent();

    void paint (Graphics&) override;
    void resized() override;

    void mouseWheelMove(const MouseEvent& event,
        const MouseWheelDetails& wheel);
    const int getPixelPerBeat()
    {
        return m_pixelPerBeats;
    }

private:

    OwnedArray<TrackHeaderComponent>& m_trackComponents;
    tracktion_engine::Edit& m_edit;
    int& m_pixelPerBeats;
    PositionLine m_positionLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangerComponent)
};

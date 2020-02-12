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
{
public:
    ArrangerComponent(OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit& edit);
    ~ArrangerComponent();

    void paint (Graphics&) override;
    void resized() override;

    void mouseWheelMove(const MouseEvent& event,
        const MouseWheelDetails& wheel)
    {
        m_pixelPerBeats = m_pixelPerBeats + wheel.deltaY;
        resized();
    }

    void setPixelPerBeats(double pixelPerBeats)
    {
        m_pixelPerBeats = pixelPerBeats;
    }

    double getPixelPerBeats()
    {
        return m_pixelPerBeats;
    }
private:

    OwnedArray<TrackHeaderComponent>& m_trackComponents;
    tracktion_engine::Edit& m_edit;
    double m_pixelPerBeats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangerComponent)
};

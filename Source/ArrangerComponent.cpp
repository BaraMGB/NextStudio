/*
  ==============================================================================

    ArrangerComponent.cpp
    Created: 7 Jan 2020 8:29:38pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ArrangerComponent.h"

//==============================================================================
ArrangerComponent::ArrangerComponent(
    OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit & edit)
    : m_trackComponents(trackComps)
    , m_edit(edit)
    , m_pixelPerBeats(1)
{
    setSize(3000, 3000);
}

ArrangerComponent::~ArrangerComponent()
{
}

void ArrangerComponent::paint (Graphics& g)
{
    g.fillAll (Colour(0xff181818));
    auto area = getLocalBounds();
    g.setColour (Colours::black);

    for(auto& track : m_trackComponents)
    {
        auto height = track->getTrackheight();
        area.removeFromTop(height);
        g.drawLine(area.getX(), area.getY(), area.getWidth(), area.getY());
    }
}

void ArrangerComponent::resized()
{
    auto area = getLocalBounds();
    for (auto& trackComp : m_trackComponents)
    {
        for (auto& clipComp : *trackComp->getClipComponents())
        {
            const double length = m_edit.tempoSequence.timeToBeats(clipComp->getLength()) * m_pixelPerBeats;
            const double start  = m_edit.tempoSequence.timeToBeats(clipComp->getStart())  * m_pixelPerBeats;
           //std::cout << "Length: " << length << "  Start: " << start << std::endl;
            clipComp->setBounds(area.getX() + start,
                                area.getY(),
                                length,
                                trackComp->getTrackheight());
        }
        area.removeFromTop(trackComp->getTrackheight());
    }
}

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
ArrangerComponent::ArrangerComponent(OwnedArray<TrackHeaderComponent>& tracks)
    : m_trackComponents(tracks)
{
    
    setSize(3000, 3000);
}

ArrangerComponent::~ArrangerComponent()
{
}

void ArrangerComponent::paint (Graphics& g)
{
    g.fillAll (Colour(0xff181818));   // clear the background
    auto area = getLocalBounds();
    g.setColour (Colours::black);
    
    for(auto i = 0; i < m_trackComponents.size(); i++)
    {
        auto track = m_trackComponents.getUnchecked(i);
        auto height = track->getTrackheight();
        area.removeFromTop(height);
        g.drawLine(area.getX(), area.getY(), area.getWidth(), area.getY());
       
    }
}

void ArrangerComponent::resized()
{
    auto area = getLocalBounds();
    for (auto i = 0; i < m_trackComponents.size(); i++)
    {
        auto track = m_trackComponents.getUnchecked(i);
        for (auto j = 0; j < track->getClips()->size(); j++)
        {
            auto clip = track->getClips()->getUnchecked(j);
            clip->setBounds(area.getX() + clip->clipPosition() , area.getY(), clip->clipLength(), track->getTrackheight());
            clip->setClipColour(track->trackColour());
        }
        area.removeFromTop(track->getTrackheight());
    }
}

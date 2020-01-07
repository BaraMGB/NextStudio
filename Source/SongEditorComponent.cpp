/*
  ==============================================================================

    SongEditorComponent.cpp
    Created: 7 Jan 2020 8:31:49pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SongEditorComponent.h"

//==============================================================================
SongEditorComponent::SongEditorComponent()
{
   /* addAndMakeVisible(m_arrangerViewport);
    m_arrangerViewport.getVerticalScrollBar().addListener(this);
    m_arrangerViewport.addAndMakeVisible(m_arrangerComponent);
    m_arrangerViewport.setViewedComponent(&m_arrangerComponent, false);
    m_arrangerViewport.setScrollBarsShown(true, true, true, true);*/

    addTrack();
    addTrack();
    addTrack();
    addTrack();
    addTrack();
    addTrack();
    addTrack();
    addTrack();


   //addAndMakeVisible(m_toolBox);
}

SongEditorComponent::~SongEditorComponent()
{
}

void SongEditorComponent::paint (Graphics& g)
{
   /* g.setColour(Colour(0xffff0000));
    g.fillRect(0,0,50,50);*/
}

void SongEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto trackRack = area.removeFromLeft(310);

    for (auto i = 0; i < m_tracks.size(); i++)
    {
        auto track = m_tracks.getReference(i);
        auto trackRect = trackRack.removeFromTop(track->getTrackheight());
        track->setBounds(trackRect);
        
    }

    
}

void SongEditorComponent::addTrack()
{
    auto track = new TrackHeaderComponent();
    addAndMakeVisible(track);
    auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    track->setTrackColour(Colour(red, gre, blu));
    track->setTrackName("Track " + String(m_tracks.size() + 1));
    m_tracks.add(track);
}

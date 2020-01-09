/*
  ==============================================================================

    SongEditorComponent.cpp
    Created: 7 Jan 2020 8:31:49pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SongEditorComponent.h"
#include "ClipComponent.h"



//==============================================================================
SongEditorComponent::SongEditorComponent()
    : m_arranger(new ArrangerComponent(&m_tracks))
{
    addAndMakeVisible(m_arrangeViewport);
    m_arrangeViewport.addChangeListener(this);
    m_arrangeViewport.addAndMakeVisible(m_arranger);
    m_arrangeViewport.setViewedComponent(m_arranger, false);
    m_arrangeViewport.setScrollBarsShown(true, true, true, true);

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
}

void SongEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto trackRack = area.removeFromLeft(310);

    for (auto i = 0; i < m_tracks.size(); i++)
    {
        auto track = m_tracks.getReference(i);
        auto trackRect = trackRack.removeFromTop(track->getTrackheight());
        track->setBounds(trackRect.getX(),
                         trackRect.getY() - m_arrangeViewport.getVerticalScrollBar().getCurrentRangeStart(),
                         trackRack.getWidth(),
                         track->getTrackheight());
    }
    m_arrangeViewport.setBounds(area);
}

void SongEditorComponent::addTrack()
{
    auto track = new TrackHeaderComponent();
    addAndMakeVisible(track);
    track->addChangeListener(m_arranger);
    auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    track->setTrackColour(Colour(red, gre, blu));
    track->setTrackName("Track " + String(m_tracks.size() + 1));
    m_tracks.add(track);
}

void SongEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    //reposition the TrackRack
    resized();
}



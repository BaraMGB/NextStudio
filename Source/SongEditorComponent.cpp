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
SongEditorComponent::SongEditorComponent(tracktion_engine::Edit &edit)
    : m_arranger(new ArrangerComponent(&m_tracks))
    , m_edit(&edit)
{
    addAndMakeVisible(m_arrangeViewport);
    m_arrangeViewport.addChangeListener(this);
    m_arrangeViewport.addAndMakeVisible(m_arranger);
    m_arrangeViewport.setViewedComponent(m_arranger, false);
    m_arrangeViewport.setScrollBarsShown(true, true, true, true);


    
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

void SongEditorComponent::addTrack(File& f)
{
    auto trackComponent = new TrackHeaderComponent(*m_edit);
    addAndMakeVisible(trackComponent);
    trackComponent->addChangeListener(m_arranger);
    auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    trackComponent->setTrackColour(Colour(red, gre, blu));
    trackComponent->setTrackName("Track " + String(m_tracks.size() + 1));
    m_tracks.add(trackComponent);
    if (auto track = getOrInsertAudioTrackAt(m_tracks.size()))
    {
        removeAllClips(*track);
        // Add a new clip to this track
        tracktion_engine::AudioFile audioFile(f);

        if (audioFile.isValid())
            if (auto newClip = track->insertWaveClip(f.getFileNameWithoutExtension(), f,
                { { 0.0, audioFile.getLength() }, 0.0 }, false))
            {
                m_arranger->addAndMakeVisible(trackComponent->createClip(0, audioFile.getLength()));
                m_arranger->resized();
                m_arranger->repaint();
            }
    }
    resized();

}

void SongEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    //reposition the TrackRack
    resized();
}

tracktion_engine::AudioTrack* SongEditorComponent::getOrInsertAudioTrackAt(int index)
{
    m_edit->ensureNumberOfAudioTracks(index + 1);
    return tracktion_engine::getAudioTracks(*m_edit)[index];
}



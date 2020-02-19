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
SongEditorComponent::SongEditorComponent(tracktion_engine::Edit& edit)
    : m_edit(edit)
    , m_arranger(m_tracks, edit)
{
    addAndMakeVisible(m_arrangeViewport);
    m_arrangeViewport.addChangeListener(this);
    m_arranger.addChangeListener(this);
    m_arrangeViewport.addAndMakeVisible(m_arranger);
    m_arrangeViewport.setViewedComponent(&m_arranger, false);
    m_arrangeViewport.setScrollBarsShown(true, true, true, true);

    edit.state.addListener(this);
    addAndMakeVisible(m_timeLineComp);
    m_timeLineComp.setZoom(m_arranger.getPixelPerBeats());

    addAndMakeVisible(m_toolBox);
}

SongEditorComponent::~SongEditorComponent()
{
    m_tracks.clear();
}

void SongEditorComponent::paint (Graphics& g)
{
}

void SongEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto timeline = area.removeFromTop(50);
    auto trackRack = area.removeFromLeft(310);
    auto toolBox = timeline.removeFromLeft(trackRack.getWidth());
    m_toolBox.setBounds(toolBox);
    auto lenght = m_edit.tempoSequence.timeToBeats(m_edit.getLength()) * m_arranger.getPixelPerBeats();
    auto arrangerPos = m_arrangeViewport.getViewPositionX();
    m_timeLineComp.setBounds(timeline.getX() -  arrangerPos, timeline.getY(), jmax(timeline.getWidth(), timeline.getX()) + lenght , timeline.getHeight());
    for (auto& track : m_tracks)
    {
        auto trackRect = trackRack.removeFromTop(track->getTrackheight());
        track->setBounds(trackRect.getX(),
                         trackRect.getY() - m_arrangeViewport.getVerticalScrollBar().getCurrentRangeStart(),
                         trackRack.getWidth(),
                         track->getTrackheight());
    }
    
    m_arranger.setSize(area.getWidth() + lenght, area.getHeight());
    m_arrangeViewport.setBounds(area);
}

void SongEditorComponent::addTrack(File& f)
{
    auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    
    if (auto track = getOrInsertAudioTrackAt(m_tracks.size()))
    {
        track->setName("Track " + String(m_tracks.size() + 1));
        track->setColour(Colour(red, gre, blu));
        auto trackHeader = new TrackHeaderComponent(*track);
        addAndMakeVisible(trackHeader);
        m_tracks.add(trackHeader);

        removeAllClips(*track);
        // Add a new clip to this track
        tracktion_engine::AudioFile audioFile(f);

        if (audioFile.isValid())
            if (auto newClip = track->insertWaveClip(f.getFileNameWithoutExtension(), f,
                { { 0.0, 0.0 + audioFile.getLength() }, 0.0 }, false))
            {
                newClip->setColour(track->getColour());
                m_arranger.addAndMakeVisible(trackHeader->createClip(*newClip));
                m_arranger.resized();
                m_arranger.repaint();

                track->getVolumePlugin()->volume = 0.1f;
            }
    }
    
    resized();

}

void SongEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    std::cout << "sdfsdfsdfghdfhfdj";
    //reposition the TrackRack
    resized();
    m_timeLineComp.setZoom(m_arranger.getPixelPerBeats());
}

tracktion_engine::AudioTrack* SongEditorComponent::getOrInsertAudioTrackAt(int index)
{
    m_edit.ensureNumberOfAudioTracks(index + 1);
    return tracktion_engine::getAudioTracks(m_edit)[index];
}



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
SongEditorComponent::SongEditorComponent(tracktion_engine::Edit& edit, tracktion_engine::SelectionManager& selectionManager)
    : m_edit(edit)
    , m_songEditorState(edit, selectionManager)
    , m_arranger(m_tracks, edit, m_songEditorState)
    , m_timeLineComp(m_songEditorState, m_arrangeViewport)
{
    m_edit.state.addListener(this);
    selectionManager.addChangeListener(this);
    addAndMakeVisible(m_arrangeViewport);
    m_arrangeViewport.addChangeListener(this);
    m_arranger.addChangeListener(this);
    m_arrangeViewport.addAndMakeVisible(m_arranger);
    m_arrangeViewport.setViewedComponent(&m_arranger, false);
    m_arrangeViewport.setScrollBarsShown(true, true, true, true);

    addAndMakeVisible(m_timeLineComp);
    m_timeLineComp.addChangeListener(this); 


    edit.state.addListener(this);
    buildTracks();
    addAndMakeVisible(m_toolBox);
}

SongEditorComponent::~SongEditorComponent()
{
    m_tracks.clear();
}

void SongEditorComponent::paint (Graphics& g)
{
    auto area = getLocalBounds();
    auto timeline = area.removeFromTop(50);
    auto trackRack = area.removeFromLeft(310);
    g.setColour(Colour(0xff181818));
    g.fillRect(getLocalBounds());
    g.setColour(Colours::white);
    g.drawRect(trackRack);
    Logger::outputDebugString("SE: painted " );
}

void SongEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto timeline = area.removeFromTop(50);
    auto trackRack = area.removeFromLeft(310);
    auto toolBox = timeline.removeFromLeft(trackRack.getWidth());
    auto lenght = jmax(
        500 * m_songEditorState.m_pixelPerBeat
        , m_edit.tempoSequence.timeToBeats(m_edit.getLength()) * m_songEditorState.m_pixelPerBeat
    );
    auto arrangerPos = m_arrangeViewport.getViewPositionX();

    //Tracks
    
    Logger::outputDebugString( m_edit.state.toXmlString());
    auto rackHeight = 0;
    for (auto& trackComp : m_tracks)
    {
        
        auto trackRect = trackRack.removeFromTop(trackComp->getTrackheight());
        trackComp->setBounds(
            trackRect.getX()
            , trackRect.getY() - m_arrangeViewport.getVerticalScrollBar().getCurrentRangeStart()
            , trackRack.getWidth()
            , trackComp->getTrackheight()
        );
        rackHeight += trackRect.getHeight();
    }

    m_toolBox.setBounds(toolBox);
    m_toolBox.setAlwaysOnTop(true);
    //Timeline
    m_timeLineComp.setBounds(timeline.getX() -  arrangerPos
        , timeline.getY()
        , jmax(timeline.getWidth()
            , timeline.getX()) + lenght
        , timeline.getHeight()
    );
    //inform the timeline about its screen position
    m_timeLineComp.setScreen(
        m_toolBox.getScreenPosition().getX() + m_toolBox.getWidth()
        , getScreenX() + getWidth()
    );
    //Logger::outputDebugString(String("rackHeigth : ") + String(rackHeight) + String("getHeight() : " + String(area.getHeight())));

    m_arranger.setSize(lenght, rackHeight + 300);
    m_arrangeViewport.setBounds(area);

    Logger::outputDebugString("SE: resized" );
}

void SongEditorComponent::addTrack(File& f)
{
    auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));
    if (auto track = getOrInsertAudioTrackAt(tracktion_engine::getAudioTracks(m_edit).size()))
    {

        track->setName("Track " + String(tracktion_engine::getAudioTracks(m_edit).size()));
        track->setColour(Colour(red, gre, blu));
       
        removeAllClips(*track);
        // Add a new clip to this track
        tracktion_engine::AudioFile audioFile(m_edit.engine, f);
        if (audioFile.isValid())
            if (auto newClip = track->insertWaveClip(f.getFileNameWithoutExtension(), f,
                { { 0.0, 0.0 + audioFile.getLength() }, 0.0 }, false))
            {
                newClip->setColour(track->getColour());
            }
        track->getVolumePlugin()->volume = 0.1f;
    }
    Logger::outputDebugString("SE: Track added with File: " + f.getFileNameWithoutExtension());
    //buildTracks();
    //m_arranger.resized();
    //m_arranger.repaint();
    //resized();
}

void SongEditorComponent::addTrack()
{
    if (auto track = getOrInsertAudioTrackAt(tracktion_engine::getAudioTracks(m_edit).size()))
    {
        auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
        auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
        auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));

        track->setName("Track " + String(m_tracks.size()));
        track->setColour(Colour(red, gre, blu));

        removeAllClips(*track);
        track->getVolumePlugin()->volume = 0.4f;
        Logger::outputDebugString("SE: Track added");
        //buildTracks();
        //resized();
    //    m_arranger.resized();
    //    m_arranger.repaint();
    }
}

void SongEditorComponent::buildTracks()
{
    m_tracks.clear();
    for (auto track : tracktion_engine::getAllTracks(m_edit))
    {
        TrackHeaderComponent* thc = nullptr;

        if (track->isTempoTrack())
        {
            if (m_songEditorState.m_showGlobalTrack)
                thc = new TrackHeaderComponent(m_songEditorState, track);
        }
        else if (track->isMarkerTrack())
        {
            if (m_songEditorState.m_showMarkerTrack)
                thc = new TrackHeaderComponent(m_songEditorState, track);
        }
        else if (track->isChordTrack())
        {
            if (m_songEditorState.m_showChordTrack)
                thc = new TrackHeaderComponent(m_songEditorState, track);
        }
        else if(track->isAudioTrack())
        {
            thc = new TrackHeaderComponent(m_songEditorState, track);
        }

        if (thc != nullptr)
        {
            Logger::outputDebugString("buildTracks : TrackName: " + track->getName());
            m_tracks.add(thc);
             addAndMakeVisible(thc);
        }
    }
    //Logger::outputDebugString( m_edit.state.toXmlString());
    resized();
    Logger::outputDebugString("SE: Tracks build");
}

void SongEditorComponent::mouseDown(const MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        PopupMenu m;
        m.addItem(1, "add Track");
        m.addItem(2, "item 2");

        const int result = m.show();

        if (result == 0)
        {
            // user dismissed the menu without picking anything
        }
        else if (result == 1)
        {
            addTrack();
            // user picked item 1
        }
        else if (result == 2)
        {
            // user picked item 2
        }
    }
    else if (event.mods.isLeftButtonDown())
    {
        m_songEditorState.m_selectionManager.deselectAll();
    }
}

void SongEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    Logger::outputDebugString("SE:ChangeListener" );
    if (source == &m_timeLineComp)
    {
        m_timeLineComp.repaint();
        resized();
    }
    else if (source == &m_arrangeViewport)
    {
        resized();
        m_arranger.setYpos(m_arrangeViewport.getViewPositionY());
    }
    else if (source == &m_songEditorState.m_selectionManager)
    {
        for (auto& th : m_tracks)
        {
            th->repaint();
        }
    }
}

tracktion_engine::AudioTrack* SongEditorComponent::getOrInsertAudioTrackAt(int index)
{
    m_edit.ensureNumberOfAudioTracks(index + 1);
    return tracktion_engine::getAudioTracks(m_edit)[index];
}



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
    OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit & edit, SongEditorViewState& state)
    : m_trackComponents(trackComps)
    , m_edit(edit)
    , m_state(state)
{
    addAndMakeVisible(m_positionLine);
    m_positionLine.setAlwaysOnTop(true);
}

ArrangerComponent::~ArrangerComponent()
{
}

void ArrangerComponent::paint (Graphics& g)
{
    g.fillAll (Colour(0xff181818));
    auto area = getLocalBounds();
    g.setColour (Colour(0xff343434));



    //horizontal line for tracks
    for(auto& track : m_trackComponents)
    {
        auto height = track->getTrackheight();
        area.removeFromTop(height);
        g.drawLine(area.getX(), area.getY(), area.getWidth(), area.getY());
    }

    //vertical lines
    auto beatLine = 0, barline = 0;
    while (beatLine < getWidth())
    {
        barline++;
        g.setColour(Colour(0xff242424));
        if (barline == 4)
        {
            g.setColour(Colour(0xff343434));
            barline = 0;
        }
        beatLine = beatLine + m_state.m_pixelPerBeat;
        if (m_state.m_pixelPerBeat > 8.0 || barline == 0)
        {
            g.drawLine(beatLine, 0, beatLine, getHeight());
        }
    }
    Logger::outputDebugString("AR:painted " );
}

void ArrangerComponent::resized()
{
    auto area = getLocalBounds();

    for (auto & trackComp : m_trackComponents)
    {
        trackComp->m_clipComponents.clear();
        for (auto i = 0; i < trackComp->getEngineTrack()->getNumTrackItems(); i++)
        {
            auto clip = dynamic_cast<tracktion_engine::WaveAudioClip*>(trackComp->getEngineTrack()->getTrackItem(i));
            if (clip)
            {
                const double length = clip->getLengthInBeats() * m_state.m_pixelPerBeat;
                const double start = clip->getStartBeat() * m_state.m_pixelPerBeat;
                auto clipRect = Rectangle<int>(area.getX() + start,
                    area.getY(),
                    length,
                    trackComp->getEngineTrack()->defaultTrackHeight);
                auto clipComp = std::make_unique<ClipComponent>(*clip, m_state);
                addAndMakeVisible(clipComp.get());
                clipComp.get()->setBounds(clipRect);
                trackComp->m_clipComponents.add(std::move(clipComp));
            }
    }
    area.removeFromTop(trackComp->getEngineTrack()->defaultTrackHeight);
    }
    updatePositionLine();

    Logger::outputDebugString("AR: resized" );
}

void ArrangerComponent::updatePositionLine()
{
    double pos = m_edit.tempoSequence.timeToBeats(m_edit.getTransport().getCurrentPosition()) * static_cast<int>(m_state.m_pixelPerBeat);
    m_positionLine.setBounds(pos, 0, 1, getLocalBounds().getHeight());
}

void ArrangerComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    auto dropPos = dragSourceDetails.localPosition;
    auto sourcePosY = dragSourceDetails.sourceComponent.get()->getY();
    TrackHeaderComponent* targetTrackHeaderComp = nullptr;
    for (auto& trackComp : m_trackComponents)
    {
        if (   (dropPos.getY() - m_yPos)  > (trackComp->getY() - 50) 
            && (dropPos.getY() - m_yPos) < ((trackComp->getY() - 50) + trackComp->getHeight()))
        {
            targetTrackHeaderComp = trackComp;
        }
    }
    auto timePos = m_edit.tempoSequence.beatsToTime(dropPos.getX() / m_state.m_pixelPerBeat);
    if (dragSourceDetails.description == "Clip")
    {
        auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
        if (clipComp && targetTrackHeaderComp)
        {
            for (auto& trackComp : m_trackComponents)
            {
                if (trackComp->m_clipComponents.contains(clipComp))
                {
                    trackComp->m_clipComponents.removeObject(clipComp, false);
                    targetTrackHeaderComp->m_clipComponents.add(clipComp);
                }
            }
            clipComp->moveToTrack(*targetTrackHeaderComp->getEngineTrack());
        }
    }
    auto fileTreeComp = dynamic_cast<FileTreeComponent*>(dragSourceDetails.sourceComponent.get());
    if (fileTreeComp && targetTrackHeaderComp)
    {
        auto track = dynamic_cast<tracktion_engine::AudioTrack*>(targetTrackHeaderComp->getEngineTrack());
        if (track)
        {
            auto f = fileTreeComp->getSelectedFile();
            tracktion_engine::AudioFile audioFile(m_edit.engine, f);
            if (audioFile.isValid())
                if (auto newClip = track->insertWaveClip(f.getFileNameWithoutExtension(), f,
                    { { timePos, 0.0 + audioFile.getLength() }, 0.0 }, false))
                {
                    newClip->setColour(track->getColour());
                    //resized();
                }
        }
    }


    Logger::outputDebugString("AR:Item dropped" );

    //sendChangeMessage();
    //resized();
    //repaint();
}

void ArrangerComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    TrackHeaderComponent* targetTrackHeaderComp = nullptr;
    for (auto& trackComp : m_trackComponents)
    {
        
        if ((   getMouseXYRelative().getY() - m_yPos) > (trackComp->getY() - 50)
            && (getMouseXYRelative().getY() - m_yPos) < ((trackComp->getY() - 50) + trackComp->getHeight()))
        {
            targetTrackHeaderComp = trackComp;
        }
    }
    auto oldBounds = dragSourceDetails.sourceComponent.get()->getLocalBounds();
    if (dragSourceDetails.description == "Clip")
    {
        auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
        if (clipComp && targetTrackHeaderComp)
        {
            auto newY = targetTrackHeaderComp->getY() - oldBounds.getY();
            clipComp->setBounds(clipComp->getX(), newY - 50 + m_yPos,  clipComp->getWidth(), clipComp->getHeight());
        }
    }
}

void ArrangerComponent::ClipsMoved()
{
    auto area = getLocalBounds();
    for (auto& trackComp : m_trackComponents)
    {
        for(auto& clipComp : trackComp->m_clipComponents)
        {
            auto& clip = clipComp->getEngineClip();
                const double length = clip.getLengthInBeats() * m_state.m_pixelPerBeat;
                const double start = clip.getStartBeat() * m_state.m_pixelPerBeat;
                auto clipRect = Rectangle<int>(area.getX() + start,
                    area.getY(),
                    length,
                    trackComp->getEngineTrack()->defaultTrackHeight);
                 
                clipComp->setBounds(clipRect);
        }
        area.removeFromTop(trackComp->getEngineTrack()->defaultTrackHeight);
    }
    Logger::outputDebugString("AR:Clips moved" );
}

void ArrangerComponent::mouseDown(const MouseEvent& event)
{
    if (event.mods.isLeftButtonDown())
    {
        m_state.m_selectionManager.deselectAll();
    }
}

inline void ArrangerComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (event.mods.isCtrlDown() || event.mods.isCommandDown())
    {
        m_state.m_pixelPerBeat = jmax(3.0, (m_state.m_pixelPerBeat + (wheel.deltaY * 10.0)));
    }
    else if(event.mods.isShiftDown())
    {
        //move left right
    }
    
    sendChangeMessage();
    repaint();
}

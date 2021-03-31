#include "TrackComponent.h"
#include "NextLookAndFeel.h"
#include "EditComponent.h"

TrackComponent::TrackComponent (EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs), m_track (t)
{
    setWantsKeyboardFocus(true);

    m_editViewState.m_state.addListener (this);
    m_editViewState.m_selectionManager.addChangeListener (this);
    m_track->state.addListener(this);
    m_track->edit.getTransport().addChangeListener (this);

    markAndUpdate (updateClips);
    addAndMakeVisible(m_trackOverlay);
    m_trackOverlay.setAlwaysOnTop(true);
    m_trackOverlay.setVisible(false);
}

TrackComponent::~TrackComponent()
{
    m_track->state.removeListener (this);
    m_editViewState.m_selectionManager.removeChangeListener (this);
    m_editViewState.m_state.removeListener(this);
    m_track->edit.getTransport().removeChangeListener (this);
}

void TrackComponent::paint (juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff181818));
    g.fillAll ();
    g.setColour(juce::Colour(0xff2b2b2b));
    g.drawRect(0,0, getWidth(), getHeight() );
    double x2 = m_editViewState.m_viewX2;
    double x1 = m_editViewState.m_viewX1;
    g.setColour(juce::Colour(0xff333333));
    double zoom = x2 -x1;
    int firstBeat = static_cast<int>(x1);
    if(m_editViewState.beatsToX(firstBeat,getWidth()) < 0)
    {
        firstBeat++;
    }

    if (m_editViewState.m_selectionManager.isSelected (m_track.get()))
    {
        g.setColour (juce::Colour(0xff202020));

        auto rc = getLocalBounds();
        if (m_editViewState.m_showHeaders) rc = rc.withTrimmedLeft (-4);
        if (m_editViewState.m_showFooters) rc = rc.withTrimmedRight (-4);

        g.fillRect (rc);
        g.setColour(juce::Colour(0xff333333));
    }

    auto pixelPerBeat = getWidth() / zoom;
    for (int beat = firstBeat - 1; beat <= m_editViewState.m_viewX2; beat++)
    {
        const int BeatX = m_editViewState.beatsToX(beat, getWidth());
        auto zBars = 16;
        if (zoom < 240)
        {
            zBars /= 2;
        }
        if (zoom < 120)
        {
            zBars /=2;
        }
        if (beat % zBars == 0)
        {
            g.drawLine(BeatX, 0, BeatX, getHeight());
        }

        if (zoom < 60)
        {
            g.drawLine(BeatX,0, BeatX, getHeight());
        }
        if (zoom < 25)
        {
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5)
            {
                g.drawLine(BeatX + quarterBeat * i ,0,
                           BeatX + quarterBeat * i ,getHeight());
                i++;
            }
        }
    }
    if (isOver)
    {
        g.setColour(juce::Colours::white);
        g.drawRect (getLocalBounds ());
    }
}

void TrackComponent::mouseDown (const juce::MouseEvent&event)
{
    bool isMidiTrack = m_track->state.getProperty (IDs::isMidiTrack);
    if (event.mods.isRightButtonDown())
    {
//        juce::PopupMenu m;

    }
    else if (event.mods.isLeftButtonDown ())
    {
        if (event.getNumberOfClicks () > 1)
        {
            if (isMidiTrack)
            {
                createNewMidiClip (m_editViewState.xToBeats (event.x,getWidth ()));
                resized ();
            }
        }
        else
        {
            getParentComponent ()->mouseDown
                    (event.getEventRelativeTo (getParentComponent ()));
        }
    }
}

void TrackComponent::mouseUp(const juce::MouseEvent &e)
{
    if (!e.mouseWasDraggedSinceMouseDown ())
    {
        m_editViewState.m_selectionManager.deselectAll ();
    }
}


void TrackComponent::changeListenerCallback (juce::ChangeBroadcaster* changebroadcaster)
{
    if (changebroadcaster == &m_editViewState.m_selectionManager)
    {
        for (auto & clipcomps : m_clips)
        {
            clipcomps->repaint ();
        }
    }
    markAndUpdate (updateRecordClips);
}

void TrackComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::Clip::isClipState (v))
    {
        if (i == te::IDs::start
            || i == te::IDs::length)
        {
            markAndUpdate (updatePositions);
        }
    }

    if (v.hasType (te::IDs::NOTE))
    {
        if (i != te::IDs::c)
        {
            for (auto &clip : m_clips)
            {
                clip->repaint ();
            }
        }
    }


    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            repaint();
            //markAndUpdate (updatePositions);
        }
    }
    if(i.toString() == "bpm")
    {
         markAndUpdate(updateClips);
    }
}

void TrackComponent::valueTreeChildAdded (juce::ValueTree&v, juce::ValueTree& c)
{
    if (te::Clip::isClipState (c))
    {
        markAndUpdate (updateClips);
    }
    if (v.hasType (te::IDs::SEQUENCE))
    {
        for (auto &clip : m_clips)
        {
            clip->repaint ();
        }
    }
}

void TrackComponent::valueTreeChildRemoved (juce::ValueTree&v, juce::ValueTree& c, int)
{
    if (v.hasType (te::IDs::SEQUENCE))
    {
        for (auto &clip : m_clips)
        {
            clip->repaint ();
        }
    }
    if (te::Clip::isClipState (c))
    {
        markAndUpdate (updateClips);
    }

}

void TrackComponent::valueTreeChildOrderChanged (juce::ValueTree& v, int a, int b)
{
    if (te::Clip::isClipState (v.getChild (a)))
        markAndUpdate (updatePositions);
    else if (te::Clip::isClipState (v.getChild (b)))
        markAndUpdate (updatePositions);
}

void TrackComponent::handleAsyncUpdate()
{
    if (compareAndReset (updateClips))
        buildClips();
    if (compareAndReset (updatePositions))
    {
        resized();
        for (auto &cc : m_clips)
        {
            cc->resized ();
        }
    }

    if (compareAndReset (updateRecordClips))
        buildRecordClips();
}

void TrackComponent::resized()
{
    for (auto cc : m_clips)
    {
        auto c = cc->getClip();
        int startTime = m_editViewState.beatsToX (c->getStartBeat (), getWidth());
        int endTime = m_editViewState.beatsToX (c->getEndBeat (), getWidth());

        cc->setBounds (startTime, 0, endTime - startTime, getHeight());
    }
    m_trackOverlay.setBounds(0, 0, getWidth(), getHeight());
}

void TrackComponent::inserWave(juce::File f, double time)
{
    tracktion_engine::AudioFile audioFile(m_editViewState.m_edit.engine, f);
    if (audioFile.isValid() && !(m_track->state.getProperty (IDs::isMidiTrack)))
    {
        if (auto audioTrack = dynamic_cast<tracktion_engine::AudioTrack*>(
                                                                m_track.get()))
        {
            if (auto newClip = audioTrack->insertWaveClip(
                        f.getFileNameWithoutExtension()
                      , f
                      , { { time, time + audioFile.getLength() }, 0.0 }
                      , true))
            {
                newClip->setColour(m_track->getColour());
            }
        }
    }
}

bool TrackComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::createFromDescription("CTRL + D"))
    {
        auto clipSelection = m_editViewState.m_selectionManager
                .getSelectedObjects ()
                .getItemsOfType<te::Clip>();

        m_editViewState.m_selectionManager.deselectAll ();

        auto selectionRange = te::getTimeRangeForSelectedItems (clipSelection);
        m_editViewState.m_edit.getTransport ().setCurrentPosition (selectionRange.end);
        for (auto& selectedClip : clipSelection)
        {
            m_editViewState.m_selectionManager.addToSelection
                    (te::duplicateClip (*selectedClip));
        }

        te::moveSelectedClips (m_editViewState.m_selectionManager.getItemsOfType<te::Clip>()
                               ,m_editViewState.m_edit
                               ,te::MoveClipAction::moveStartToCursor
                               ,true);

        return true;
    }

    if (key == juce::KeyPress::deleteKey)
    {
        EngineHelpers::deleteSelectedClips (m_editViewState);
        return true;
    }
    return false;
}

void TrackComponent::buildClips()
{
    m_clips.clear();
    auto wrongTrack = false;
    if (auto ct = dynamic_cast<te::ClipTrack*> (m_track.get()))
    {
        for (auto c : ct->getClips())
        {
            c->setColour (m_track->getColour ());
            ClipComponent* cc = nullptr;

            if (dynamic_cast<te::WaveAudioClip*> (c))
            {
                if (!isMidiTrack ())
                {
                    cc = new AudioClipComponent (m_editViewState, c);
                }
                else
                {
                    GUIHelpers::log ("couldn't insert AudioClip on Miditrack");
                    c->removeFromParentTrack ();
                    wrongTrack = true;
                }

            }
            else if (dynamic_cast<te::MidiClip*> (c))
            {
                if (isMidiTrack ())
                {
                    cc = new MidiClipComponent (m_editViewState, c);
                }
                else
                {
                    GUIHelpers::log("couldn't insert MidiClip on Audiotrack");
                    c->removeFromParentTrack ();
                    wrongTrack = true;
                }
            }
            else
                cc = new ClipComponent (m_editViewState, c);

            if (cc)
            {
                m_clips.add (cc);
                addAndMakeVisible (cc);
                if (auto editcomp = dynamic_cast<EditComponent*>(getParentComponent ()))
                {
                    if (auto midiClipcomp = dynamic_cast<MidiClipComponent*>(cc))
                    {
                        midiClipcomp->addChangeListener (&editcomp->lowerRange ());
                    }
                }
            }
        }
    }
    if (wrongTrack)
        m_editViewState.m_edit.undo ();
    else
        resized();
}

void TrackComponent::buildRecordClips()
{
    bool needed = false;
    if (m_track->edit.getTransport().isRecording())
    {
        for (auto in : m_track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive() && m_track == *(in->getTargetTracks().getFirst()))
            {
                needed = true;
                break;
            }
        }
    }

    if (needed)
    {
        recordingClip = std::make_unique<RecordingClipComponent>
                                            (m_track, m_editViewState);
        addAndMakeVisible (*recordingClip);
    }
    else
    {
        recordingClip = nullptr;
    }
}

tracktion_engine::MidiClip::Ptr TrackComponent::createNewMidiClip(double beatPos)
{
    if (auto audiotrack = dynamic_cast<te::AudioTrack*>(m_track.get ()))
    {
        te::EditTimeRange newPos;
        newPos.start = m_editViewState.beatToTime (beatPos);
        newPos.end = newPos.start + m_editViewState.beatToTime (4);
        return audiotrack->insertMIDIClip (newPos,&m_editViewState.m_selectionManager);
    }
    return nullptr;
}

TrackOverlayComponent& TrackComponent::getTrackOverlay()
{
    return m_trackOverlay;
}

bool TrackComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        return true;
    }
    else
    {
        return false;
    }
}

juce::OwnedArray<ClipComponent> &TrackComponent::getClipComponents()
{
    return m_clips;
}

te::Track::Ptr TrackComponent::getTrack() const
{
    return m_track;
}

void TrackComponent::itemDragMove(
        const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    isOver = true;
    repaint();
}
void TrackComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    isOver = false;
    repaint ();
}
void TrackComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if(dragSourceDetails.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<juce::ListBox*>(
            dragSourceDetails.sourceComponent.get ()))
        {
            if (auto lbm =
                dynamic_cast<PluginListBoxComponent*>(listbox->getModel()))
            {
                getTrack()->pluginList.insertPlugin(
                    lbm->getSelectedPlugin()
                    , getTrack()->pluginList.size() - 2 //set before LevelMeter and Volume
                    , nullptr);
            }
        }
    }
    isOver = false;
    repaint();
}

//==============================================================================



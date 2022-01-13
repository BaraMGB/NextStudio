#include "TrackComponent.h"
#include "EditComponent.h"
//#include <utility>


TrackComponent::TrackComponent (EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs), m_track (std::move(t))
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
    g.setColour(juce::Colour(0xff222222));
    if (isSelected ())
        g.setColour(juce::Colour(0xff444444));
    g.fillAll ();
    g.setColour(  juce::Colour(0xff2b2b2b));
    g.drawRect(0,0, getWidth(), m_track->state.getProperty(
                   tracktion_engine::IDs::height));
    double x2beats = m_editViewState.m_viewX2;
    double x1beats = m_editViewState.m_viewX1;
    g.setColour(juce::Colour(0xff333333));

    GUIHelpers::drawBarsAndBeatLines (g, m_editViewState, x1beats, x2beats, getBounds ());
    if (isOver)
    {
        g.setColour(juce::Colours::white);
        g.drawRect (getLocalBounds ());
    }
}

void TrackComponent::mouseDown (const juce::MouseEvent&event)
{
    //m_editViewState.m_selectionManager.selectOnly (m_track);
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
                createNewMidiClip (m_editViewState.xToBeats (event.x,getWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
                resized ();
            }
        }
        else
        {
            if (auto ec = dynamic_cast<EditComponent*>(getParentComponent ()))
            {
                ec->getLasso()->startLasso(event.getEventRelativeTo (ec->getLasso()));
            }
        }
    }
}

void TrackComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (auto ec = dynamic_cast<EditComponent*>(getParentComponent ()))
    {
        if ( ec->getLasso ()->isVisible())
        {
            ec->getLasso()->updateLasso(e.getEventRelativeTo (ec->getLasso()));
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
        }
    }
}

void TrackComponent::mouseUp(const juce::MouseEvent &e)
{
    if (!e.mouseWasDraggedSinceMouseDown ())
    {
        m_editViewState.m_selectionManager.deselectAll ();
    }
    if (auto ec = dynamic_cast<EditComponent*>(getParentComponent ()))
    {
        if (ec->getLasso ()->isVisible())
        {
            ec->getLasso()->stopLasso();
            setMouseCursor (juce::MouseCursor::NormalCursor);
        }
    }
}


void TrackComponent::changeListenerCallback (juce::ChangeBroadcaster* cbc)
{
    if (cbc == &m_editViewState.m_selectionManager)
    {
        for (auto & cc: m_clipComponents)
        {
            cc->repaint ();
        }
        getParentComponent()->repaint();
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
            for (auto &clip : m_clipComponents)
            {
                clip->repaint ();
            }
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
        for (auto &clip : m_clipComponents)
        {
            clip->repaint ();
        }
    }
}

void TrackComponent::valueTreeChildRemoved (juce::ValueTree&v, juce::ValueTree& c, int)
{
    if (v.hasType (te::IDs::SEQUENCE))
    {
        for (auto &clip : m_clipComponents)
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
    if (te::Clip::isClipState (v.getChild (a))
        || te::Clip::isClipState (v.getChild (b)))
        markAndUpdate (updatePositions);
}

void TrackComponent::handleAsyncUpdate()
{
    if (compareAndReset (updateClips))
        buildClips();
    if (compareAndReset (updatePositions))
    {
        resized();
        for (auto &cc : m_clipComponents)
        {
            cc->resized ();
        }
    }

    if (compareAndReset (updateRecordClips))
        buildRecordClips();
}

void TrackComponent::resized()
{
    for (auto cc : m_clipComponents)
    {
        if (auto c = cc->getClip ())
        {
            int startX = m_editViewState.beatsToX (c->getStartBeat (), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int endX = m_editViewState.beatsToX (c->getEndBeat (), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int clipHeight = (bool) m_track->state.getProperty (IDs::isTrackMinimized)
                    ? (int) m_editViewState.m_trackHeightMinimized
                    : (int) m_track->state.getProperty(
                          tracktion_engine::IDs::height, 50);

            cc->setBounds (startX, 0, endX - startX + 1, clipHeight);
        }
    }
    m_trackOverlay.setBounds(0, 0, getWidth(), m_track->state.getProperty(
                                 tracktion_engine::IDs::height, 50));
    double nextLaneStart = m_track->state.getProperty(
                tracktion_engine::IDs::height);
    for (auto al : m_automationLanes)
    {
        int height = al->getCurve ().state.getProperty(
                    tracktion_engine::IDs::height, (int) m_editViewState.m_trackHeightMinimized);
        al->setBounds(0, (int) nextLaneStart, getWidth(), height);
        nextLaneStart = nextLaneStart + al->getHeight();
    }
}

void TrackComponent::insertWave(const juce::File& f, double time)
{
    tracktion_engine::AudioFile audioFile(m_editViewState.m_edit.engine, f);
    if (audioFile.isValid() && !isMidiTrack ())
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

void TrackComponent::buildClips()
{
    m_clipComponents.clear();
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
                    GUIHelpers::log ("couldn't insert audio clip on midi track");
                    //c->removeFromParentTrack ();
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
                    GUIHelpers::log("couldn't insert midi clip on audio track");
                    wrongTrack = true;
                }
            }
            else
                cc = new ClipComponent (m_editViewState, c);

            if (cc)
            {
                m_clipComponents.add (cc);
                addAndMakeVisible (cc);
                if (auto ec = dynamic_cast<EditComponent*>(getParentComponent ()))
                {
                    if (auto mcc = dynamic_cast<MidiClipComponent*>(cc))
                    {
                        mcc->addChangeListener (&ec->lowerRange ());
                    }
                }
            }
        }
    }
    if (wrongTrack)
        m_editViewState.m_edit.undo ();
    else
        resized();
    buildAutomationLanes();
}

void TrackComponent::buildAutomationLanes()
{
    m_automationLanes.clear(true);
    for (auto apEditItems : m_track->getAllAutomatableEditItems())
    {
        for (auto ap : apEditItems->getAutomatableParameters())
        {
            if (ap->getCurve().getNumPoints() > 0)
            {
                m_automationLanes.add(new AutomationLaneComponent(ap->getCurve(), m_editViewState));
                addAndMakeVisible(m_automationLanes.getLast());
            }

        }
    }
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
        newPos.start = juce::jmax(0.0, m_editViewState.beatToTime (beatPos));
        newPos.end = newPos.start + m_editViewState.beatToTime (4);
        auto mc = audiotrack->insertMIDIClip (
                    newPos
                  , &m_editViewState.m_selectionManager);
        mc->setName (audiotrack->getName ());
        return mc;
    }
    return nullptr;
}

TrackOverlayComponent& TrackComponent::getTrackOverlay()
{
    return m_trackOverlay;
}

bool TrackComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (!(dragSourceDetails.description == "PluginListEntry"))
        return false;
    return true;
}

juce::OwnedArray<ClipComponent> &TrackComponent::getClipComponents()
{
    return m_clipComponents;
}

te::Track::Ptr TrackComponent::getTrack() const
{
    return m_track;
}

void TrackComponent::itemDragMove(
        const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
{
    isOver = true;
    repaint();
}
void TrackComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
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

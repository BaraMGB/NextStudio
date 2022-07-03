#include "TrackComponent.h"
#include "EditComponent.h"
//#include <utility>


TrackComponent::TrackComponent (EditViewState& evs, LowerRangeComponent& lr, te::Track::Ptr t)
    : m_editViewState (evs), m_lowerRange(lr),  m_track (std::move(t))
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
    auto area = getLocalBounds ();
    area.reduce (0, 1);
    g.setColour(juce::Colour(0xff181818));
    g.fillRect (area);

    double x2beats = m_editViewState.m_viewX2;
    double x1beats = m_editViewState.m_viewX1;

    if (isSelected ())
    {
        g.setColour(juce::Colour(0xff242424));
        g.fillRect(area);
    }

    GUIHelpers::drawBarsAndBeatLines (g, m_editViewState, x1beats, x2beats, getBounds ());
    if (isOver)
    {
        g.setColour(juce::Colours::white);
        g.drawRect (getLocalBounds ());
    }
}

void TrackComponent::mouseDown (const juce::MouseEvent&e)
{
    bool isMidiTrack = m_track->state.getProperty (IDs::isMidiTrack);
    if (e.mods.isRightButtonDown())
    {
//        juce::PopupMenu m;
    }
    else if (e.mods.isLeftButtonDown ())
    {
        if (e.getNumberOfClicks () > 1)
        {
            if (isMidiTrack)
            {
                auto st = m_editViewState.getBestSnapType(
                    m_editViewState.m_viewX1,
                    m_editViewState.m_viewX2,
                    getWidth());

                createNewMidiClip (m_editViewState.getQuantizedBeat(
                    xToBeats(e.x),
                    st,
                    true));

                resized ();
            }
        }
        else
        {
            if (auto se = dynamic_cast<SongEditorView*>(getParentComponent ()))
                se->startLasso(e.getEventRelativeTo (se));
        }
    }
}

void TrackComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (auto se = dynamic_cast<SongEditorView*>(getParentComponent ()))
    {
        if (se->getLasso ().isVisible())
        {
            se->updateLasso(e.getEventRelativeTo (&se->getLasso ()));
        }
    }
}

void TrackComponent::mouseUp(const juce::MouseEvent &e)
{
    if (!e.mouseWasDraggedSinceMouseDown () && e.getNumberOfClicks() < 2)
    {
		getParentComponent()->mouseUp(e.getEventRelativeTo(getParentComponent()));
    }

    if (auto se = dynamic_cast<SongEditorView*>(getParentComponent ()))
    {
        if (se->getLasso ().isVisible())
        {
            se->getLasso ().stopLasso();
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
            int startX = m_editViewState.beatsToX (c->getStartBeat ().inBeats(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int endX = m_editViewState.beatsToX (c->getEndBeat ().inBeats(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int clipHeight = (bool) m_track->state.getProperty (IDs::isTrackMinimized)
                    ? (int) m_editViewState.m_trackHeightMinimized
                    : (int) m_track->state.getProperty(
                          tracktion_engine::IDs::height, 50);

            cc->setBounds (startX, 0, endX - startX + 1, clipHeight);
        }
    }
    m_trackOverlay.setBounds(0, 0, getWidth(), (bool) m_track->state.getProperty (IDs::isTrackMinimized)
                    ? (int) m_editViewState.m_trackHeightMinimized
                    : (int) m_track->state.getProperty(
                          tracktion_engine::IDs::height, 50));

    double nextLaneStart = m_track->state.getProperty(
                tracktion_engine::IDs::height);
	if (m_track->isFolderTrack())
		nextLaneStart = m_editViewState.m_folderTrackHeight;
    for (auto al : m_automationLanes)
    {
        int height = al->getCurve ().state.getProperty(
                    tracktion_engine::IDs::height, (int) m_editViewState.m_trackDefaultHeight);
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
                      ,  { { {}, tracktion::core::TimePosition::fromSeconds (audioFile.getLength()) }, {} }
                      , true))
            {
                newClip->setColour(m_track->getColour());
				newClip->setAutoTempo(false);
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
                    GUIHelpers::log ("couldn't insert audio clip on this track");
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
                    GUIHelpers::log("couldn't insert midi clip on this track");
                    wrongTrack = true;
                }
            }
            else if (dynamic_cast<te::CollectionClip*> (c))
            {
                if (isFolderTrack())
                {
                    cc = new ClipComponent (m_editViewState, c);
                }
                else
                {
                    GUIHelpers::log("couldn't insert collection clip on this track");
                    wrongTrack = true;
                }
            }
            else
                cc = new ClipComponent (m_editViewState, c);

            if (cc)
            {
                m_clipComponents.add (cc);
                addAndMakeVisible (cc);

                if (auto mcc = dynamic_cast<MidiClipComponent*>(cc))
                {
                    mcc->addChangeListener (&m_lowerRange);
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
                m_automationLanes.getLast()->grabKeyboardFocus();
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
    if (auto at = dynamic_cast<te::AudioTrack*>(m_track.get ()))
    {
        auto start = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime (beatPos)));
        auto end   = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime (beatPos)) + m_editViewState.beatToTime (4));

        tracktion::core::TimeRange newPos(start, end);
        at->deleteRegion(newPos, &m_editViewState.m_selectionManager);

        auto mc = at->insertMIDIClip (newPos, &m_editViewState.m_selectionManager);
        mc->setName (at->getName ());
        GUIHelpers::centerMidiEditorToClip(m_editViewState, mc);

        return mc;
    }
    return nullptr;
}
bool TrackComponent::isSelected()
{
    return m_editViewState.m_selectionManager.getItemsOfType<te::Track>().contains (m_track);
}
TrackOverlayComponent& TrackComponent::getTrackOverlay()
{
    return m_trackOverlay;
}
bool TrackComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
        return true;

	if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
		auto f = te::AudioFile(m_editViewState.m_edit.engine, fileTreeComp->getSelectedFile());
		if (f.isValid())
			return true;
	}
	if (auto lb = dynamic_cast<juce::ListBox*>(dragSourceDetails.sourceComponent.get()))
		if (auto fileListComp = dynamic_cast<FileListBoxComponent*>(lb->getModel ()))
		{
			tracktion_engine::AudioFile audioFile(
                    m_editViewState.m_edit.engine, fileListComp->getFileList ()[lb->getLastRowSelected ()]);
                if (audioFile.isValid ())
					return true;
		}
  
	return false;
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
    auto dropPos = dragSourceDetails.localPosition;
    auto dropTime = m_editViewState.xToTime (
        dropPos.getX()
        , getWidth()
        , m_editViewState.m_viewX1
        , m_editViewState.m_viewX2);
    dropTime = juce::jlimit(0.0,(double) m_editViewState.m_viewX2, dropTime);
 
        if (auto lb = dynamic_cast<juce::ListBox*>(dragSourceDetails.sourceComponent.get()))
        {
            if (auto fileListComp =
                    dynamic_cast<FileListBoxComponent*>(lb->getModel ()))
            {
                tracktion_engine::AudioFile audioFile(
                    m_editViewState.m_edit.engine
                    , fileListComp->getFileList ()[lb->getLastRowSelected ()]);
                if (audioFile.isValid ())
                {
                    insertWave(
                        fileListComp->getFileList()[lb->getLastRowSelected()],
                        getSnapedTime(dropTime));
                }
            }
        }
		if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
        {
            auto f = fileTreeComp->getSelectedFile();
			insertWave(f, getSnapedTime(dropTime));
        }


     if (dragSourceDetails.description == "PluginListEntry")
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
bool TrackComponent::isFolderTrack()
{
    return m_track->isFolderTrack();
}
double TrackComponent::getSnapedTime(double t)
{
	auto st =  m_editViewState.getBestSnapType(m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth());
	return m_editViewState.getSnapedTime(t, st, true);
}

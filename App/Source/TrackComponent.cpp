#include "TrackComponent.h"
#include "NextLookAndFeel.h"
#include "EditComponent.h"
//==============================================================================
TrackHeaderComponent::TrackHeaderComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_armButton,
                                         &m_muteButton,
                                         &m_soloButton
                                         });


    m_trackName.setText(m_track->getName(), juce::NotificationType::dontSendNotification);
    m_trackName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);

    if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        auto audioTrackPtr{dynamic_cast<te::AudioTrack*>(m_track.get())};
        levelMeterComp = std::make_unique<LevelMeterComponent>(audioTrackPtr->getLevelMeterPlugin()->measurer);
        addAndMakeVisible(levelMeterComp.get());

        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), juce::dontSendNotification);
        m_armButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), juce::dontSendNotification);
        };
        m_muteButton.onClick = [audioTrack] { audioTrack->setMute (! audioTrack->isMuted (false)); };
        m_soloButton.onClick = [audioTrack] { audioTrack->setSolo (! audioTrack->isSolo (false)); };

        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), juce::dontSendNotification);

        m_volumeKnob.setOpaque(false);
        addAndMakeVisible(m_volumeKnob);
        m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
        m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob.getValueObject().referTo(audioTrack->getVolumePlugin()->volume.getPropertyAsValue());
            m_volumeKnob.setValue(audioTrack->getVolumePlugin()->volume);

        }
        m_volumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        m_volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
    }
    else
    {
        m_armButton.setVisible (false);
        m_muteButton.setVisible (false);
        m_soloButton.setVisible (false);
    }

    m_track->state.addListener (this);
    inputsState = m_track->edit.state.getChildWithName (te::IDs::INPUTDEVICES);
    inputsState.addListener (this);

    valueTreePropertyChanged (m_track->state, te::IDs::mute);
    valueTreePropertyChanged (m_track->state, te::IDs::solo);
    valueTreePropertyChanged (inputsState, te::IDs::targetIndex);


}

TrackHeaderComponent::~TrackHeaderComponent()
{
    m_track->state.removeListener (this);
}

void TrackHeaderComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::TrackList::isTrack (v))
    {
        if (i == te::IDs::mute)
            m_muteButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        else if (i == te::IDs::solo)
            m_soloButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        else if (i == te::IDs::height)
            getParentComponent()->resized();
    }
    else if (v.hasType (te::IDs::INPUTDEVICES)
             || v.hasType (te::IDs::INPUTDEVICE)
             || v.hasType (te::IDs::INPUTDEVICEDESTINATION))
    {
        if (auto at = dynamic_cast<te::AudioTrack*> (m_track.get()))
        {
            m_armButton.setEnabled (EngineHelpers::trackHasInput (*at));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*at), juce::dontSendNotification);
        }
    }
}

void TrackHeaderComponent::showPopupMenu(tracktion_engine::AudioTrack *at)
{
    at->edit.playInStopEnabled = true;
    juce::PopupMenu m;
    m.addItem(2000, "delete Track");
    m.addSeparator();

    if (EngineHelpers::trackHasInput(*at))
    {
        bool ticked = EngineHelpers::isInputMonitoringEnabled(*at);
        m.addItem(1000, "Input Monitoring", true, ticked);
        m.addSeparator();
    }

    int id = 1;
    for (auto instance: at->edit.getAllInputDevices())
    {
        if (instance->getInputDevice().getDeviceType()
            == te::InputDevice::waveDevice)
        {
            bool ticked = instance->getTargetTracks().getFirst() == at;
            m.addItem(id++,
                      instance->getInputDevice().getName(),
                      true,
                      ticked);
        }
    }

    m.addSeparator();

    id = 100;
    for (auto instance: at->edit.getAllInputDevices())
    {
        if (instance->getInputDevice().getDeviceType()
            == te::InputDevice::physicalMidiDevice)
        {
            bool ticked = instance->getTargetTracks().getFirst() == at;
            m.addItem(id++,
                      instance->getInputDevice().getName(),
                      true,
                      ticked);
        }
    }

    const int result = m.show();

    if (result == 2000)
    {
        deleteTrackFromEdit();
    }
    else if (result == 1000)
    {
        EngineHelpers::enableInputMonitoring(
            *at, !EngineHelpers::isInputMonitoringEnabled(*at));
    }
    else if (result >= 100)
    {
        int id = 100;

        for (auto instance: at->edit.getAllInputDevices())
        {
            if (instance->getInputDevice().getDeviceType()
                == te::InputDevice::physicalMidiDevice)
            {
                if (id == result)
                {
                    {
                        instance->setTargetTrack(*at, 0, true);
                    }
                }
                id++;
            }
        }
        EngineHelpers::enableInputMonitoring(
            *at, !EngineHelpers::isInputMonitoringEnabled(*at));
        EngineHelpers::enableInputMonitoring(
            *at, !EngineHelpers::isInputMonitoringEnabled(*at));
    }
    else if (result >= 1)
    {
        int id = 1;
        for (auto instance: at->edit.getAllInputDevices())
        {
            if (instance->getInputDevice().getDeviceType()
                == te::InputDevice::waveDevice)
            {
                if (id == result)
                {
                    if (instance->getTargetTracks().getFirst() == at)
                    {
                        instance->removeTargetTrack(*at);
                    }
                    else
                    {
                        instance->setTargetTrack(*at, 0, true);
                    }
                }
                id++;
            }
        }
    }
}

void TrackHeaderComponent::deleteTrackFromEdit()
{
    m_track->deselect();
    m_track->edit.deleteTrack(m_track);
    auto i = tracktion_engine::getAllTracks(editViewState.m_edit).getLast();

    if (!(i->isArrangerTrack()
        || i->isTempoTrack()
        || i->isMarkerTrack()
        || i->isChordTrack()))
    {
        editViewState.m_selectionManager.selectOnly(i);
    }
    else
    {
        editViewState.m_selectionManager.deselectAll();
    }
}

void TrackHeaderComponent::paint (juce::Graphics& g)
{   
    auto cornerSize = 10.0f;
    juce::Rectangle<float> area = getLocalBounds().toFloat();

    area.reduce(1, 1);

    auto buttonColour = juce::Colour(0xff4b4b4b);

    if (!editViewState.m_selectionManager.isSelected (m_track))
    {
        buttonColour = buttonColour.darker (0.4f);
    }
    g.setColour(buttonColour);
    GUIHelpers::drawRoundedRectWithSide(g,area,cornerSize,true);

    if (drawOverlayTrackColour)
    {
        auto trackColorOverlay = m_track->getColour ().darker (0.5);
        g.setColour (trackColorOverlay.withAlpha (0.1f));

        GUIHelpers::drawRoundedRectWithSide(g, area, cornerSize, true);
    }

    // TrackColour
    juce::Rectangle<float> trackColorIndicator = getLocalBounds().removeFromLeft(15).toFloat();
    auto trackColor =  m_track->getColour();

    g.setColour (trackColor);
    GUIHelpers::drawRoundedRectWithSide(g, trackColorIndicator.reduced(1,1), cornerSize, true);
    if (m_isAboutToResizing)
    {
        g.setColour(juce::Colour(0x66ffffff));
        g.drawRect(getLocalBounds().removeFromBottom(1));
    }
    if (m_isResizing)
    {
        g.setColour(juce::Colour(0xffffffff));
        g.drawRect(getLocalBounds().removeFromBottom(3));
    }
}

void TrackHeaderComponent::mouseDown (const juce::MouseEvent& event)
{
    if (!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown ())
            {
                if (auto at = dynamic_cast<te::AudioTrack*>(m_track.get()))
                {
                    showPopupMenu(at);
                }
            }
            else if (event.mods.isShiftDown())
            {
                if (editViewState.m_selectionManager.getNumObjectsSelected())
                {
                    editViewState.m_selectionManager.addToSelection(m_track);
                }
            }
            else
            {
                editViewState.m_selectionManager.selectOnly(m_track);
                m_yPosAtMouseDown = event.mouseDownPosition.y;
                m_trackHeightATMouseDown = m_track->state.getProperty(te::IDs::height);
            }
    }
}



void TrackHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_yPosAtMouseDown > m_trackHeightATMouseDown - 10)
    {
        m_isResizing = true;
        auto newHeight = juce::jlimit(static_cast<int> (m_track->minTrackHeight)
                                     ,static_cast<int> (m_track->maxTrackHeight)
                                     ,static_cast<int> (m_trackHeightATMouseDown
                                        + event.getDistanceFromDragStartY()));

        m_track->state.setProperty(te::IDs::height
                                   , newHeight
                                   , &(editViewState.m_edit.getUndoManager()));
    }


}

void TrackHeaderComponent::mouseUp(const juce::MouseEvent &event)
{
    m_isResizing = false;
    repaint();
}

void TrackHeaderComponent::mouseMove(const juce::MouseEvent &event)
{
    if (event.getPosition().y > getHeight() - 10)
    {
        m_isAboutToResizing = true;
    }
    else
    {
        m_isAboutToResizing = false;
    }
    repaint();
}

void TrackHeaderComponent::mouseExit(const juce::MouseEvent &event)
{
    m_isAboutToResizing = false;
    repaint();
}

void TrackHeaderComponent::resized()
{
    auto defaultTrackHeight = m_track->defaultTrackHeight;
    auto area = getLocalBounds().removeFromTop(defaultTrackHeight);//getLocalBounds();
    auto peakDisplay = area.removeFromRight(15);
    peakDisplay.reduce(2, 2);
    levelMeterComp->setBounds (peakDisplay);
    auto volSlider = area.removeFromRight(area.getHeight());
    m_volumeKnob.setBounds(volSlider);

    auto buttonGroup = area.removeFromRight(area.getHeight());
    auto buttonwidth = buttonGroup.getWidth() / 2;
    auto buttonHeight = buttonGroup.getHeight() / 2;
    m_soloButton.setBounds(buttonGroup.getX(), buttonGroup.getY(), buttonwidth, buttonHeight);
    m_soloButton.setComponentID ("solo");
    m_soloButton.setName ("S");
    m_muteButton.setBounds(buttonGroup.getX(), buttonGroup.getY() + buttonHeight, buttonwidth, buttonHeight);
    m_muteButton.setComponentID ("mute");
    m_muteButton.setName ("M");
    m_armButton.setBounds(buttonGroup.getX() + buttonwidth, buttonGroup.getY(), buttonwidth, buttonHeight);
    m_armButton.setComponentID ("arm");
    m_armButton.setName ("A");

    area.removeFromLeft(20);
    m_trackName.setBounds(area);
}

juce::Colour TrackHeaderComponent::getTrackColour()
{
    return m_track->getColour ();
}


//==============================================================================
TrackComponent::TrackComponent (EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs), track (t)
{
    m_editViewState.m_state.addListener (this);
    track->state.addListener(this);
    track->edit.getTransport().addChangeListener (this);

    markAndUpdate (updateClips);
    addAndMakeVisible(m_trackOverlay);
    m_trackOverlay.setAlwaysOnTop(true);
    m_trackOverlay.setVisible(false);
}

TrackComponent::~TrackComponent()
{
    track->state.removeListener (this);
    m_editViewState.m_state.removeListener(this);
    track->edit.getTransport().removeChangeListener (this);
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

    if (m_editViewState.m_selectionManager.isSelected (track.get()))
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
        const int BeatX = m_editViewState.beatsToX(beat, getWidth()) - 1;
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
}

void TrackComponent::mouseDown (const juce::MouseEvent&event)
{
    m_editViewState.m_selectionManager.selectOnly (track.get());
    if (event.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem(1, "Paste");


        const int result = m.show();

        if (result == 0)
        {
            // user dismissed the menu without picking anything
        }
        else if(result == 1)
        {
           auto insertpoint = te::EditInsertPoint(m_editViewState.m_edit);
           insertpoint.setNextInsertPoint(
                          m_editViewState.beatToTime(
                              m_editViewState.xToBeats(event.x, getWidth()))
                                                     , track);
           auto clipBoard = te::Clipboard::getInstance();
           if (clipBoard->hasContentWithType<te::Clipboard::Clips>())
           {
               clipBoard->getContentWithType<te::Clipboard::Clips>()
                            ->pasteIntoEdit(
                              te::Clipboard::ContentType::EditPastingOptions
                              (m_editViewState.m_edit, insertpoint));
           }
        }
    }
}


void TrackComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
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
            for (auto &clip : clips)
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

void TrackComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::Clip::isClipState (c))
        markAndUpdate (updateClips);
}

void TrackComponent::valueTreeChildRemoved (juce::ValueTree&v, juce::ValueTree& c, int)
{
    if (te::Clip::isClipState (c))
        markAndUpdate (updateClips);
    if (v.hasType (te::IDs::NOTE))
    {
        for (auto &clip : clips)
        {
             clip->repaint ();
        }
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
        for (auto &cc : clips)
        {
            cc->resized ();
        }
    }

    if (compareAndReset (updateRecordClips))
        buildRecordClips();
}

void TrackComponent::resized()
{
    for (auto cc : clips)
    {
        auto& c = cc->getClip();
        int startTime = m_editViewState.beatsToX (c.getStartBeat (), getWidth());
        int endTime = m_editViewState.beatsToX (c.getEndBeat (), getWidth());

        cc->setBounds (startTime, 0, endTime - startTime, getHeight());
    }
    m_trackOverlay.setBounds(0, 0, getWidth(), getHeight());
}

void TrackComponent::itemDropped(
        const DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto dropPos = dragSourceDetails.localPosition;
    auto dropTime = m_editViewState.beatToTime(
                m_editViewState.xToBeats(dropPos.getX(), getWidth()));

    //Dropping Clips, with CTRL copy Clip
    if (dragSourceDetails.description == "Clip")
    {
        auto clipboard = tracktion_engine::Clipboard::getInstance();
        if (clipboard->hasContentWithType<te::Clipboard::Clips>())
        {
            auto clipContent = clipboard
                    ->getContentWithType<te::Clipboard::Clips>();
            te::EditInsertPoint insertPoint(m_editViewState.m_edit);
            insertPoint.setNextInsertPoint(0, track);
            te::Clipboard::ContentType::EditPastingOptions options
                    (m_editViewState.m_edit, insertPoint);

            auto& clip{ clipContent->clips[0] };
            auto clipstart = static_cast<double>(clip.state.getProperty(te::IDs::start));

            if (auto clipComp = dynamic_cast<ClipComponent*>
                    (dragSourceDetails.sourceComponent.get()))
            {
                //same track? only move
                auto clickTimeOffset = clipComp->getClickPosTime();
                auto xTime = m_editViewState.beatToTime(m_editViewState.m_viewX1);
                if (clipComp->getClip().getTrack() == getTrack().get()
                && !clipComp->isCopying())
                {
                    auto rawtime = dropTime - clickTimeOffset + xTime;
                    auto snapedTime = m_editViewState.getSnapedTime (rawtime);
                    auto pasteTime = clipComp->isShiftDown ()
                                   ? rawtime
                                   : snapedTime;
                    clipComp->getClip().setStart(
                                pasteTime
                                , false
                                , true);
                }
                else
                {
                    if (!clipComp->isCopying())
                    {
                        clipComp->getClip().removeFromParentTrack();
                    }
                    else
                    {
                        clipComp->setIsCopying(false);
                    }

                    auto rawTime = dropTime - clickTimeOffset + xTime;

                    auto snapedTime = m_editViewState.getSnapedTime (rawTime);
                    auto pasteTime = clipComp->isShiftDown ()
                                   ? rawTime - clipstart
                                   : snapedTime - clipstart;
                    options.startTime = pasteTime;
                    clipContent->pasteIntoEdit(options);
                }
            }
        }
    }
    //File droped ?
    auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
            (dragSourceDetails.sourceComponent.get());
    if (fileTreeComp)
    {
        auto f = fileTreeComp->getSelectedFile();
        tracktion_engine::AudioFile audioFile(m_editViewState.m_edit.engine, f);
        if (audioFile.isValid())
        {
            if (auto audioTrack = dynamic_cast<tracktion_engine::AudioTrack*>(track.get()))
            {
                if (auto newClip = audioTrack->insertWaveClip(f.getFileNameWithoutExtension()
                                                         ,f
                                                         ,{ { dropTime, dropTime + audioFile.getLength() }, 0.0 }
                                                         , false))
                {
                    newClip->setColour(track->getColour());
                }
            }

        }
    }
    m_dragging = false;
    m_trackOverlay.setVisible(false);
    repaint();
}

void TrackComponent::itemDragMove(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "Clip")
        {
            auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
            if (clipComp)
            {
                m_dragging = true;
                m_posInClip = m_editViewState.beatsToX(
                            m_editViewState.timeToBeat(
                            clipComp->getClickPosTime()
                            ), getWidth());
                m_dragImage = clipComp->createComponentSnapshot({0
                                                                 , 0
                                                                 , clipComp->getWidth()
                                                                 , clipComp->getHeight()}, false);
                m_trackOverlay.setImage(m_dragImage);
                m_trackOverlay.setImagePos(clipComp->isShiftDown ()
                                           ? getMouseXYRelative().x
                                             - m_posInClip
                                           : m_editViewState.snapedX (
                                               getMouseXYRelative().x
                                               - m_posInClip
                                               ,getWidth ()));
                m_trackOverlay.repaint();
                m_trackOverlay.setVisible(true);
            }
    }
}

void TrackComponent::itemDragExit(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    m_dragging = false;
    m_trackOverlay.setVisible(false);
}



void TrackComponent::buildClips()
{
    clips.clear();

    if (auto ct = dynamic_cast<te::ClipTrack*> (track.get()))
    {
        for (auto c : ct->getClips())
        {
            ClipComponent* cc = nullptr;

            if (dynamic_cast<te::WaveAudioClip*> (c))
                cc = new AudioClipComponent (m_editViewState, c);
            else if (dynamic_cast<te::MidiClip*> (c))
                cc = new MidiClipComponent (m_editViewState, c);
            else
                cc = new ClipComponent (m_editViewState, c);

            clips.add (cc);
            addAndMakeVisible (cc);
        }
    }

    resized();
}

void TrackComponent::buildRecordClips()
{
    bool needed = false;
    if (track->edit.getTransport().isRecording())
    {
        for (auto in : track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive() && track == *(in->getTargetTracks().getFirst()))
            {
                needed = true;
                break;
            }
        }
    }

    if (needed)
    {
        recordingClip = std::make_unique<RecordingClipComponent> (track, m_editViewState);
        addAndMakeVisible (*recordingClip);
    }
    else
    {
        recordingClip = nullptr;
    }
}

te::Track::Ptr TrackComponent::getTrack() const
{
    return track;
}




//==============================================================================



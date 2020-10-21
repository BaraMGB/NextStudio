#include "TrackComponent.h"

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_armButton,
                                         &m_muteButton,
                                         &m_soloButton
                                         });


    m_trackName.setText(m_track->getName(), NotificationType::dontSendNotification);
    m_trackName.setColour(Label::textColourId, Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);

    if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        auto audioTrackPtr{dynamic_cast<te::AudioTrack*>(m_track.get())};
        levelMeterComp = std::make_unique<LevelMeterComponent>(audioTrackPtr->getLevelMeterPlugin()->measurer);
        addAndMakeVisible(levelMeterComp.get());

        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        m_armButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        };

        m_volumeKnob.setOpaque(false);
        addAndMakeVisible(m_volumeKnob);
        m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
        m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob.getValueObject().referTo(audioTrack->getVolumePlugin()->volume.getPropertyAsValue());
            m_volumeKnob.setValue(audioTrack->getVolumePlugin()->volume);

        }
        m_volumeKnob.setSliderStyle(Slider::RotaryVerticalDrag);
        m_volumeKnob.setTextBoxStyle(Slider::NoTextBox, 0, 0, false);
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
            m_muteButton.setToggleState ((bool)v[i], dontSendNotification);
        else if (i == te::IDs::solo)
            m_soloButton.setToggleState ((bool)v[i], dontSendNotification);
    }
    else if (v.hasType (te::IDs::INPUTDEVICES)
             || v.hasType (te::IDs::INPUTDEVICE)
             || v.hasType (te::IDs::INPUTDEVICEDESTINATION))
    {
        if (auto at = dynamic_cast<te::AudioTrack*> (m_track.get()))
        {
            m_armButton.setEnabled (EngineHelpers::trackHasInput (*at));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*at), dontSendNotification);
        }
    }
}

void TrackHeaderComponent::paint (Graphics& g)
{
    Rectangle<float> area = getLocalBounds().toFloat();

        g.setColour(m_track->getColour());
        Rectangle<float> trackColorIndicator = area.removeFromLeft(18);
        g.fillRect(trackColorIndicator);
        g.setColour(Colour(0xff343434));
        g.drawRect(trackColorIndicator);
        g.drawRect(area);
        area.reduce(1, 1);
        if (editViewState.selectionManager.isSelected(m_track))
        {
            g.setColour(Colour(0xff383838));
        }
        else
        {
            g.setColour(Colour(0xff181818));
        }

        g.fillRect(area);
}

void TrackHeaderComponent::mouseDown (const MouseEvent& event)
{

    if (!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown ())
            {
                if (auto at = dynamic_cast<te::AudioTrack*>(m_track.get()))
                {
                    PopupMenu m;
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

                    at->edit.playInStopEnabled = true;
                    auto& dm = at->edit.engine.getDeviceManager();
                    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
                    {
                        if (auto wip = dm.getMidiInDevice(i))
                        {
                            wip->setEndToEndEnabled(true);
                            wip->setEnabled(true);
                        }
                    }
                    at->edit.restartPlayback();

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

                    const int res = m.show();

                    if (res == 2000)
                    {
                        m_track->deselect();
                        m_track->edit.deleteTrack(m_track);
                        auto i = tracktion_engine::getAllTracks(editViewState.edit).getLast();

                        if (!(i->isArrangerTrack()
                            || i->isTempoTrack()
                            || i->isMarkerTrack()
                            || i->isChordTrack()))
                        {
                            editViewState.selectionManager.selectOnly(i);
                        }
                        else
                        {
                            editViewState.selectionManager.deselectAll();
                        }
                    }
                    else if (res == 1000)
                    {
                        EngineHelpers::enableInputMonitoring(
                            *at, !EngineHelpers::isInputMonitoringEnabled(*at));
                    }
                    else if (res >= 100)
                    {
                        int id = 100;

                        for (auto instance: at->edit.getAllInputDevices())
                        {
                            if (instance->getInputDevice().getDeviceType()
                                == te::InputDevice::physicalMidiDevice)
                            {
                                if (id == res)
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
                    else if (res >= 1)
                    {
                        int id = 1;
                        for (auto instance: at->edit.getAllInputDevices())
                        {
                            if (instance->getInputDevice().getDeviceType()
                                == te::InputDevice::waveDevice)
                            {
                                if (id == res)
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
            }
            else if (event.mods.isShiftDown())
            {
                if (editViewState.selectionManager.getNumObjectsSelected())
                {
                    editViewState.selectionManager.addToSelection(m_track);
                }
            }
            else
            {
                editViewState.selectionManager.selectOnly(m_track);
            }
        }
}

void TrackHeaderComponent::resized()
{
    auto area = getLocalBounds();
    auto peakDisplay = area.removeFromRight(15);
    peakDisplay.reduce(2, 2);
    levelMeterComp->setBounds (peakDisplay);
    auto volSlider = area.removeFromRight(area.getHeight());
    m_volumeKnob.setBounds(volSlider);

    auto buttonGroup = area.removeFromRight(area.getHeight());
    auto buttonwidth = buttonGroup.getWidth() / 2;
    auto buttonHeight = buttonGroup.getHeight() / 2;
    m_soloButton.setBounds(buttonGroup.getX(), buttonGroup.getY(), buttonwidth, buttonHeight);
    m_muteButton.setBounds(buttonGroup.getX(), buttonGroup.getY() + buttonHeight, buttonwidth, buttonHeight);
    m_armButton.setBounds(buttonGroup.getX() + buttonwidth, buttonGroup.getY(), buttonwidth, buttonHeight);

    area.removeFromLeft(20);
    m_trackName.setBounds(area);
}


//==============================================================================
TrackComponent::TrackComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    editViewState.state.addListener (this);
    track->state.addListener(this);
    track->edit.getTransport().addChangeListener (this);

    markAndUpdate (updateClips);
}

TrackComponent::~TrackComponent()
{
    track->state.removeListener (this);
    editViewState.state.removeListener(this);
    track->edit.getTransport().removeChangeListener (this);
}

void TrackComponent::paint (Graphics& g)
{
    g.fillAll ();
    g.setColour(Colour(0xff111111));
    g.drawRect(0,0, getWidth(), getHeight() );
    double x2 = editViewState.viewX2;
    double x1 = editViewState.viewX1;
    g.setColour(Colour(0xff333333));
    double zoom = x2 -x1;
    int firstBeat = static_cast<int>(x1);
    if(editViewState.beatsToX(firstBeat,getWidth()) < 0)
    {
        firstBeat++;
    }

    if (editViewState.selectionManager.isSelected (track.get()))
    {
        g.setColour (Colour(0xff111111));

        auto rc = getLocalBounds();
        if (editViewState.showHeaders) rc = rc.withTrimmedLeft (-4);
        if (editViewState.showFooters) rc = rc.withTrimmedRight (-4);

        g.fillRect (rc);
        g.setColour(Colour(0xff333333));
    }

    auto pixelPerBeat = getWidth() / zoom;
    //std::cout << zoom << std::endl;
    for (int beat = firstBeat - 1; beat <= editViewState.viewX2; beat++)
    {
        int BeatX = editViewState.beatsToX(beat, getWidth());

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
    auto firstLine = getLocalBounds ().removeFromLeft (1);
    g.setColour (Colours::white);
    g.fillRect (firstLine);
}

void TrackComponent::mouseDown (const MouseEvent&event)
{
    editViewState.selectionManager.selectOnly (track.get());
    if (event.mods.isRightButtonDown())
    {
        PopupMenu m;
        m.addItem(1, "Paste");


        const int result = m.show();

        if (result == 0)
        {
            // user dismissed the menu without picking anything
        }
        else if(result == 1)
        {
           auto ip = te::EditInsertPoint(editViewState.edit);
           ip.setNextInsertPoint(editViewState.beatToTime(editViewState.xToBeats(event.x, getWidth()))  ,track);
           te::Clipboard::getInstance()->getContentWithType<te::Clipboard::Clips>()->pasteIntoEdit(te::Clipboard::ContentType::EditPastingOptions (editViewState.edit, ip));
        }
    }

}

void TrackComponent::changeListenerCallback (ChangeBroadcaster*)
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
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            repaint();
            markAndUpdate (updatePositions);
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

void TrackComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::Clip::isClipState (c))
        markAndUpdate (updateClips);
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
        auto pos = c.getPosition();
        int x1 = editViewState.beatsToX (editViewState.edit.tempoSequence.timeToBeats(pos.getStart()), getWidth());
        int x2 = editViewState.beatsToX (editViewState.edit.tempoSequence.timeToBeats(pos.getEnd()), getWidth());

        cc->setBounds (x1, 0, x2 - x1, getHeight());
    }
}

void TrackComponent::itemDropped(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{

    auto dropPos = dragSourceDetails.localPosition;
    auto dropTime = editViewState.beatToTime(editViewState.xToBeats(dropPos.getX(), getWidth()));

    if (dragSourceDetails.description == "Clip")
        {
            auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
            if (clipComp)
            {
                clipComp->getClip().moveToTrack(*track);
            }
    }

    auto fileTreeComp = dynamic_cast<FileTreeComponent*>(dragSourceDetails.sourceComponent.get());
    if (fileTreeComp)
    {
        auto f = fileTreeComp->getSelectedFile();
        tracktion_engine::AudioFile audioFile(editViewState.edit.engine, f);
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
}

void TrackComponent::itemDragMove(const DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "Clip")
        {
            auto clipComp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get());
            if (clipComp)
            {
                addAndMakeVisible( clipComp);
            }
    }

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
                cc = new AudioClipComponent (editViewState, c);
            else if (dynamic_cast<te::MidiClip*> (c))
                cc = new MidiClipComponent (editViewState, c);
            else
                cc = new ClipComponent (editViewState, c);

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
        recordingClip = std::make_unique<RecordingClipComponent> (track, editViewState);
        addAndMakeVisible (*recordingClip);
    }
    else
    {
        recordingClip = nullptr;
    }
}




//==============================================================================
TrackFooterComponent::TrackFooterComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    addAndMakeVisible (addButton);

    buildPlugins();

    track->state.addListener (this);

    addButton.onClick = [this]
    {
        if (auto plugin = showMenuAndCreatePlugin (track->edit))
            track->pluginList.insertPlugin (plugin, 0, &editViewState.selectionManager);
        editViewState.selectionManager.selectOnly (track);
    };
}

TrackFooterComponent::~TrackFooterComponent()
{
    track->state.removeListener (this);
}

void TrackFooterComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void TrackFooterComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void TrackFooterComponent::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    markAndUpdate (updatePlugins);
}

void TrackFooterComponent::paint (Graphics& g)
{
    g.setColour (Colour(0x181818));
    g.fillRect (getLocalBounds().withTrimmedLeft (2));
}

void TrackFooterComponent::mouseDown (const MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void TrackFooterComponent::resized()
{
    auto area = getLocalBounds().reduced (5);

    addButton.setBounds (area.removeFromLeft(15));


    for (auto p : plugins)
    {
        area.removeFromLeft (5);
        p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));
    }
}

void TrackFooterComponent::handleAsyncUpdate()
{
    if (compareAndReset (updatePlugins))
        buildPlugins();
}

void TrackFooterComponent::buildPlugins()
{
    plugins.clear();

    for (auto plugin : track->pluginList)
    {
        //Hier zum richtigen Plugin die richtige Componente laden.
        if(plugin->getPluginType() == "volume")
        {
            auto volumePlugin = dynamic_cast<tracktion_engine::VolumeAndPanPlugin*>(plugin);
            //auto vp = new VolumePluginComponent( editViewState, volumePlugin);

        }
        auto p = new PluginComponent (editViewState, plugin);
        addAndMakeVisible (p);
        plugins.add (p);
    }
    resized();
}


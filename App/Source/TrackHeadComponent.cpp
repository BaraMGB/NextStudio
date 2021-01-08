#include "TrackHeadComponent.h"

TrackHeaderComponent::TrackHeaderComponent (EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs), m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_armButton,
                                         &m_muteButton,
                                         &m_soloButton
                                         });


    m_trackName.setText(m_track->getName(), juce::NotificationType::dontSendNotification);
    m_trackName.setJustificationType (juce::Justification::topLeft);
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
    removeAllChangeListeners ();
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
    bool isMidiTrack = m_track->state.getProperty (IDs::isMidiTrack);
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

    int id = 0;
    if (!isMidiTrack)
    {

        id = 1;
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
    }else
    {
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
        //toDO ... hack!
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
    te::Clipboard::getInstance()->clear();
    m_track->deselect();
    m_track->edit.deleteTrack(m_track);
    auto i = te::getAllTracks(m_editViewState.m_edit).getLast();

    if (!(i->isArrangerTrack()
        || i->isTempoTrack()
        || i->isMarkerTrack()
        || i->isChordTrack()))
    {
        m_editViewState.m_selectionManager.selectOnly(i);
    }
    else
    {
        m_editViewState.m_selectionManager.deselectAll();
    }
}

te::Track::Ptr TrackHeaderComponent::getTrack() const
{
    return m_track;
}

void TrackHeaderComponent::updateMidiInputs()
{
    if (auto at = dynamic_cast<te::AudioTrack*>(getTrack ().get ()))
    {
        if ( at->state.getProperty (IDs::isMidiTrack))
        {
            auto &dm = m_editViewState.m_edit.engine.getDeviceManager ();
            for (auto instance: m_editViewState.m_edit.getAllInputDevices())
            {
                if (auto midiIn = dynamic_cast<te::MidiInputDevice*>(&instance->getInputDevice ()))
                {

                    if (midiIn == dm.getDefaultMidiInDevice ())
                    {
                        instance->setTargetTrack(*at, 0, true);
                        m_editViewState.m_edit.restartPlayback();
                    }
                }
            }
            if (m_editViewState.m_isAutoArmed)
            {
                for (auto&i : m_editViewState.m_edit.getTrackList ())
                {
                    if (auto audioTrack = dynamic_cast<te::AudioTrack*>(i))
                    {
                        EngineHelpers::armTrack (*audioTrack,false);
                    }
                }
                EngineHelpers::armTrack (*at, true);
            }
        }
    }

}

void TrackHeaderComponent::paint (juce::Graphics& g)
{
    auto cornerSize = 10.0f;
    juce::Rectangle<float> area = getLocalBounds().toFloat();

    area.reduce(1, 1);

    auto buttonColour = juce::Colour(0xff4b4b4b);

    if (!m_editViewState.m_selectionManager.isSelected (m_track))
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

    GUIHelpers::drawFromSvg (g
                             , m_track->state.getProperty(IDs::isMidiTrack)
                                ? BinaryData::piano_svg
                                : BinaryData::waveform_svg
                             , "#ffffff"
                             , {20,5,20,20});

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

    area.removeFromLeft(45);
    area.removeFromTop (7);

    m_trackName.setBounds(area);
}

void TrackHeaderComponent::mouseDown (const juce::MouseEvent& event)
{
    m_trackHeightATMouseDown = getHeight ();
    m_yPosAtMouseDown = event.mouseDownPosition.y;
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
                if (m_editViewState.m_selectionManager.getNumObjectsSelected())
                {
                    m_editViewState.m_selectionManager.addToSelection(m_track);
                }
            }
            else if (event.mods.isLeftButtonDown ())
            {
                if (event.mods.isCtrlDown ())
                {
                    m_editViewState.m_selectionManager.addToSelection (m_track);
                }
                else
                {
                    m_editViewState.m_selectionManager.selectOnly(m_track);
                }

                updateMidiInputs ();
                if (event.getNumberOfClicks () > 1 || !m_editViewState.m_isPianoRollVisible)
                {
                    m_editViewState.m_isPianoRollVisible = false;
                    sendChangeMessage ();
                }
            }
    }
}



void TrackHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_yPosAtMouseDown > m_trackHeightATMouseDown - 10)
    {
        m_isResizing = true;
        auto newHeight = static_cast<int> (m_trackHeightATMouseDown
                                        + event.getDistanceFromDragStartY());

        m_track->state.setProperty(te::IDs::height
                                   , juce::jlimit(40, 250, newHeight)
                                   , &(m_editViewState.m_edit.getUndoManager()));
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


juce::Colour TrackHeaderComponent::getTrackColour()
{
    return m_track->getColour ();
}


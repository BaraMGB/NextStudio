#include "TrackHeadComponent.h"

TrackHeaderComponent::TrackHeaderComponent (EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs), m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_armButton,
                                         &m_muteButton,
                                         &m_soloButton
                                         });
    m_trackName.addListener (this);
    m_trackName.setText(m_track->getName(), juce::NotificationType::dontSendNotification);
    m_trackName.setJustificationType (juce::Justification::topLeft);
    m_trackName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);
    m_trackName.setEditable (false, false, true);

    if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        m_isAudioTrack = true;
        levelMeterComp = std::make_unique<LevelMeterComponent>(audioTrack->getLevelMeterPlugin()->measurer);
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

        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob.addListener (this);
            m_volumeKnob.setOpaque(false);
            addAndMakeVisible(m_volumeKnob);
            m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
            m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
            m_volumeKnob.setValue(audioTrack->getVolumePlugin()->volume);
            m_volumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
        }
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
    m_trackName.removeListener (this);
    m_track->state.removeListener (this);
}

void TrackHeaderComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::TrackList::isTrack (v))
    {
        if (i == te::IDs::mute)
        {
            m_muteButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        }
        else if (i == te::IDs::solo)
        {
            m_soloButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        }
        else if (i == te::IDs::height)
        {
            getParentComponent()->resized();
        }
        else if (i == te::IDs::volume)
        {
            if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
            {
                m_volumeKnob.setValue(audioTrack->getVolumePlugin()->volume);
            }
        }
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
    if (m_isDragging)
    {
        childrenSetVisible (false);
        g.setColour (juce::Colour(0xff2b2b2b));
        if (m_trackIsOver)
        {
            g.setColour (juce::Colour(0xff4b4b4b));
        }
        g.fillRect (getLocalBounds ());
    }
    else
    {
        childrenSetVisible (true);
        auto cornerSize = 5.0f;
        juce::Rectangle<float> area = getLocalBounds().toFloat();
        area.reduce(1, 1);
        auto buttonColour = juce::Colour(0xff4b4b4b);
        if (!m_editViewState.m_selectionManager.isSelected (m_track))
        {
            buttonColour = buttonColour.darker (0.4f);
        }
        g.setColour(buttonColour);
        GUIHelpers::drawRoundedRectWithSide(g,area,cornerSize,true);

        juce::Rectangle<float> trackColorIndicator = getLocalBounds().removeFromLeft(15).toFloat();
        auto trackColor =  m_track->getColour();
        g.setColour (trackColor);
        GUIHelpers::drawRoundedRectWithSide(g, trackColorIndicator.reduced(1,1), cornerSize, true);

        if (m_trackIsOver)
        {
            g.setColour(juce::Colour(0x66ffffff));
            g.drawRect(getLocalBounds().removeFromTop(1));
        }
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
                                 , {20, 3, 24, 24});
        if (m_contentIsOver)
        {
            g.setColour(juce::Colours::white);
            g.drawRect (getLocalBounds ());
        }
    }
}

void TrackHeaderComponent::resized()
{
    auto defaultTrackHeight = m_track->defaultTrackHeight;
    auto area = getLocalBounds().removeFromTop(defaultTrackHeight);//getLocalBounds();
    auto peakdisplay = getLocalBounds ().removeFromRight (15);
    peakdisplay.reduce (2,2);
    if (levelMeterComp)
        levelMeterComp->setBounds (peakdisplay);
    area.removeFromRight (peakdisplay.getWidth ());
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
                else if (event.getNumberOfClicks () > 1)
                {
                    m_trackName.showEditor ();
                }
                else
                {
                    m_editViewState.m_selectionManager.selectOnly(m_track);
                    m_dragImage = createComponentSnapshot (getLocalBounds ());
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
    if (event.mouseWasDraggedSinceMouseDown ())
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
        else
        {
            juce::DragAndDropContainer* dragC =
                    juce::DragAndDropContainer::findParentDragContainerFor(this);
            if (!dragC->isDragAndDropActive())
            {
                dragC->startDragging(
                            "Track"
                          , this
                          , m_dragImage);
            }
            m_isDragging = true;
        }
    }
}

void TrackHeaderComponent::mouseUp(const juce::MouseEvent &event)
{
    m_isResizing = false;
    m_isDragging = false;
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

bool TrackHeaderComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::createFromDescription("CTRL + D"))
    {
        auto trackContent = std::make_unique<te::Clipboard::Tracks>();
        trackContent->tracks.push_back (m_track->state);
        te::EditInsertPoint insertPoint(m_editViewState.m_edit);
        te::Clipboard::Tracks::EditPastingOptions options(m_editViewState.m_edit
                                                          ,insertPoint
                                                          , &m_editViewState.m_selectionManager);
        options.startTrack = m_track;
        trackContent->pasteIntoEdit (options);
        return true;

    }
    if (key == juce::KeyPress::deleteKey)
    {
        for (auto t : m_editViewState.m_selectionManager.getItemsOfType<te::Track>())
        {
            m_editViewState.m_edit.deleteTrack (t);
        }
        return true;
    }
    return false;
}


juce::Colour TrackHeaderComponent::getTrackColour()
{
    return m_track->getColour ();
}
bool TrackHeaderComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry"
     || dragSourceDetails.description == "Track")
    {
        return true;
    }
    return false;
}

void TrackHeaderComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        m_contentIsOver = true;
    }
    else if (dragSourceDetails.description == "Track")
    {
        m_trackIsOver = true;
    }
    repaint ();
}
void TrackHeaderComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
}
void TrackHeaderComponent::itemDropped(
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
    if (dragSourceDetails.description == "Track")
    {
        if (auto tc = dynamic_cast<TrackHeaderComponent*>(dragSourceDetails.sourceComponent.get ()))
        {

            m_editViewState.m_edit.moveTrack (
                        tc->getTrack ()
                        , { nullptr
                          , getTrack ()->getSiblingTrack (-1, false)});
        }
    }
    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
}

void TrackHeaderComponent::sliderValueChanged(juce::Slider *slider)
{
    if (slider == &m_volumeKnob)
    {
        if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
        {
            audioTrack->getVolumePlugin ()->volParam->setParameter (slider->getValue (), juce::NotificationType::dontSendNotification);
        }
    }
}

void TrackHeaderComponent::labelTextChanged(juce::Label *labelThatHasChanged)
{
    if (labelThatHasChanged == &m_trackName)
    {
        m_track->setName (labelThatHasChanged->getText ());
    }
}

void TrackHeaderComponent::childrenSetVisible(bool v)
{
    if (m_isAudioTrack)
    {
        m_armButton.setVisible (v);
        m_muteButton.setVisible (v);
        m_soloButton.setVisible (v);
        m_volumeKnob.setVisible (v);
        m_trackName.setVisible (v);
    }
}

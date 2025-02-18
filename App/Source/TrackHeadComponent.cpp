
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TrackHeadComponent.h"
#include "EditComponent.h"
#include "EditViewState.h"
#include "PluginBrowser.h"
#include "Utilities.h"
#include "InstrumentEffectChooser.h"
AutomationLaneHeaderComponent::AutomationLaneHeaderComponent(tracktion_engine::AutomatableParameter &ap , EditViewState& evs)
	: m_automatableParameter(ap)
        , m_evs (evs)
    , m_slider(ap)
{
    addAndMakeVisible(m_parameterName);
    addAndMakeVisible (m_pluginName);
    addAndMakeVisible (m_slider);
    juce::String pluginDescription = m_automatableParameter.getFullName ()
            .fromFirstOccurrenceOf (">>", false, false);
    juce::String parameterName = pluginDescription
            .fromFirstOccurrenceOf (">>", false, false);
    juce::String pluginName = pluginDescription
            .upToFirstOccurrenceOf (">>", false, false);

    m_pluginName.setText (pluginName, juce::dontSendNotification);
    m_pluginName.setJustificationType (juce::Justification::centredLeft);
    m_pluginName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_pluginName.setInterceptsMouseClicks(false, false);
    m_pluginName.setEditable (false, false, true);
    m_pluginName.setMinimumHorizontalScale (1);
    m_pluginName.setColour (juce::Label::backgroundColourId, juce::Colour(0xff333333));
    m_pluginName.setFont (juce::Font (12.0f, juce::Font::plain));
    m_parameterName.setFont (juce::Font (12.0f, juce::Font::plain));
    m_parameterName.setText (parameterName, juce::dontSendNotification);
    m_parameterName.setMinimumHorizontalScale (1);
    m_parameterName.setJustificationType(juce::Justification::centredLeft);
    m_parameterName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_parameterName.setColour (juce::Label::backgroundColourId, juce::Colour(0xff333333));
    m_parameterName.setInterceptsMouseClicks(false, false);
    m_parameterName.setEditable (false, false, true);
    m_slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    m_slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, false);

}

void AutomationLaneHeaderComponent::paint(juce::Graphics &g)
{
    if (m_automatableParameter.getTrack() == nullptr)
        return;

    g.setColour (juce::Colours::white);
    const int minimizedHeight = m_evs.m_trackHeightManager->getTrackMinimizedHeight();
    auto area = getLocalBounds().removeFromTop(minimizedHeight);
    area.removeFromLeft (10);
    GUIHelpers::drawFromSvg (g
                             , BinaryData::automation_svg
                             , juce::Colour(0xffffffff) 
                             , area.removeFromLeft (20).toFloat ());
    area.removeFromLeft (5);

    g.setColour(juce::Colours::black);


    int strokeHeight = 1;
    if (m_hovering)
    {
        g.setColour(juce::Colour(0x33ffffff));
        strokeHeight = 3;
    }
    if (m_resizing)
    {
        g.setColour(juce::Colour(0x55ffffff));
        strokeHeight = 3;
    }

    auto r = juce::Rectangle<int>(getLocalBounds ().removeFromBottom (strokeHeight));
    r.removeFromLeft (10);
    g.fillRect (r);
}

void AutomationLaneHeaderComponent::resized()
{
    const int gap = 3;
    const int minimizedHeigth = m_evs.m_trackHeightManager->getTrackMinimizedHeight();
    auto area = getLocalBounds().removeFromTop(minimizedHeigth);
    auto peakdisplay = area.removeFromRight (15);
    peakdisplay.reduce (gap, gap);
    auto volSlider = area.removeFromRight(area.getHeight ());
    m_slider.setBounds (volSlider);

    area.removeFromLeft (37);
    area.reduce (0, 6);

    m_parameterName.setBounds (area.removeFromRight (area.getWidth ()/2));
    area.removeFromRight (gap);
    m_pluginName.setBounds (area);
}

te::AutomatableParameter &AutomationLaneHeaderComponent::automatableParameter() const
{
    return m_automatableParameter;
}

void AutomationLaneHeaderComponent::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem(2000, "Delete automation");
        const int result = m.show();
        if (result == 2000)
        {
            m_automatableParameter.getCurve().clear();
            te::AutomationCurve::removeAllAutomationCurvesRecursively(m_automatableParameter.getCurve().parentState);
        }
    }
    else if (event.mods.isLeftButtonDown ())
    {
        m_mouseDownY = event.y;
        m_heightAtMouseDown = getHeight ();
        getParentComponent ()->mouseDown (event);
    }
}

void AutomationLaneHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (event.mouseWasDraggedSinceMouseDown ())
    {
        if (m_mouseDownY > m_heightAtMouseDown - 10)
        {
            m_resizing = true;
            auto newHeight = static_cast<int> (m_heightAtMouseDown + event.getDistanceFromDragStartY ());
            newHeight = juce::jlimit(30 , 250, newHeight);
            m_evs.m_trackHeightManager->setAutomationHeight(&m_automatableParameter, newHeight);
            getParentComponent()->getParentComponent()->resized();
        }
    }
}

void AutomationLaneHeaderComponent::mouseMove(const juce::MouseEvent &event)
{
    auto old = m_hovering;
    if (event.y > getHeight ()- 10)
    {
        m_hovering = true;
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        GUIHelpers::log("AutomationHeader move");
    }
    else
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        m_hovering = false;
    }

    if (m_hovering != old)
    {
        auto stripeRect = getLocalBounds();
        repaint (stripeRect.removeFromBottom(10));
    }
}

void AutomationLaneHeaderComponent::mouseExit(const juce::MouseEvent &/*event*/)
{
    m_resizing = false;
    m_hovering = false;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    auto stripeRect = getLocalBounds();
    repaint (stripeRect.removeFromBottom(10));
}

//------------------------------------------------------------------------------

TrackHeaderComponent::TrackHeaderComponent (
    EditViewState& evs, te::Track::Ptr t)
    : m_editViewState (evs)
    , m_track (t)
{
    Helpers::addAndMakeVisible (*this, { &m_trackName,
                                         &m_muteButton,
                                         &m_soloButton
                                         });
    m_trackName.addListener (this);
    m_trackName.setText(m_track->getName()
                        , juce::NotificationType::dontSendNotification);
    m_trackName.setJustificationType (juce::Justification::topLeft);
    m_trackName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);
    m_trackName.setEditable (false, false, true);

    if (auto folderTrack = dynamic_cast<te::FolderTrack*> (m_track.get()))
    {
		addAndMakeVisible(m_muteButton);
		addAndMakeVisible(m_soloButton);
        m_muteButton.onClick = [folderTrack]
        {
            folderTrack->setMute (! folderTrack->isMuted (false));
        };

        m_soloButton.onClick = [folderTrack]
        {
            folderTrack->setSolo (! folderTrack->isSolo (false));
        };

        if (folderTrack->getVolumePlugin())
        {
            m_volumeKnob = std::make_unique<AutomatableSliderComponent>(
                folderTrack->getVolumePlugin()
                    ->getAutomatableParameterByID("volume"));
            m_volumeKnob->setOpaque(false);
            addAndMakeVisible(m_volumeKnob.get());
            m_volumeKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
        }
    }

    if (auto audioTrack = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        m_isAudioTrack = true;

        if (audioTrack->getLevelMeterPlugin() != nullptr)
        {
            levelMeterComp = std::make_unique<LevelMeterComponent>(
                        audioTrack->getLevelMeterPlugin()->measurer);
            addAndMakeVisible(levelMeterComp.get());
        }

        addAndMakeVisible(m_armButton);

        m_armButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
            m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), juce::dontSendNotification);
        };

        m_muteButton.onClick = [audioTrack]
        {
            audioTrack->setMute (! audioTrack->isMuted (false));
        };

        m_soloButton.onClick = [audioTrack]
        {
            audioTrack->setSolo (! audioTrack->isSolo (false));
        };

        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack)
                                    , juce::dontSendNotification);

        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob = std::make_unique<AutomatableSliderComponent>(
                        audioTrack->getVolumePlugin()
                        ->getAutomatableParameterByID("volume"));
            m_volumeKnob->setOpaque(false);
            addAndMakeVisible(m_volumeKnob.get());
            m_volumeKnob->setRange(0.0f, 3.0f, 0.01f);
            m_volumeKnob->setSkewFactorFromMidPoint(1.0f);


            m_volumeKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);

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

    buildAutomationHeader();
}

TrackHeaderComponent::~TrackHeaderComponent()
{
    inputsState = m_track->edit.state.getChildWithName (te::IDs::INPUTDEVICES);
    inputsState.removeListener (this);
    m_track->state.removeListener (this);
    m_trackName.removeListener (this);
}

void TrackHeaderComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (te::TrackList::isTrack (v) || v.hasType (te::IDs::AUTOMATIONCURVE))
    {
        if (i == te::IDs::mute)
        {
            m_muteButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        }
        else if (i == te::IDs::solo)
        {
            m_soloButton.setToggleState ((bool)v[i], juce::dontSendNotification);
        }
    }
    if (v.hasType (te::IDs::INPUTDEVICES)
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

void TrackHeaderComponent::valueTreeChildAdded(juce::ValueTree& /*parentTree*/
                                               , juce::ValueTree& /*childWhichHasBeenAdded*/)
{
}

void TrackHeaderComponent::valueTreeChildRemoved(
    juce::ValueTree& /*parentTree*/
    , juce::ValueTree &/*childWhichHasBeenRemoved*/
    , int /*indexFromWhichChildWasRemoved*/)
{
}

void TrackHeaderComponent::showPopupMenu(tracktion_engine::Track *at)
{
    bool isMidiTrack = m_track->state.getProperty (IDs::isMidiTrack);
    at->edit.playInStopEnabled = true;
    juce::PopupMenu m;
    m.addItem(2000, "delete Track");
    m.addSeparator();

    if (auto aut = dynamic_cast<te::AudioTrack*>(m_track.get()))
    {
        if (EngineHelpers::trackHasInput(*aut))
        {
            bool ticked = EngineHelpers::isInputMonitoringEnabled(*aut);
            m.addItem(1000, "Input Monitoring", true, ticked);
            m.addSeparator();
        }
    }
    int id = 0;
    if (!isMidiTrack)
    {
        int id = 1;
        for (auto instance : at->edit.getAllInputDevices())
        {
            if (instance->getInputDevice().getDeviceType() == te::InputDevice::waveDevice)
            {
                bool ticked = instance->getTargets().contains(at->itemID);
                m.addItem (id++, instance->getInputDevice().getName(), true, ticked);
            }
        }
    m.addSeparator();
    }else
    {
        id = 100;
        for (auto instance: at->edit.getAllInputDevices())
        {
            if (instance->getInputDevice().getDeviceType() == te::InputDevice::physicalMidiDevice)
            {
                bool ticked = instance->getTargets().contains(at->itemID);
                m.addItem(id++, instance->getInputDevice().getName(), true, ticked);
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
        if (auto aut = dynamic_cast<te::AudioTrack*>(m_track.get()))
        {
            GUIHelpers::log("TrackHeadComponent.cpp : enable input");
            EngineHelpers::enableInputMonitoring(*aut, !EngineHelpers::isInputMonitoringEnabled(*aut));
        }
    }
    else if (result >= 100 && !m_track->isFolderTrack())
    {
        if (auto aut = dynamic_cast<te::AudioTrack*>(m_track.get()))
        {
            int id = 100;

            for (auto instance: at->edit.getAllInputDevices())
            {
                if (instance->getInputDevice().getDeviceType() == te::InputDevice::physicalMidiDevice)
                {
                    if (id == result)
                    {
                        if (instance->getTargets().contains(at->itemID))
                        {
                            [[ maybe_unused ]] auto result = instance->removeTarget(at->itemID, &at->edit.getUndoManager());
                        }
                        else
                        {
                            auto num = instance->getTargets().size();
                            [[ maybe_unused ]] auto result = instance->setTarget (at->itemID, true, &at->edit.getUndoManager(), num);
                        }
                    }
                    id++;
                }
            }
        }
    }
    else if (result >= 1 && !m_track->isFolderTrack())
    {
        if (auto aut = dynamic_cast<te::AudioTrack*>(m_track.get()))
        {
            int id = 1;
            for (auto instance : at->edit.getAllInputDevices())
            {
                if (instance->getInputDevice().getDeviceType() == te::InputDevice::waveDevice)
                {
                    if (id == result)
                    {
                        if (instance->getTargets().contains(at->itemID))
                        {
                            [[ maybe_unused ]] auto result = instance->removeTarget(at->itemID, &at->edit.getUndoManager());
                        }
                        else
                        {
                            auto num = instance->getTargets().size();
                            [[ maybe_unused ]] auto result = instance->setTarget (at->itemID, true, &at->edit.getUndoManager(), num);
                        }
                    }
                    id++;
                }
            }
        }

    }
}



void TrackHeaderComponent::deleteTrackFromEdit()
{
    m_editViewState.m_selectionManager.deselectAll();
    te::Clipboard::getInstance()->clear();
    m_track->edit.deleteTrack(m_track);
}

void TrackHeaderComponent::buildAutomationHeader()
{
    m_automationHeaders.clear(true);

    m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromStates(tracktion::getAllTracks(m_track->edit));

    auto* trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    for (const auto& [paramID, height] : trackInfo->automationParameterHeights)
    {
        GUIHelpers::log("TrackHeaderComponent: found Parameter: " + paramID);
        auto* parameter = m_editViewState.m_trackHeightManager->findAutomatableParameterByID(m_track, paramID);
        if (parameter && parameter->getCurve().getNumPoints() > 0)
        {
            GUIHelpers::log("TrackHeaderComponent: add Parameter: " + paramID);
            m_automationHeaders.add(new AutomationLaneHeaderComponent(*parameter, m_editViewState));
            addAndMakeVisible(m_automationHeaders.getLast());
        }
    }

    resized();
}

te::Track::Ptr TrackHeaderComponent::getTrack() const
{
    return m_track;
}

void TrackHeaderComponent::updateMidiInputs()
{
    EngineHelpers::updateMidiInputs(m_editViewState, getTrack());
}

void TrackHeaderComponent::paint (juce::Graphics& g)
{
    const int headWidth = 20;
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
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
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
        GUIHelpers::drawRoundedRectWithSide(g, area, cornerSize,true, false, true, false);

        juce::Rectangle<float> trackColorIndicator
                = getLocalBounds().removeFromLeft(headWidth).toFloat();
        auto trackColor =  m_track->getColour();
        g.setColour(trackColor);
        GUIHelpers::drawRoundedRectWithSide(
                    g, trackColorIndicator.reduced(1, 1), cornerSize, true, false, true, false);
        GUIHelpers::drawFromSvg (
                    g
                  , isMinimized
                        ? BinaryData::arrowright18_svg
                        : BinaryData::arrowdown18_svg
                  , juce::Colour(0xff000000)
                  , {1, 6, 18, 18});

        g.setColour (juce::Colours::black);
        if (isMinimized)
        {
            int strokeHeight = 1;
            if (m_isHover)
            {
                g.setColour(juce::Colour(0x33ffffff));
                strokeHeight = 3;
            }
            if (m_isResizing)
            {
                g.setColour(juce::Colour(0x55ffffff));
                strokeHeight = 3;
            }
            int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
            if (isFolderTrack())
                height = m_editViewState.m_folderTrackHeight;
            g.fillRect (juce::Rectangle<int>(
                headWidth - 1
                , height - strokeHeight
                , getWidth () - headWidth
                , strokeHeight));
        }

        if (m_trackIsOver)
        {
            g.setColour(juce::Colour(0x66ffffff));
            g.drawRect(getLocalBounds().removeFromTop(1));        
        }
        
        const char* icon = BinaryData::wavetest5_svg;;
        if (m_track->isFolderTrack())
            icon = BinaryData::folderopen_svg;
        if (m_track->state.getProperty(IDs::isMidiTrack))
            icon = BinaryData::piano5_svg;

        GUIHelpers::drawFromSvg (g, icon, juce::Colour(0xffffffff), {20, 6, 18, 18});

        if (m_contentIsOver)
        {
            g.setColour(juce::Colours::white);
            g.drawRect (getLocalBounds ());
        }
    }
}

void TrackHeaderComponent::resized()
{
    const int gap = 3;
    const int minimizedHeigth = 30;
    auto area = getLocalBounds().removeFromTop(minimizedHeigth);
    auto peakdisplay = area.removeFromRight (15);
    peakdisplay.reduce (gap, gap);
    if (levelMeterComp)
        levelMeterComp->setBounds (peakdisplay);
    auto volSlider = area.removeFromRight(area.getHeight ());
    if (m_volumeKnob)
        m_volumeKnob->setBounds(volSlider);

    auto gapX = 1, gapY = 7;
    auto buttonwidth = minimizedHeigth - 10;
    auto solo = area.removeFromRight (buttonwidth).reduced(gapX, gapY);
    m_soloButton.setBounds(solo);
    m_soloButton.setComponentID ("solo");
    m_soloButton.setTooltip ("solo track");
    m_soloButton.setName ("S");
    //m_soloButton.setMouseClickGrabsKeyboardFocus (false);
    m_soloButton.setWantsKeyboardFocus (false);

    auto mute = area.removeFromRight (buttonwidth).reduced(gapX, gapY);
    m_muteButton.setBounds(mute);
    m_muteButton.setComponentID ("mute");
    m_muteButton.setTooltip ("mute track");
    m_muteButton.setName ("M");
    m_muteButton.setWantsKeyboardFocus (false);
    //m_muteButton.setMouseClickGrabsKeyboardFocus (false);

    auto arm = area.removeFromRight (buttonwidth).reduced(gapX, gapY);
    m_armButton.setBounds(arm);
    m_armButton.setTooltip ("arm track");
    m_armButton.setComponentID ("arm");
    m_armButton.setName ("A");
    m_armButton.setWantsKeyboardFocus (false);
    //m_armButton.setMouseClickGrabsKeyboardFocus (false);

    area.removeFromLeft(45);
    area.removeFromTop (6);
    m_trackName.setBounds(area);

    // AutomationLanes
    auto* trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);

    auto rect = getLocalBounds();
    rect.removeFromLeft(peakdisplay.getWidth());
    rect.removeFromTop(height);


    for (auto* ahs : m_automationHeaders)
    {
        auto paramID = ahs->automatableParameter().paramID;
        int automationHeight = trackInfo->automationParameterHeights[paramID];
        GUIHelpers::log("AUTOMATIONHIGHT: ", automationHeight);
        automationHeight = automationHeight < minimizedHeigth ? minimizedHeigth : automationHeight;
        ahs->setBounds(rect.removeFromBottom(automationHeight));
    }

}

void TrackHeaderComponent::mouseDown (const juce::MouseEvent& event)
{
    // m_trackHeightATMouseDown = m_track->state.getProperty
    //         (te::IDs::height, (int) m_editViewState.m_trackHeightMinimized);
    m_trackHeightATMouseDown = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);

    m_yPosAtMouseDown = event.y;
    auto area = getLocalBounds ().removeFromLeft (20);
    if (area.contains (event.getPosition ()))
    {
         // m_track->state.setProperty (IDs::isTrackMinimized,
         //         !(m_track->state.getProperty (IDs::isTrackMinimized)), nullptr);
        bool isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        m_editViewState.m_trackHeightManager->setMinimized(m_track, !isMinimized);
    }
    if (!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown ())
            {
                if (m_track->isAudioTrack() || m_track->isFolderTrack())
                {
                    showPopupMenu(m_track);
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
                    // m_trackName.showEditor ();
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
                    for (auto t : te::getAllTracks(m_editViewState.m_edit))
                    {
                        t->state.setProperty(IDs::showLowerRange, false, nullptr);
                        if (t == m_track.get())
                        { 
                            t->state.setProperty(IDs::showLowerRange, true, nullptr);
                        }
                    }
                }
            }
    }
}

void TrackHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (event.mouseWasDraggedSinceMouseDown ())
    {
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        if (m_yPosAtMouseDown > m_trackHeightATMouseDown - 10 && !isMinimized && !isFolderTrack())
        {
            m_isResizing = true;
            auto newHeight = static_cast<int> (m_trackHeightATMouseDown
                                               + event.getDistanceFromDragStartY());

            auto& trackHeightManager = m_editViewState.m_trackHeightManager;
            trackHeightManager->setTrackHeight(m_track, newHeight);
            trackHeightManager->setMinimized(m_track, false);
            getParentComponent()->resized();
            // m_track->state.setProperty(te::IDs::height
            //                            , juce::jlimit(30, 300, newHeight)
            //                            , &(m_editViewState.m_edit.getUndoManager()));
            // m_track->state.setProperty (IDs::isTrackMinimized
            //                           , false
            //                           , &m_editViewState.m_edit.getUndoManager ());
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

void TrackHeaderComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    m_isResizing = false;
    m_isDragging = false;
    // repaint();
}

void TrackHeaderComponent::mouseMove(const juce::MouseEvent& e)
{
    if (! m_isResizing)
    {
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        auto old = m_isHover;
        // int height = m_track->state.getProperty (te::IDs::height, getHeight ());
        int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        if (e.y > height - 10 && !isMinimized && !isFolderTrack())
        {
            m_isHover = true;
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
            GUIHelpers::log("TrackHeader move");
        }
        else
        {
            m_isHover = false;
            setMouseCursor (juce::MouseCursor::NormalCursor);
        }

        if (m_isHover != old)
        {
            auto stripeRect = getLocalBounds();
            stripeRect.removeFromBottom(getHeight() - height);
            repaint(stripeRect.removeFromBottom(10));
        }
    }
}

void TrackHeaderComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    m_isHover = false;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    // int height = m_track->state.getProperty (te::IDs::height, getHeight ());
    int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
    auto stripeRect = getLocalBounds();
    stripeRect.removeFromBottom(getHeight() - height);
    repaint(stripeRect.removeFromBottom(10));
}

juce::Colour TrackHeaderComponent::getTrackColour()
{
    return m_track->getColour ();
}
bool TrackHeaderComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry"
     || dragSourceDetails.description == "Track"
     || dragSourceDetails.description == "Instrument or Effect")
    {
        return true;
    }
    return false;
}

void TrackHeaderComponent::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
        m_contentIsOver = true;
    else if (dragSourceDetails.description == "Track")
        m_trackIsOver = true;

    repaint ();
}
void TrackHeaderComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails& /*dragSourceDetails*/)
{
    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
}
void TrackHeaderComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& details)
{
    
    if  (details.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<PluginListbox*>(details.sourceComponent.get ()))
        {
            EngineHelpers::insertPlugin (getTrack(), listbox->getSelectedPlugin(m_editViewState.m_edit));
        }
    }

    if (details.description == "Instrument or Effect")
    {
        if (auto lb = dynamic_cast<InstrumentEffectTable*>(details.sourceComponent.get()))
        {
            EngineHelpers::insertPlugin (getTrack(), lb->getSelectedPlugin(m_editViewState.m_edit));
        }
    }

    auto tc = dynamic_cast<TrackHeaderComponent*>(details.sourceComponent.get ());
    auto isTrack = details.description == "Track";
    auto tip = te::TrackInsertPoint(*getTrack (), true);

    if (tc && isTrack)
    {
        if (isFolderTrack())
            tip = te::TrackInsertPoint(m_track, m_track->getAllSubTracks(false).getLast());

        m_editViewState.m_edit.moveTrack (tc->getTrack (), tip);
    }


    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
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
        if (m_volumeKnob)
            m_volumeKnob->setVisible (v);
        m_trackName.setVisible (v);
    }
}

void TrackHeaderComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_updateAutomationLanes))
    {
        buildAutomationHeader();
    }
    if (compareAndReset (m_updateTrackHeight))
    {
        getParentComponent ()->resized ();
    }
}
void TrackHeaderComponent::collapseTrack(bool minimize)
{
    // m_track->state.setProperty(IDs::isTrackMinimized, minimize, &m_editViewState.m_edit.getUndoManager());
    m_editViewState.m_trackHeightManager->setMinimized(m_track, minimize);
}


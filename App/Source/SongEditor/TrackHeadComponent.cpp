
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "SongEditor/TrackHeadComponent.h"
#include "SideBrowser/InstrumentEffectChooser.h"
#include "SideBrowser/PluginBrowser.h"
#include "SongEditor/EditComponent.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"
#include "juce_graphics/juce_graphics.h"

namespace
{
bool isMasterAutomationParameter(const te::AutomatableParameter::Ptr &ap)
{
    if (ap == nullptr)
        return false;

    if (auto *track = ap->getTrack())
        return track->isMasterTrack();

    auto &edit = ap->getEdit();
    if (ap == edit.getMasterSliderPosParameter() || ap == edit.getMasterPanParameter())
        return true;

    if (auto *masterTrack = edit.getMasterTrack())
    {
        for (auto *masterParam : masterTrack->getAllAutomatableParams())
        {
            if (masterParam == ap.get())
                return true;
        }
    }

    return false;
}
} // namespace

AutomationLaneHeaderComponent::AutomationLaneHeaderComponent(tracktion_engine::AutomatableParameter::Ptr ap, EditViewState &evs)
    : m_automatableParameter(ap),
      m_evs(evs),
      m_slider(ap)
{
    addAndMakeVisible(m_parameterName);
    addAndMakeVisible(m_pluginName);
    addAndMakeVisible(m_slider);

    juce::String pluginName;
    juce::String parameterName = m_automatableParameter->getParameterName();

    if (auto *plugin = m_automatableParameter->getPlugin())
    {
        pluginName = plugin->getName();
    }
    else
    {
        auto pluginAndParam = m_automatableParameter->getPluginAndParamName();
        if (pluginAndParam.contains(">>"))
        {
            pluginName = pluginAndParam.upToFirstOccurrenceOf(">>", false, false).trim();
            parameterName = pluginAndParam.fromFirstOccurrenceOf(">>", false, false).trim();
        }
        else if (isMasterAutomationParameter(m_automatableParameter))
        {
            pluginName = "Master";
            if (m_automatableParameter == m_automatableParameter->getEdit().getMasterSliderPosParameter())
                parameterName = "Volume";
            else if (m_automatableParameter == m_automatableParameter->getEdit().getMasterPanParameter())
                parameterName = "Pan";
            else if (parameterName.isEmpty())
                parameterName = pluginAndParam;
        }

        if (pluginName.isEmpty())
            pluginName = "Automation";
    }

    m_pluginName.setText(pluginName, juce::dontSendNotification);
    m_pluginName.setJustificationType(juce::Justification::centredLeft);
    m_pluginName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_pluginName.setInterceptsMouseClicks(false, false);
    m_pluginName.setEditable(false, false, true);
    m_pluginName.setMinimumHorizontalScale(1);
    m_pluginName.setColour(juce::Label::backgroundColourId, juce::Colour(0xff333333));
    m_pluginName.setFont(juce::Font(12.0f, juce::Font::plain));
    m_parameterName.setFont(juce::Font(12.0f, juce::Font::plain));
    m_parameterName.setText(parameterName, juce::dontSendNotification);
    m_parameterName.setMinimumHorizontalScale(1);
    m_parameterName.setJustificationType(juce::Justification::centredLeft);
    m_parameterName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_parameterName.setColour(juce::Label::backgroundColourId, juce::Colour(0xff333333));
    m_parameterName.setInterceptsMouseClicks(false, false);
    m_parameterName.setEditable(false, false, true);
    m_slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
}

void AutomationLaneHeaderComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::white);
    const int minimizedHeight = m_evs.m_trackHeightManager->getTrackMinimizedHeight();
    auto area = getLocalBounds().removeFromTop(minimizedHeight);
    area.removeFromLeft(10);
    GUIHelpers::drawFromSvg(g, BinaryData::automation_svg, juce::Colour(0xffffffff), area.removeFromLeft(20).toFloat());
    area.removeFromLeft(5);

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

    const auto resizeFromTop = isMasterAutomationParameter(m_automatableParameter);
    auto r = juce::Rectangle<int>(resizeFromTop ? getLocalBounds().removeFromTop(strokeHeight) : getLocalBounds().removeFromBottom(strokeHeight));
    r.removeFromLeft(10);
    g.fillRect(r);
}

void AutomationLaneHeaderComponent::resized()
{
    const int gap = 3;
    const int minimizedHeigth = m_evs.m_trackHeightManager->getTrackMinimizedHeight();
    auto area = getLocalBounds().removeFromTop(minimizedHeigth);
    auto peakdisplay = area.removeFromRight(15);
    peakdisplay.reduce(gap, gap);
    auto volSlider = area.removeFromRight(area.getHeight());
    m_slider.setBounds(volSlider);

    area.removeFromLeft(37);
    area.reduce(0, 6);

    m_parameterName.setBounds(area.removeFromRight(area.getWidth() / 2));
    area.removeFromRight(gap);
    m_pluginName.setBounds(area);
}

te::AutomatableParameter::Ptr AutomationLaneHeaderComponent::automatableParameter() const { return m_automatableParameter; }

void AutomationLaneHeaderComponent::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem(2000, "Delete automation");
        const int result = m.show();
        if (result == 2000)
        {
            m_automatableParameter->getCurve().clear();
            te::AutomationCurve::removeAllAutomationCurvesRecursively(m_automatableParameter->getCurve().parentState);
        }
    }
    else if (event.mods.isLeftButtonDown())
    {
        m_mouseDownY = event.y;
        m_heightAtMouseDown = getHeight();
        getParentComponent()->mouseDown(event);
    }
}

void AutomationLaneHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (event.mouseWasDraggedSinceMouseDown())
    {
        const auto resizeFromTop = isMasterAutomationParameter(m_automatableParameter);
        const auto resizeHandleHit = resizeFromTop ? (m_mouseDownY < 10) : (m_mouseDownY > m_heightAtMouseDown - 10);

        if (resizeHandleHit)
        {
            m_resizing = true;
            auto newHeight = static_cast<int>(resizeFromTop ? (m_heightAtMouseDown - event.getDistanceFromDragStartY()) : (m_heightAtMouseDown + event.getDistanceFromDragStartY()));
            newHeight = juce::jlimit(30, 250, newHeight);
            m_evs.m_trackHeightManager->setAutomationHeight(m_automatableParameter, newHeight);

            if (auto *editComponent = findParentComponentOfClass<EditComponent>())
                editComponent->resized();
            else if (auto *parent = getParentComponent())
                parent->resized();
        }
    }
}

void AutomationLaneHeaderComponent::mouseMove(const juce::MouseEvent &event)
{
    auto old = m_hovering;
    const auto resizeFromTop = isMasterAutomationParameter(m_automatableParameter);

    if (resizeFromTop ? (event.y < 10) : (event.y > getHeight() - 10))
    {
        m_hovering = true;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        m_hovering = false;
    }

    if (m_hovering != old)
    {
        auto stripeRect = getLocalBounds();
        repaint(resizeFromTop ? stripeRect.removeFromTop(10) : stripeRect.removeFromBottom(10));
    }
}

void AutomationLaneHeaderComponent::mouseExit(const juce::MouseEvent & /*event*/)
{
    m_resizing = false;
    m_hovering = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    const auto resizeFromTop = isMasterAutomationParameter(m_automatableParameter);
    auto stripeRect = getLocalBounds();
    repaint(resizeFromTop ? stripeRect.removeFromTop(10) : stripeRect.removeFromBottom(10));
}

//------------------------------------------------------------------------------

TrackHeaderComponent::TrackHeaderComponent(EditViewState &evs, te::Track::Ptr t)
    : m_editViewState(evs),
      m_track(t)
{
    Helpers::addAndMakeVisible(*this, {&m_trackName, &m_muteButton, &m_soloButton});
    m_trackName.addListener(this);
    m_trackName.setText(m_track->getName(), juce::NotificationType::dontSendNotification);
    m_trackName.setJustificationType(juce::Justification::topLeft);
    m_trackName.setColour(juce::Label::textColourId, juce::Colours::white);
    m_trackName.setInterceptsMouseClicks(false, false);
    m_trackName.setEditable(false, false, true);

    if (auto folderTrack = dynamic_cast<te::FolderTrack *>(m_track.get()))
    {
        addAndMakeVisible(m_muteButton);
        addAndMakeVisible(m_soloButton);
        m_muteButton.onClick = [folderTrack] { folderTrack->setMute(!folderTrack->isMuted(false)); };

        m_soloButton.onClick = [folderTrack] { folderTrack->setSolo(!folderTrack->isSolo(false)); };

        if (folderTrack->getVolumePlugin())
        {
            m_volumeKnob = std::make_unique<AutomatableSliderComponent>(folderTrack->getVolumePlugin()->getAutomatableParameterByID("volume"));
            m_volumeKnob->setOpaque(false);
            addAndMakeVisible(m_volumeKnob.get());
            m_volumeKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
        }
    }

    if (auto audioTrack = dynamic_cast<te::AudioTrack *>(m_track.get()))
    {
        m_isAudioTrack = true;

        if (audioTrack->getLevelMeterPlugin() != nullptr)
        {
            levelMeterComp = std::make_unique<LevelMeterComponent>(audioTrack->getLevelMeterPlugin()->measurer);
            addAndMakeVisible(levelMeterComp.get());
        }

        addAndMakeVisible(m_armButton);

        m_armButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack(*audioTrack, !EngineHelpers::isTrackArmed(*audioTrack));
            m_armButton.setToggleState(EngineHelpers::isTrackArmed(*audioTrack), juce::dontSendNotification);
        };

        m_muteButton.onClick = [audioTrack] { audioTrack->setMute(!audioTrack->isMuted(false)); };

        m_soloButton.onClick = [audioTrack] { audioTrack->setSolo(!audioTrack->isSolo(false)); };

        m_armButton.setToggleState(EngineHelpers::isTrackArmed(*audioTrack), juce::dontSendNotification);

        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob = std::make_unique<AutomatableSliderComponent>(audioTrack->getVolumePlugin()->getAutomatableParameterByID("volume"));
            m_volumeKnob->setOpaque(false);
            addAndMakeVisible(m_volumeKnob.get());
            m_volumeKnob->setRange(0.0f, 3.0f, 0.01f);
            m_volumeKnob->setSkewFactorFromMidPoint(1.0f);

            m_volumeKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
        }
    }

    if (m_track->isMasterTrack())
    {
        levelMeterComp = std::make_unique<LevelMeterComponent>(
            [this]() -> te::LevelMeasurer *
            {
                if (auto epc = m_track->edit.getTransport().getCurrentPlaybackContext())
                    return &epc->masterLevels;

                return nullptr;
            });
        addAndMakeVisible(levelMeterComp.get());

        m_soloButton.setVisible(false);
        m_soloButton.setEnabled(false);
        m_armButton.setVisible(false);
        m_armButton.setEnabled(false);

        m_trackName.setText("Master", juce::dontSendNotification);

        if (auto masterVol = m_track->edit.getMasterVolumePlugin())
        {
            m_muteButton.onClick = [this]
            {
                if (auto currentMasterVol = m_track->edit.getMasterVolumePlugin())
                {
                    currentMasterVol->muteOrUnmute();
                    m_muteButton.setToggleState(currentMasterVol->getSliderPos() <= 0.0f, juce::dontSendNotification);
                }
            };

            m_muteButton.setToggleState(masterVol->getSliderPos() <= 0.0f, juce::dontSendNotification);

            m_volumeKnob = std::make_unique<AutomatableSliderComponent>(m_track->edit.getMasterSliderPosParameter());
            m_volumeKnob->setOpaque(false);
            addAndMakeVisible(m_volumeKnob.get());
            m_volumeKnob->setRange(0.0f, 3.0f, 0.01f);
            m_volumeKnob->setSkewFactorFromMidPoint(1.0f);
            m_volumeKnob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            m_volumeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
            GUIHelpers::log("Master header: volume knob created");
        }
    }

    m_track->state.addListener(this);
    inputsState = m_track->edit.state.getChildWithName(te::IDs::INPUTDEVICES);
    inputsState.addListener(this);

    valueTreePropertyChanged(m_track->state, te::IDs::mute);
    valueTreePropertyChanged(m_track->state, te::IDs::solo);
    valueTreePropertyChanged(inputsState, te::IDs::targetIndex);

    buildAutomationHeader();
}

TrackHeaderComponent::~TrackHeaderComponent()
{
    inputsState = m_track->edit.state.getChildWithName(te::IDs::INPUTDEVICES);
    inputsState.removeListener(this);
    m_track->state.removeListener(this);
    m_trackName.removeListener(this);
}

void TrackHeaderComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (te::TrackList::isTrack(v) || v.hasType(te::IDs::AUTOMATIONCURVE))
    {
        if (i == te::IDs::mute)
        {
            m_muteButton.setToggleState((bool)v[i], juce::dontSendNotification);
        }
        else if (i == te::IDs::solo)
        {
            m_soloButton.setToggleState((bool)v[i], juce::dontSendNotification);
        }
    }
    if (v.hasType(te::IDs::INPUTDEVICES) || v.hasType(te::IDs::INPUTDEVICE) || v.hasType(te::IDs::INPUTDEVICEDESTINATION))
    {
        if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            // m_armButton.setEnabled (EngineHelpers::trackHasInput (*at));
            m_armButton.setToggleState(EngineHelpers::isTrackArmed(*at), juce::dontSendNotification);
        }
    }
}

void TrackHeaderComponent::valueTreeChildAdded(juce::ValueTree & /*parentTree*/
                                               ,
                                               juce::ValueTree & /*childWhichHasBeenAdded*/)
{
}

void TrackHeaderComponent::valueTreeChildRemoved(juce::ValueTree & /*parentTree*/
                                                 ,
                                                 juce::ValueTree & /*childWhichHasBeenRemoved*/
                                                 ,
                                                 int /*indexFromWhichChildWasRemoved*/)
{
}

void TrackHeaderComponent::showPopupMenu(tracktion_engine::Track *at)
{
    bool isMidiTrack = m_track->state.getProperty(IDs::isMidiTrack);
    at->edit.playInStopEnabled = true;
    juce::PopupMenu m;

    auto pencilIcon = std::unique_ptr<juce::Drawable>(juce::Drawable::createFromImageData(BinaryData::pencil_svg, BinaryData::pencil_svgSize));
    auto deleteIcon = std::unique_ptr<juce::Drawable>(juce::Drawable::createFromImageData(BinaryData::delete_icon_svg, BinaryData::delete_icon_svgSize));

    auto textColour = m_editViewState.m_applicationState.getTextColour();
    auto iconSourceColour = juce::Colour(0xff626262);

    if (pencilIcon)
        pencilIcon->replaceColour(iconSourceColour, textColour);
    if (deleteIcon)
        deleteIcon->replaceColour(iconSourceColour, textColour);

    m.addItem(2001, "Rename Track", true, false, std::move(pencilIcon));
    m.addItem(2000, "Delete Track", true, false, std::move(deleteIcon));
    m.addSeparator();

    if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
    {
        if (EngineHelpers::trackHasInput(*aut))
        {
            bool ticked = EngineHelpers::isInputMonitoringEnabled(*aut);
            m.addItem(1000, "Input Monitoring", true, ticked);
            m.addSeparator();
        }
    }

    juce::PopupMenu inputMenu;
    int id = 0;

    if (!isMidiTrack)
    {
        int id = 1;
        auto &dm = at->edit.engine.getDeviceManager();

        // Show all available wave input devices from DeviceManager
        for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        {
            if (auto waveDevice = dm.getWaveInDevice(i))
            {
                // Check if this device already has an instance targeting this track
                bool ticked = false;
                bool isEnabled = true; // Start with enabled

                for (auto instance : at->edit.getAllInputDevices())
                {
                    if (&instance->getInputDevice() == waveDevice)
                    {
                        ticked = instance->getTargets().contains(at->itemID);
                        // Audio inputs are exclusive - disabled if they have other targets
                        isEnabled = instance->getTargets().size() == 0 || ticked;
                        break;
                    }
                }

                inputMenu.addItem(id++, waveDevice->getName(), isEnabled, ticked);
            }
        }
        m.addSubMenu("Audio Input", inputMenu);
        m.addSeparator();
    }
    else
    {
        id = 100;
        for (auto instance : at->edit.getAllInputDevices())
        {
            if (instance->getInputDevice().getDeviceType() == te::InputDevice::physicalMidiDevice)
            {
                bool ticked = instance->getTargets().contains(at->itemID);
                bool isEnabled = true; // MIDI inputs can have multiple targets - always enabled
                inputMenu.addItem(id++, instance->getInputDevice().getName(), isEnabled, ticked);
            }
        }
        m.addSubMenu("MIDI Input", inputMenu);

        // MIDI Output Selection
        if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            juce::PopupMenu outputMenu;

            int outputId = 500;
            auto &dm = at->edit.engine.getDeviceManager();
            auto currentOutputID = aut->getOutput().getOutputDeviceID();

            bool isDefault = aut->getOutput().usesDefaultMIDIOut();
            outputMenu.addItem(outputId++, "Default MIDI Output", true, isDefault);

            for (int i = 0; i < dm.getNumMidiOutDevices(); i++)
            {
                if (auto midiDevice = dm.getMidiOutDevice(i))
                {
                    if (midiDevice->isEnabled())
                    {
                        bool ticked = !isDefault && (midiDevice->getDeviceID() == currentOutputID);
                        outputMenu.addItem(outputId++, midiDevice->getName(), true, ticked);
                    }
                }
            }
            m.addSubMenu("MIDI Output", outputMenu);
        }
    }

    auto colours = m_editViewState.m_applicationState.m_trackColours;
    auto colourGridComponent = std::make_unique<ColourGridComponent>(m_track, colours, m_track->getColour(), 5, 4);

    m.addCustomItem(3000, std::move(colourGridComponent), nullptr, "Track Colours");

    const int result = m.show();
    if (result == 2000)
    {
        deleteTrackFromEdit();
    }
    else if (result == 2001)
    {
        m_trackName.showEditor();
    }
    else if (result == 1000)
    {
        if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            GUIHelpers::log("TrackHeadComponent.cpp : enable input");
            bool ticked = EngineHelpers::isInputMonitoringEnabled(*aut);
            EngineHelpers::enableInputMonitoring(*aut, !ticked);
        }
    }
    else if (result == 3000)
    {
        if (m_volumeKnob)
            m_volumeKnob->setTrackColour(m_track->getColour());

        repaint();
    }
    else if (result >= 500 && !m_track->isFolderTrack())
    {
        if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            auto at = m_track.get();
            int outputId = 500;
            if (result == outputId++)
            {
                aut->getOutput().setOutputToDefaultDevice(true);
            }
            else
            {
                auto &dm = at->edit.engine.getDeviceManager();
                for (int i = 0; i < dm.getNumMidiOutDevices(); i++)
                {
                    if (auto midiDevice = dm.getMidiOutDevice(i))
                    {
                        if (midiDevice->isEnabled())
                        {
                            if (result == outputId)
                            {
                                aut->getOutput().setOutputToDeviceID(midiDevice->getDeviceID());
                                break;
                            }
                            outputId++;
                        }
                    }
                }
            }
        }
    }
    else if (result >= 100 && !m_track->isFolderTrack())
    {
        if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            int id = 100;

            for (auto instance : at->edit.getAllInputDevices())
            {
                if (instance->getInputDevice().getDeviceType() == te::InputDevice::physicalMidiDevice)
                {
                    if (id == result)
                    {
                        if (instance->getTargets().contains(at->itemID))
                        {
                            [[maybe_unused]] auto result = instance->removeTarget(at->itemID, &at->edit.getUndoManager());
                        }
                        else
                        {
                            // MIDI inputs can have multiple targets - don't remove existing ones
                            [[maybe_unused]] auto result = instance->setTarget(at->itemID, false, &at->edit.getUndoManager(), 0);
                        }

                        // Restart playback to apply MIDI input changes
                        at->edit.getTransport().ensureContextAllocated();
                        at->edit.restartPlayback();
                    }
                    id++;
                }
            }
        }
    }
    else if (result >= 1 && !m_track->isFolderTrack())
    {
        if (auto aut = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            int id = 1;
            auto &dm = at->edit.engine.getDeviceManager();

            // Handle selection from all available wave input devices
            for (int i = 0; i < dm.getNumWaveInDevices(); i++)
            {
                if (auto waveDevice = dm.getWaveInDevice(i))
                {
                    if (id == result)
                    {
                        // Find existing instance for this device, if any
                        te::InputDeviceInstance *targetInstance = nullptr;
                        for (auto instance : at->edit.getAllInputDevices())
                        {
                            if (&instance->getInputDevice() == waveDevice)
                            {
                                targetInstance = instance;
                                break;
                            }
                        }

                        if (targetInstance)
                        {
                            // Instance exists, toggle its target
                            if (targetInstance->getTargets().contains(at->itemID))
                            {
                                [[maybe_unused]] auto result = targetInstance->removeTarget(at->itemID, &at->edit.getUndoManager());
                            }
                            else
                            {
                                for (auto currentTargetID : targetInstance->getTargets())
                                {
                                    targetInstance->removeTarget(currentTargetID, &at->edit.getUndoManager());
                                }
                                [[maybe_unused]] auto result = targetInstance->setTarget(at->itemID, false, &at->edit.getUndoManager(), 0);
                            }
                        }
                        else
                        {
                            // No instance exists, create one by enabling the device and setting target
                            waveDevice->setEnabled(true);
                            waveDevice->setMonitorMode(te::InputDevice::MonitorMode::automatic);

                            // The engine should create an instance when we restart playback
                            at->edit.getTransport().ensureContextAllocated();
                            at->edit.restartPlayback();

                            // Now find the newly created instance and set target
                            for (auto instance : at->edit.getAllInputDevices())
                            {
                                if (&instance->getInputDevice() == waveDevice)
                                {
                                    [[maybe_unused]] auto result = instance->setTarget(at->itemID, false, &at->edit.getUndoManager(), 0);
                                    break;
                                }
                            }
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

    m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromEdit(m_track->edit);

    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    juce::Array<te::AutomatableParameter *> params;
    for (const auto &[ap, height] : trackInfo->automationParameterHeights)
        if (ap && ap->getCurve().getNumPoints() > 0 && m_editViewState.m_trackHeightManager->isAutomationVisible(*ap))
            params.add(ap);

    // Sort the parameters by their ID string to ensure a consistent order
    std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

    for (auto *ap : params)
    {
        m_automationHeaders.add(new AutomationLaneHeaderComponent(ap, m_editViewState));
        addAndMakeVisible(m_automationHeaders.getLast());
    }

    resized();
}

te::Track::Ptr TrackHeaderComponent::getTrack() const { return m_track; }

void TrackHeaderComponent::paint(juce::Graphics &g)
{
    m_trackName.setColour(juce::Label::ColourIds::textColourId, m_editViewState.m_applicationState.getTrackHeaderTextColour());
    const int headWidth = 20;
    if (m_isDragging)
    {
        childrenSetVisible(false);
        g.setColour(juce::Colour(0xff2b2b2b));
        if (m_trackIsOver)
        {
            g.setColour(juce::Colour(0xff4b4b4b));
        }
        g.fillRect(getLocalBounds());
    }
    else
    {
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        childrenSetVisible(true);
        auto cornerSize = 7.0f;
        juce::Rectangle<float> area = getLocalBounds().toFloat();
        area.reduce(2, 1);
        auto buttonColour = m_editViewState.m_applicationState.getTrackHeaderBackgroundColour();
        if (!m_editViewState.m_selectionManager.isSelected(m_track))
        {
            buttonColour = buttonColour.darker(0.4f);
        }
        g.setColour(buttonColour);
        GUIHelpers::drawRoundedRectWithSide(g, area, cornerSize, true, false, true, false);

        auto borderRect = area;

        juce::Rectangle<float> trackColorIndicator = area.removeFromLeft(headWidth).toFloat();
        auto trackColor = m_track->getColour();
        g.setColour(trackColor);
        GUIHelpers::drawRoundedRectWithSide(g, trackColorIndicator, cornerSize, true, false, true, false);

        g.setColour(m_editViewState.m_applicationState.getBorderColour());
        // g.setColour(buttonColour.brighter(0.2f));
        GUIHelpers::strokeRoundedRectWithSide(g, borderRect, cornerSize, true, false, true, false);

        GUIHelpers::drawFromSvg(g, isMinimized ? BinaryData::arrowright18_svg : BinaryData::arrowdown18_svg, juce::Colour(0xff000000), {1, 6, 18, 18});

        g.setColour(juce::Colours::black);
        if (!isMinimized)
        {
            int strokeHeight = 0;
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
            const auto resizeFromTop = m_track->isMasterTrack();
            const auto y = resizeFromTop ? 0 : (height - strokeHeight);
            g.fillRect(juce::Rectangle<int>(headWidth - 1, y, getWidth() - headWidth, strokeHeight));
        }

        if (m_trackIsOver)
        {
            g.setColour(juce::Colour(0x66ffffff));
            g.drawRect(getLocalBounds().removeFromTop(1));
        }

        const char *icon = BinaryData::wavetest5_svg;
        ;
        if (m_track->isFolderTrack())
            icon = BinaryData::folderopen_svg;
        if (m_track->state.getProperty(IDs::isMidiTrack))
            icon = BinaryData::piano5_svg;

        GUIHelpers::drawFromSvg(g, icon, m_editViewState.m_applicationState.getTrackHeaderTextColour(), {20, 6, 18, 18});

        if (m_contentIsOver)
        {
            g.setColour(m_editViewState.m_applicationState.getPrimeColour());
            g.drawRect(getLocalBounds());
        }
    }
}

void TrackHeaderComponent::resized()
{
    // This method combines the original layout for the main header controls
    // with the new top-down layout for the automation headers to fix the ordering issue.

    // === Main Header Controls Layout (Original Logic) ===
    const int gap = 3;
    const int minimizedHeigth = 30;

    // Use a fixed 30px-high area for the main controls, regardless of the track's actual height.
    // This keeps the controls at a consistent size.
    auto controlArea = getLocalBounds().removeFromTop(minimizedHeigth);

    auto peakdisplay = controlArea.removeFromRight(15);
    peakdisplay.reduce(gap, gap);
    if (levelMeterComp)
        levelMeterComp->setBounds(peakdisplay);

    auto volSlider = controlArea.removeFromRight(controlArea.getHeight());
    if (m_volumeKnob)
        m_volumeKnob->setBounds(volSlider);

    auto gapX = 1;
    auto gapY = 7;
    auto buttonwidth = minimizedHeigth - 10;

    auto solo = controlArea.removeFromRight(buttonwidth).reduced(gapX, gapY);
    m_soloButton.setBounds(solo);
    m_soloButton.setComponentID("solo");
    m_soloButton.setTooltip("solo track");
    m_soloButton.setName("S");
    m_soloButton.setWantsKeyboardFocus(false);

    auto mute = controlArea.removeFromRight(buttonwidth).reduced(gapX, gapY);
    m_muteButton.setBounds(mute);
    m_muteButton.setComponentID("mute");
    m_muteButton.setTooltip("mute track");
    m_muteButton.setName("M");
    m_muteButton.setWantsKeyboardFocus(false);

    auto arm = controlArea.removeFromRight(buttonwidth).reduced(gapX, gapY);
    m_armButton.setBounds(arm);
    m_armButton.setTooltip("arm track");
    m_armButton.setComponentID("arm");
    m_armButton.setName("A");
    m_armButton.setWantsKeyboardFocus(false);

    controlArea.removeFromLeft(45);
    controlArea.removeFromTop(6);
    m_trackName.setBounds(controlArea);

    // === Automation Headers Layout (New Top-Down Logic) ===
    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    // Get the full height of the main track header (which can be > 30px when resized)
    int mainHeaderHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
    if (isFolderTrack())
        mainHeaderHeight = m_editViewState.m_folderTrackHeight;

    // The automation headers are drawn in the area below the main header
    auto automationArea = getLocalBounds();
    automationArea.removeFromTop(mainHeaderHeight);

    // Restore the original left indent to align the automation headers correctly.
    // The old code used `peakdisplay.getWidth()`, which was 15.
    automationArea.removeFromLeft(15);

    for (auto *ahs : m_automationHeaders)
    {
        auto ap = ahs->automatableParameter();
        int automationHeight = trackInfo->automationParameterHeights[ap];
        automationHeight = automationHeight < minimizedHeigth ? minimizedHeigth : automationHeight;
        // Lay out headers from the top to match the visual order
        ahs->setBounds(automationArea.removeFromTop(automationHeight));
    }
}

void TrackHeaderComponent::mouseDown(const juce::MouseEvent &event)
{
    m_trackHeightATMouseDown = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);

    m_yPosAtMouseDown = event.y;
    auto area = getLocalBounds().removeFromLeft(20);
    if (area.contains(event.getPosition()))
    {
        bool isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        m_editViewState.m_trackHeightManager->setMinimized(m_track, !isMinimized);
    }
    if (!event.mouseWasDraggedSinceMouseDown())
    {
        if (event.mods.isRightButtonDown())
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
        else if (event.mods.isLeftButtonDown())
        {
            if (event.mods.isCtrlDown())
            {
                m_editViewState.m_selectionManager.addToSelection(m_track);
            }
            else if (event.getNumberOfClicks() > 1)
            {
                if (!m_track->isMasterTrack())
                    m_trackName.showEditor();
            }
            else
            {
                m_editViewState.m_selectionManager.selectOnly(m_track);
                m_dragImage = createComponentSnapshot(getLocalBounds());

                m_editViewState.setLowerRangeView(LowerRangeView::pluginRack);
                for (auto t : te::getAllTracks(m_editViewState.m_edit))
                {
                    t->state.setProperty(IDs::showLowerRange, false, nullptr);
                    if (t == m_track.get())
                    {
                        t->state.setProperty(IDs::showLowerRange, true, nullptr);
                    }
                }

                if (auto *masterTrack = m_editViewState.m_edit.getMasterTrack())
                {
                    if (m_track->isMasterTrack())
                        masterTrack->state.setProperty(IDs::showLowerRange, true, nullptr);
                    else
                        masterTrack->state.setProperty(IDs::showLowerRange, false, nullptr);
                }
            }
        }
    }
}

void TrackHeaderComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (event.mouseWasDraggedSinceMouseDown())
    {
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        const auto resizeFromTop = m_track->isMasterTrack();
        const auto resizeHandleHit = resizeFromTop ? (m_yPosAtMouseDown < 10) : (m_yPosAtMouseDown > m_trackHeightATMouseDown - 10);

        if (resizeHandleHit && !isMinimized && !isFolderTrack())
        {
            m_isResizing = true;
            auto newHeight = static_cast<int>(resizeFromTop ? (m_trackHeightATMouseDown - event.getDistanceFromDragStartY()) : (m_trackHeightATMouseDown + event.getDistanceFromDragStartY()));

            auto &trackHeightManager = m_editViewState.m_trackHeightManager;
            trackHeightManager->setTrackHeight(m_track, newHeight);
            trackHeightManager->setMinimized(m_track, false);
            getParentComponent()->resized();
        }
        else
        {
            if (m_track->isMasterTrack())
                return;

            juce::DragAndDropContainer *dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);

            if (!dragC->isDragAndDropActive())
                dragC->startDragging("Track", this, m_dragImage);

            m_isDragging = true;
        }
    }
}

void TrackHeaderComponent::mouseUp(const juce::MouseEvent & /*e*/)
{
    m_isResizing = false;
    m_isDragging = false;
}

void TrackHeaderComponent::mouseMove(const juce::MouseEvent &e)
{
    if (!m_isResizing)
    {
        auto isMinimized = m_editViewState.m_trackHeightManager->isTrackMinimized(m_track);
        auto old = m_isHover;
        int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        const auto resizeFromTop = m_track->isMasterTrack();

        if ((resizeFromTop ? (e.y < 10) : (e.y > height - 10)) && !isMinimized && !isFolderTrack())
        {
            m_isHover = true;
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        else
        {
            m_isHover = false;
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }

        if (m_isHover != old)
        {
            auto stripeRect = getLocalBounds();
            stripeRect.removeFromBottom(getHeight() - height);
            repaint(resizeFromTop ? stripeRect.removeFromTop(10) : stripeRect.removeFromBottom(10));
        }
    }
}

void TrackHeaderComponent::mouseExit(const juce::MouseEvent & /*e*/)
{
    m_isHover = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    int height = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
    const auto resizeFromTop = m_track->isMasterTrack();
    auto stripeRect = getLocalBounds();
    stripeRect.removeFromBottom(getHeight() - height);
    repaint(resizeFromTop ? stripeRect.removeFromTop(10) : stripeRect.removeFromBottom(10));
}

juce::Colour TrackHeaderComponent::getTrackColour() { return m_track->getColour(); }
bool TrackHeaderComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry" || dragSourceDetails.description == "Track" || dragSourceDetails.description == "Instrument or Effect")
    {
        return true;
    }
    return false;
}

void TrackHeaderComponent::itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
        m_contentIsOver = true;
    else if (dragSourceDetails.description == "Track")
        m_trackIsOver = true;

    repaint();
}
void TrackHeaderComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails & /*dragSourceDetails*/)
{
    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
}
void TrackHeaderComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails &details)
{

    if (details.description == "Track" && m_track->isMasterTrack())
    {
        m_contentIsOver = false;
        m_trackIsOver = false;
        repaint();
        return;
    }

    if (details.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<PluginListbox *>(details.sourceComponent.get()))
        {
            EngineHelpers::insertPluginWithPreset(m_editViewState, getTrack(), listbox->getSelectedPlugin(m_editViewState.m_edit));
        }
    }

    if (details.description == "Instrument or Effect")
    {
        if (auto lb = dynamic_cast<InstrumentEffectTable *>(details.sourceComponent.get()))
        {
            EngineHelpers::insertPluginWithPreset(m_editViewState, getTrack(), lb->getSelectedPlugin(m_editViewState.m_edit));
        }
    }

    auto tc = dynamic_cast<TrackHeaderComponent *>(details.sourceComponent.get());
    auto isTrack = details.description == "Track";
    auto tip = te::TrackInsertPoint(*getTrack(), true);

    if (tc && isTrack)
    {
        if (isFolderTrack())
            tip = te::TrackInsertPoint(m_track, m_track->getAllSubTracks(false).getLast());

        m_editViewState.m_edit.moveTrack(tc->getTrack(), tip);
    }

    m_contentIsOver = false;
    m_trackIsOver = false;
    repaint();
}

void TrackHeaderComponent::labelTextChanged(juce::Label *labelThatHasChanged)
{
    if (m_track->isMasterTrack())
        return;

    if (labelThatHasChanged == &m_trackName)
    {
        m_track->setName(labelThatHasChanged->getText());
    }
}

void TrackHeaderComponent::childrenSetVisible(bool v)
{
    if (m_isAudioTrack)
    {
        m_armButton.setVisible(v);
        m_muteButton.setVisible(v);
        m_soloButton.setVisible(v);
        if (m_volumeKnob)
            m_volumeKnob->setVisible(v);
        m_trackName.setVisible(v);
    }
}

void TrackHeaderComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_updateAutomationLanes))
    {
        buildAutomationHeader();
    }
    if (compareAndReset(m_updateTrackHeight))
    {
        getParentComponent()->resized();
    }
}
void TrackHeaderComponent::collapseTrack(bool minimize) { m_editViewState.m_trackHeightManager->setMinimized(m_track, minimize); }

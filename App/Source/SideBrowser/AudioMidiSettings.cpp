
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

#include "SideBrowser/AudioMidiSettings.h"

MidiSettings::MidiSettings(tracktion_engine::Engine &engine, ApplicationViewState &appState)
    : m_engine(engine),
      m_appState(appState)
{
    addAndMakeVisible(m_midiDefaultChooser);
    addAndMakeVisible(m_midiDefaultLabel);
    m_midiDefaultChooser.addListener(this);
    m_midiDefaultLabel.attachToComponent(&m_midiDefaultChooser, true);
    m_midiDefaultLabel.setText("default Controller : ", juce::dontSendNotification);

    addAndMakeVisible(m_exclusiveMidiFocusButton);
    m_exclusiveMidiFocusButton.setButtonText("Exclusive MIDI Focus");
    m_exclusiveMidiFocusButton.addListener(this);
    m_exclusiveMidiFocusButton.setToggleState(m_appState.m_exclusiveMidiFocusEnabled, juce::dontSendNotification);
    addAndMakeVisible(m_exclusiveMidiFocusLabel);
    m_exclusiveMidiFocusLabel.attachToComponent(&m_exclusiveMidiFocusButton, true);
    m_exclusiveMidiFocusLabel.setText("Focus on selected Track:", juce::dontSendNotification);

    populateMidiDevices();
}
MidiSettings::~MidiSettings()
{
    m_midiDefaultChooser.removeListener(this);
    m_exclusiveMidiFocusButton.removeListener(this);
}
void MidiSettings::populateMidiDevices()
{
    m_midiDefaultChooser.clear(juce::dontSendNotification);

    auto &dm = m_engine.getDeviceManager();

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
    {
        if (auto mip = dm.getMidiInDevice(i))
        {
            if (mip->getDeviceType() == te::InputDevice::physicalMidiDevice || mip->getDeviceType() == te::InputDevice::virtualMidiDevice)
            {
                m_midiDefaultChooser.addItem(mip->getName(), i + 1);
                if (dm.getDefaultMidiInDevice() == mip.get())
                {
                    m_midiDefaultChooser.setSelectedId(i + 1);
                }
            }
        }
    }
}

void MidiSettings::resized()
{
    auto area = getLocalBounds();
    auto defaultController = area.removeFromTop(30);
    defaultController.removeFromRight(getWidth() * 0.05);
    defaultController.removeFromLeft(defaultController.getWidth() / 3);
    m_midiDefaultChooser.setBounds(defaultController);

    auto exclusiveFocusRow = area.removeFromTop(30);
    exclusiveFocusRow.removeFromRight(getWidth() * 0.05);
    exclusiveFocusRow.removeFromLeft(exclusiveFocusRow.getWidth() / 3);
    m_exclusiveMidiFocusButton.setBounds(exclusiveFocusRow);
}

void MidiSettings::visibilityChanged()
{
    if (isVisible())
        populateMidiDevices();
}

void MidiSettings::comboBoxChanged(juce::ComboBox *comboBox)
{
    auto &dm = m_engine.getDeviceManager();
    if (auto defaultMidiIn = dm.getMidiInDevice(comboBox->getSelectedId() - 1))
        dm.setDefaultMidiInDevice(defaultMidiIn->getDeviceID());
}

void MidiSettings::buttonClicked(juce::Button *button)
{
    if (button == &m_exclusiveMidiFocusButton)
    {
        m_appState.m_exclusiveMidiFocusEnabled = m_exclusiveMidiFocusButton.getToggleState();
    }
}

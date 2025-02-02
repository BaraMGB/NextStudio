
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

#include "AudioMidiSettings.h"
#include "ApplicationViewState.h"

MidiSettings::MidiSettings(tracktion_engine::Engine &engine)
  : m_engine(engine)
{
    addAndMakeVisible (m_midiDefaultChooser);
    addAndMakeVisible (m_midiDefaultLabel);
    m_midiDefaultChooser.addListener (this);
    m_midiDefaultLabel.attachToComponent(&m_midiDefaultChooser, true);
    m_midiDefaultLabel.setText("default Controller : ", juce::dontSendNotification);
}

void MidiSettings::resized()
{
    auto& dm = m_engine.getDeviceManager ();
    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
    {
        if (auto mip = dm.getMidiInDevice (i))
        {
            GUIHelpers::log("AudioMidiSettings.cpp: MIDI  Input Devices:" + mip->getName());
            if (mip->getDeviceType() == te::InputDevice::physicalMidiDevice)
            {
                m_midiDefaultChooser.addItem (mip->getName (), i + 1);
                if (dm.getDefaultMidiInDevice () == mip.get())
                {
                    m_midiDefaultChooser.setSelectedId (i + 1);
                }
            }
        }
    }
    auto area = getLocalBounds ();
    auto defaultController = area.removeFromTop(30);
    defaultController.removeFromRight(getWidth() * 0.05);
    defaultController.removeFromLeft(defaultController.getWidth() / 3);
    m_midiDefaultChooser.setBounds (defaultController );
}

void MidiSettings::comboBoxChanged(juce::ComboBox *comboBox)
{
    auto& dm = m_engine.getDeviceManager ();
    if (auto defaultMidiIn = dm.getMidiInDevice(comboBox->getSelectedId () - 1))
        dm.setDefaultMidiInDevice (defaultMidiIn->getDeviceID());
}

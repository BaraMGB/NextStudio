
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

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "PluginBrowser.h"

namespace te = tracktion_engine;
class MidiSettings : public juce::Component
, public juce::ComboBox::Listener
{
public:

    explicit MidiSettings(te::Engine& engine);

    void resized () override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;

private:

    juce::ComboBox m_midiDefaultChooser;
    juce::Label m_midiDefaultLabel;
    te::Engine& m_engine;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSettings)
};

// ----------------------------------------------------------------

class SettingsView : public juce::TabbedComponent
{
public:
    SettingsView(te::Engine& engine, juce::ApplicationCommandManager& commandManager) : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop)
        , m_commandManager(commandManager)
        , m_midiSettings(engine)
        , m_audioSettings(engine.getDeviceManager().deviceManager, 0, 512, 1, 512, false, false, false, false) 
        , m_keyMappingEditor(*m_commandManager.getKeyMappings(), true)
        , m_pluginBrowser(engine)
    {
        addTab("Audio Settings", juce::Colours::darkgrey, &m_audioSettings, true);
        addTab("MIDI Settings", juce::Colours::darkgrey, &m_midiSettings, true);
        addTab("Plugin Settings", juce::Colours::darkgrey, &m_pluginBrowser, true);
        addTab("KeyMapping", juce::Colours::darkgrey, &m_keyMappingEditor, true);
    }

private:

    juce::ApplicationCommandManager & m_commandManager;
    MidiSettings m_midiSettings;
    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::KeyMappingEditorComponent m_keyMappingEditor;
    PluginSettings m_pluginBrowser;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsView)
};



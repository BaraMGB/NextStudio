
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
#include "ApplicationViewState.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "PluginBrowser.h"
#include "NextLookAndFeel.h"

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
class GeneralSettings : public juce::Component
{
public:
    GeneralSettings(ApplicationViewState& appState)
        : m_appState(appState)
    {
        m_scaleLabel.setText("Scaling Factor:", juce::dontSendNotification);
        addAndMakeVisible(m_scaleLabel);

        m_scaleEditor.setMultiLine(false);
        m_scaleEditor.setJustification(juce::Justification::centredLeft);

        m_scaleEditor.setText(juce::String(juce::Desktop::getInstance().getGlobalScaleFactor()), juce::dontSendNotification);

        addAndMakeVisible(m_scaleEditor);

        m_scaleEditor.onFocusLost = [this]()
        {
            updateScaling();
        };

        m_scaleEditor.onReturnKey = [this]()
        {
            updateScaling();
        };

    }

    ~GeneralSettings() override
    {
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto labelWidth = 100; // Breite für das Label
        auto padding = 10;     // Abstand zwischen Label und Editor

        m_scaleLabel.setBounds(bounds.getX(), bounds.getY(), labelWidth, 24); // Höhe 24 ist typisch
        m_scaleEditor.setBounds(bounds.getX() + labelWidth + padding, bounds.getY(), bounds.getWidth() - labelWidth - padding, 24);

    }


private:

    ApplicationViewState& m_appState;

    juce::Label m_scaleLabel;
    juce::TextEditor m_scaleEditor;

    void updateScaling()
    {
        float newScale = m_scaleEditor.getText().getFloatValue();

        if (newScale >= 0.5f && newScale <= 2.0f)
        {
            juce::Desktop::getInstance().setGlobalScaleFactor(newScale);
            m_appState.m_appScale = newScale;
        }
        else
        {
            m_scaleEditor.setText(juce::String(juce::Desktop::getInstance().getGlobalScaleFactor()), juce::dontSendNotification);
        }
    }

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GeneralSettings)
};
// ----------------------------------------------------------------

class SettingsView : public juce::TabbedComponent
{
public:
    SettingsView(te::Engine& engine, juce::ApplicationCommandManager& commandManager, ApplicationViewState& appState) : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop)
        , m_commandManager(commandManager)
        , m_midiSettings(engine)
        , m_generalSettings(appState)
        , m_audioSettings(engine.getDeviceManager().deviceManager, 0, 512, 1, 512, false, false, false, false) 
        , m_keyMappingEditor(*m_commandManager.getKeyMappings(), true)
        , m_pluginBrowser(engine, appState)
    {
        setOutline(0);
        m_keyMappingEditor.setColours(appState.getMenuBackgroundColour(), appState.getTextColour());
        addTab("Audio", appState.getMenuBackgroundColour(), &m_audioSettings, true);
        addTab("MIDI", appState.getMenuBackgroundColour(), &m_midiSettings, true);
        addTab("Plugins",appState.getMenuBackgroundColour(), &m_pluginBrowser, true);
        addTab("General",appState.getMenuBackgroundColour(), &m_generalSettings, true);
        addTab("Keys", appState.getMenuBackgroundColour(), &m_keyMappingEditor, true);
    }

private:

    juce::ApplicationCommandManager & m_commandManager;
    MidiSettings m_midiSettings;
    GeneralSettings m_generalSettings;
    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::KeyMappingEditorComponent m_keyMappingEditor;
    PluginSettings m_pluginBrowser;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsView)
};



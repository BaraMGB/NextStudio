#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


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
    {
        addTab("Audio Settings", juce::Colours::darkgrey, &m_audioSettings, true);
        addTab("MIDI Settings", juce::Colours::darkgrey, &m_midiSettings, true);
        addTab("KeyMapping", juce::Colours::darkgrey, &m_keyMappingEditor, true);
    }

private:

    juce::ApplicationCommandManager & m_commandManager;
    MidiSettings m_midiSettings;
    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::KeyMappingEditorComponent m_keyMappingEditor;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsView)
};



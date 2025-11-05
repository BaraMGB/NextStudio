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

#include "PresetManagerComponent.h"
#include "Utilities.h"

PresetManagerComponent::PresetManagerComponent(PluginPresetInterface* pluginInterface)
    : m_pluginInterface(pluginInterface)
{
    m_presetCombo.reset(new juce::ComboBox("Presets"));
    m_presetCombo->setTextWhenNothingSelected("Select Preset");
    m_presetCombo->onChange = [this] { loadPresetFromCombo(); };
    addAndMakeVisible(*m_presetCombo);

    m_saveButton.reset(new juce::TextButton("Save"));
    m_saveButton->onClick = [this] { savePreset(); };
    addAndMakeVisible(*m_saveButton);

    m_loadButton.reset(new juce::TextButton("Load"));
    m_loadButton->onClick = [this] { loadPreset(); };
    addAndMakeVisible(*m_loadButton);

    refreshPresetList();
    loadOrInitialiseDefaultPreset();
}

void PresetManagerComponent::paint(juce::Graphics& g)
{
    auto background1 = m_pluginInterface->getApplicationViewState().getBackgroundColour1();
    auto background2 = m_pluginInterface->getApplicationViewState().getBackgroundColour2();
    auto borderColour = m_pluginInterface->getApplicationViewState().getBorderColour();

    g.fillAll(background2);
    g.setColour(background1);

    auto area = getLocalBounds().reduced(2);
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);

    g.setColour(borderColour);
    GUIHelpers::strokeRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);
}

void PresetManagerComponent::resized()
{
    auto area = getLocalBounds().reduced(5);

    int buttonHeight = 25;
    int spacing = 5;

    // Preset combo box (takes most space)
    m_presetCombo->setBounds(area.removeFromTop(buttonHeight));

    // Spacing
    area.removeFromTop(spacing);

    // Save button (full width)
    m_saveButton->setBounds(area.removeFromTop(buttonHeight));

    // Spacing
    area.removeFromTop(spacing);

    // Load button (full width)
    m_loadButton->setBounds(area.removeFromTop(buttonHeight));
}

void PresetManagerComponent::refreshPresetList()
{
    if (m_presetCombo == nullptr)
        return;

    m_presetCombo->clear();

    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    // Add all preset files to combo box
    juce::Array<juce::File> presetFiles;
    presetDir.findChildFiles(presetFiles, juce::File::findFiles, false, "*.nxtpreset");

    // Sort alphabetically
    presetFiles.sort();

    m_presetCombo->addItem("None", 1);

    for (int i = 0; i < presetFiles.size(); ++i)
    {
        juce::String presetName = presetFiles[i].getFileNameWithoutExtension();
        m_presetCombo->addItem(presetName, i + 2); // +2 because 1 is "None"
    }

    // Set selected item to "None" initially
    m_presetCombo->setSelectedId(1, juce::dontSendNotification);
}

void PresetManagerComponent::loadPresetFromCombo()
{
    if (m_presetCombo == nullptr || m_pluginInterface == nullptr)
        return;

    int selectedId = m_presetCombo->getSelectedId();

    if (selectedId == 1) // "None" selected
        return;

    auto presetDir = getPresetDirectory();
    juce::Array<juce::File> presetFiles;
    presetDir.findChildFiles(presetFiles, juce::File::findFiles, false, "*.nxtpreset");
    presetFiles.sort();

    int presetIndex = selectedId - 2; // -2 because 1 is "None"
    if (presetIndex >= 0 && presetIndex < presetFiles.size())
    {
        juce::File presetFile = presetFiles[presetIndex];
        if (presetFile.existsAsFile())
        {
            if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
            {
                juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);
                if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface->getPluginTypeName()) // Check if it's a valid preset for this plugin
                {
                    // Use plugin's restore method
                    m_pluginInterface->restorePluginState(presetState);
                }
            }
        }
    }
}

void PresetManagerComponent::savePreset()
{
    if (m_pluginInterface == nullptr)
        return;

    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    juce::FileChooser fc ("Save Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToSave (true))
    {
        juce::File presetFile = fc.getResult();
        // Ensure file has the correct extension
        if (!presetFile.hasFileExtension(".nxtpreset"))
        {
            presetFile = presetFile.withFileExtension(".nxtpreset");
        }

        juce::String presetName = presetFile.getFileNameWithoutExtension();

        // Get the plugin's current state
        juce::ValueTree pluginState = m_pluginInterface->getPluginState();
        pluginState.setProperty("name", presetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginState.createXml()))
        {
            xml->writeTo(presetFile, {});

            // Refresh preset list after saving
            refreshPresetList();

            // Select the newly saved preset
            for (int i = 1; i < m_presetCombo->getNumItems(); ++i)
            {
                if (m_presetCombo->getItemText(i) == presetName)
                {
                    m_presetCombo->setSelectedId(m_presetCombo->getItemId(i), juce::dontSendNotification);
                    break;
                }
            }
        }
    }
}

void PresetManagerComponent::loadPreset()
{
    if (m_pluginInterface == nullptr)
        return;

    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    juce::FileChooser fc ("Load Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToOpen())
    {
        juce::File presetFile = fc.getResult();
        if (presetFile.existsAsFile())
        {
            if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
            {
                juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

                // Check if it's a valid preset for this plugin
                if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface->getPluginTypeName())
                {
                    // Use plugin's restore method
                    m_pluginInterface->restorePluginState(presetState);
                }
            }
        }
    }
}

juce::File PresetManagerComponent::getPresetDirectory()
{
    return juce::File(m_pluginInterface->getApplicationViewState().m_presetDir.get())
    .getChildFile(m_pluginInterface->getPresetSubfolder());
}

void PresetManagerComponent::ensurePresetDirectoryExists()
{
    auto presetDir = getPresetDirectory();
    if (!presetDir.exists())
    {
        presetDir.createDirectory();
    }
}

void PresetManagerComponent::loadOrInitialiseDefaultPreset()
{
    ensurePresetDirectoryExists();
    auto presetFile = getPresetDirectory().getChildFile("init.nxtpreset");

    if (presetFile.existsAsFile())
    {
        if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
        {
            juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);
            if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface->getPluginTypeName())
            {
                m_pluginInterface->restorePluginState(presetState);

                // Select "init" in the combo box
                for (int i = 0; i < m_presetCombo->getNumItems(); ++i)
                {
                    if (m_presetCombo->getItemText(i).equalsIgnoreCase("init"))
                    {
                        m_presetCombo->setSelectedId(m_presetCombo->getItemId(i), juce::dontSendNotification);
                        break;
                    }
                }
            }
        }
    }
    else
{
        // 'init.nxtpreset' does not exist, so create it from the current state
        juce::String presetName = "init";
        juce::ValueTree pluginState = m_pluginInterface->getPluginState();
        pluginState.setProperty("name", presetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginState.createXml()))
        {
            xml->writeTo(presetFile, {});

            // Refresh preset list to include the new "init" preset
            refreshPresetList();

            // Select the newly created "init" preset
            for (int i = 0; i < m_presetCombo->getNumItems(); ++i)
            {
                if (m_presetCombo->getItemText(i).equalsIgnoreCase("init"))
                {
                    m_presetCombo->setSelectedId(m_presetCombo->getItemId(i), juce::dontSendNotification);
                    break;
                }
            }
        }
    }
}

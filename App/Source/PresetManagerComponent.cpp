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
    selectPreset(m_pluginInterface->getLastLoadedPresetName());
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

void PresetManagerComponent::selectPreset(const juce::String& name)
{
    if (name.isEmpty())
    {
        m_presetCombo->setSelectedId(-1, juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < m_presetCombo->getNumItems(); ++i)
    {
        if (m_presetCombo->getItemText(i).equalsIgnoreCase(name))
        {
            m_presetCombo->setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }

    GUIHelpers::log("PresetManagerComponent: Preset '" + name + "' not found in list");

    m_presetCombo->setSelectedId(-1, juce::dontSendNotification);
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

    for (int i = 0; i < presetFiles.size(); ++i)
    {
        juce::String presetName = presetFiles[i].getFileNameWithoutExtension();
        m_presetCombo->addItem(presetName, i + 1); // +1 because 0 is not used
    }
}

void PresetManagerComponent::loadPresetFromCombo()
{
    if (m_presetCombo == nullptr || m_pluginInterface == nullptr)
        return;

    int selectedId = m_presetCombo->getSelectedId();

    auto presetDir = getPresetDirectory();
    juce::Array<juce::File> presetFiles;
    presetDir.findChildFiles(presetFiles, juce::File::findFiles, false, "*.nxtpreset");
    presetFiles.sort();

    int presetIndex = selectedId - 1; // -1 because 0 is not used
    if (presetIndex >= 0 && presetIndex < presetFiles.size())
    {
        juce::File presetFile = presetFiles[presetIndex];
        if (presetFile.existsAsFile())
        {
            // XML validation: Only read and check if it has a valid PLUGIN tag
            juce::String xmlContent = presetFile.loadFileAsString();
            if (xmlContent.contains("<PLUGIN") && xmlContent.contains("</PLUGIN>"))
            {
                if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
                {
                    juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);
                    // Check if it's a valid preset for this plugin
                    if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface->getPluginTypeName())
                    {
                        m_pluginInterface->restorePluginState(presetState);
                        m_pluginInterface->setLastLoadedPresetName(presetFile.getFileNameWithoutExtension());
                        m_pluginInterface->setInitialPresetLoaded(true);
                    }
                }
            }
            else
            {
                GUIHelpers::log("PresetManagerComponent: Invalid XML format in " + presetFile.getFullPathName());
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

        // Validation: Only allow alphanumeric characters, dashes and underscores
        juce::String safePresetName = presetName.retainCharacters("a-zA-Z0-9_- ");
        if (safePresetName.isEmpty())
        {
            GUIHelpers::log("PresetManagerComponent: Invalid preset name: " + presetName);
            return;
        }

        // If the name was changed, rename the file
        if (safePresetName != presetName)
        {
            presetFile = presetFile.getSiblingFile(safePresetName + ".nxtpreset");
        }

        // Get the plugin's current state
        juce::ValueTree pluginState = m_pluginInterface->getPluginState();
        pluginState.setProperty("name", safePresetName, nullptr);

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
            // XML validation: Only read and check if it has a valid PLUGIN tag
            juce::String xmlContent = presetFile.loadFileAsString();
            if (xmlContent.contains("<PLUGIN") && xmlContent.contains("</PLUGIN>"))
            {
                if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
                {
                    juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

                    // Check if it's a valid preset for this plugin
                    if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface->getPluginTypeName())
                    {
                        m_pluginInterface->restorePluginState(presetState);
                        m_pluginInterface->setLastLoadedPresetName(presetFile.getFileNameWithoutExtension());
                        m_pluginInterface->setInitialPresetLoaded(true);
                    }
                }
            }
            else
            {
                GUIHelpers::log("PresetManagerComponent: Invalid XML format in " + presetFile.getFullPathName());
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
    // Only load/initialise if no preset has been loaded yet for this plugin instance
    if (m_pluginInterface->getInitialPresetLoaded())
        return;

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
                m_pluginInterface->setInitialPresetLoaded(true);
                m_pluginInterface->setLastLoadedPresetName("init"); // Mark "init" as the last loaded
            }
        }
    }
    else
    {
        // 'init.nxtpreset' does not exist, so create it from the default state
        juce::String presetName = "init";
        juce::ValueTree pluginDefaultState = m_pluginInterface->getFactoryDefaultState();
        pluginDefaultState.setProperty("name", presetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginDefaultState.createXml()))
        {
            xml->writeTo(presetFile, {});
            refreshPresetList(); // Refresh to include the new "init" preset
            m_pluginInterface->setInitialPresetLoaded(true);
            m_pluginInterface->setLastLoadedPresetName("init"); // Mark "init" as the last loaded
        }
    }
}

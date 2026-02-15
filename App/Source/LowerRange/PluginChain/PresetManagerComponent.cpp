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

#include "LowerRange/PluginChain/PresetManagerComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

PresetManagerComponent::PresetManagerComponent(PluginPresetInterface &pluginInterface, juce::Colour headerColour)
    : m_pluginInterface(pluginInterface),
      m_headerColour(headerColour)
{
    m_presetCombo = std::make_unique<juce::ComboBox>("Presets");
    m_presetCombo->setTextWhenNothingSelected("Select Preset");
    m_presetCombo->onChange = [this] { loadPresetFromCombo(); };
    addAndMakeVisible(*m_presetCombo);

    m_saveButton = std::make_unique<juce::TextButton>("Save");
    m_saveButton->onClick = [this] { savePreset(); };
    addAndMakeVisible(*m_saveButton);

    m_loadButton = std::make_unique<juce::TextButton>("Load");
    m_loadButton->onClick = [this] { loadPreset(); };
    addAndMakeVisible(*m_loadButton);

    refreshPresetList();
    selectPreset(m_pluginInterface.getLastLoadedPresetName());
}

void PresetManagerComponent::paint(juce::Graphics &g)
{
    auto &appState = m_pluginInterface.getApplicationViewState();
    auto borderColour = appState.getBorderColour();
    auto backgroundColour = appState.getBackgroundColour1();

    GUIHelpers::drawHeaderBox(g, getLocalBounds().reduced(2).toFloat(), m_headerColour, borderColour, backgroundColour, 20.0f, GUIHelpers::HeaderPosition::top, "Presets");
}

void PresetManagerComponent::setHeaderColour(juce::Colour colour)
{
    if (m_headerColour == colour)
        return;

    m_headerColour = colour;
    repaint();
}

void PresetManagerComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    area.removeFromTop(20); // header

    int buttonHeight = 25;
    int spacing = 5;

    // Preset combo box (takes most space)
    if (m_presetCombo)
        m_presetCombo->setBounds(area.removeFromTop(buttonHeight));

    // Spacing
    area.removeFromTop(spacing);

    // Save button (full width)
    if (m_saveButton)
        m_saveButton->setBounds(area.removeFromTop(buttonHeight));

    // Spacing
    area.removeFromTop(spacing);

    // Load button (full width)
    if (m_loadButton)
        m_loadButton->setBounds(area.removeFromTop(buttonHeight));
}

void PresetManagerComponent::selectPreset(const juce::String &name)
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
    if (m_presetCombo == nullptr)
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
            // Use XmlDocument::parse for robust XML parsing
            if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
            {
                if (xml->hasTagName("PLUGIN"))
                {
                    juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);
                    // Check if it's a valid preset for this plugin
                    if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface.getPluginTypeName())
                    {
                        m_pluginInterface.restorePluginState(presetState);
                        m_pluginInterface.setLastLoadedPresetName(presetFile.getFileNameWithoutExtension());
                        m_pluginInterface.setInitialPresetLoaded(true);
                    }
                    else
                    {
                        GUIHelpers::log("PresetManagerComponent: Preset type mismatch or invalid format in " + presetFile.getFileName());
                    }
                }
                else
                {
                    GUIHelpers::log("PresetManagerComponent: Root element is not <PLUGIN> in " + presetFile.getFileName());
                }
            }
            else
            {
                GUIHelpers::log("PresetManagerComponent: Failed to parse XML in " + presetFile.getFullPathName());
            }
        }
    }
}

void PresetManagerComponent::savePreset()
{
    // Capture state immediately before opening any modal dialogs
    juce::ValueTree pluginState = m_pluginInterface.getPluginState();

    juce::Component::SafePointer<PresetManagerComponent> safeThis(this);

    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    juce::FileChooser fc("Save Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToSave(true))
    {
        juce::File presetFile = fc.getResult();

        // Ensure file has the correct extension
        if (!presetFile.hasFileExtension(".nxtpreset"))
        {
            presetFile = presetFile.withFileExtension(".nxtpreset");
        }

        juce::String presetName = presetFile.getFileNameWithoutExtension();

        // Sanitize the preset name to create a legal filename
        juce::String safePresetName = juce::File::createLegalFileName(presetName);

        if (safePresetName.isEmpty())
        {
            // Optionally, show an alert to the user
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Preset Name", "The chosen preset name is invalid or contains only illegal characters.");
            return;
        }

        // If sanitization changed the name, update the file object to use the new name
        if (safePresetName != presetName)
        {
            presetFile = presetFile.getSiblingFile(safePresetName + ".nxtpreset");
        }

        // Update the name property in the captured state
        pluginState.setProperty("name", safePresetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginState.createXml()))
        {
            xml->writeTo(presetFile, {});

            if (safeThis == nullptr)
                return;

            // Refresh preset list after saving
            refreshPresetList();

            // Select the newly saved preset
            for (int i = 0; i < m_presetCombo->getNumItems(); ++i)
            {
                if (m_presetCombo->getItemText(i) == safePresetName)
                {
                    m_presetCombo->setSelectedItemIndex(i, juce::dontSendNotification);
                    break;
                }
            }
        }
    }
}

void PresetManagerComponent::loadPreset()
{
    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    juce::Component::SafePointer<PresetManagerComponent> safeThis(this);

    juce::FileChooser fc("Load Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToOpen())
    {
        if (safeThis == nullptr)
            return;

        juce::File presetFile = fc.getResult();
        if (presetFile.existsAsFile())
        {
            // Use XmlDocument::parse for robust XML parsing
            if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
            {
                if (xml->hasTagName("PLUGIN"))
                {
                    juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

                    // Check if it's a valid preset for this plugin
                    if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == m_pluginInterface.getPluginTypeName())
                    {
                        m_pluginInterface.restorePluginState(presetState);
                        m_pluginInterface.setLastLoadedPresetName(presetFile.getFileNameWithoutExtension());
                        m_pluginInterface.setInitialPresetLoaded(true);
                    }
                    else
                    {
                        GUIHelpers::log("PresetManagerComponent: Preset type mismatch or invalid format in " + presetFile.getFileName());
                    }
                }
                else
                {
                    GUIHelpers::log("PresetManagerComponent: Root element is not <PLUGIN> in " + presetFile.getFileName());
                }
            }
            else
            {
                GUIHelpers::log("PresetManagerComponent: Failed to parse XML in " + presetFile.getFullPathName());
            }
        }
    }
}

juce::File PresetManagerComponent::getPresetDirectory() { return PresetHelpers::getPresetDirectory(m_pluginInterface); }

void PresetManagerComponent::ensurePresetDirectoryExists()
{
    auto presetDir = getPresetDirectory();
    if (!presetDir.exists())
    {
        presetDir.createDirectory();
    }
}

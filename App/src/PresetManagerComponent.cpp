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
#include "PresetHelpers.h"
#include "Utilities.h"

namespace
{
std::unique_ptr<juce::XmlElement> parsePresetXml(const juce::File &presetFile)
{
    return std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile));
}

bool isPresetFileCompatible(const juce::File &presetFile, const PluginPresetInterface &pluginInterface)
{
    if (!presetFile.existsAsFile())
        return false;

    if (auto xml = parsePresetXml(presetFile))
    {
        if (!xml->hasTagName("PLUGIN"))
            return false;

        const auto presetState = juce::ValueTree::fromXml(*xml);
        // Deliberately use the cheap visibility filter here. This runs during preset-list
        // rebuilding for routine UI updates, so performing deep compatibility validation here
        // (especially anything that instantiates plugins) would make track selection sluggish.
        return pluginInterface.isPresetVisibleInList(presetState);
    }

    return false;
}

bool containsPresetNameIgnoreCase(const juce::StringArray &names, const juce::String &name)
{
    for (const auto &existing : names)
        if (existing.equalsIgnoreCase(name))
            return true;

    return false;
}

struct PresetFileNameComparator
{
    static int compareElements(const juce::File &first, const juce::File &second)
    {
        return first.getFileNameWithoutExtension().compareNatural(second.getFileNameWithoutExtension());
    }
};

PresetFileNameComparator presetFileNameComparator;
} // namespace

PresetManagerComponent::PresetManagerComponent(PluginPresetInterface &pluginInterface, juce::Colour headerColour, juce::String title)
    : m_pluginInterface(pluginInterface),
      m_headerColour(headerColour),
      m_title(title)
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

    GUIHelpers::drawHeaderBox(g, getLocalBounds().reduced(2).toFloat(), m_headerColour, borderColour, backgroundColour, 20.0f, GUIHelpers::HeaderPosition::top, m_title);
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
    area.removeFromTop(20);

    constexpr int buttonHeight = 25;
    constexpr int spacing = 5;

    if (m_presetCombo)
        m_presetCombo->setBounds(area.removeFromTop(buttonHeight));

    area.removeFromTop(spacing);

    if (m_saveButton)
        m_saveButton->setBounds(area.removeFromTop(buttonHeight));

    area.removeFromTop(spacing);

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

    m_presetCombo->setText(name, juce::dontSendNotification);
}

void PresetManagerComponent::refreshPresetList()
{
    if (m_presetCombo == nullptr)
        return;

    const auto previousSelection = m_presetCombo->getText();

    // Rebuilding the combo must not trigger an implicit preset load.
    m_isRefreshingPresetList = true;
    m_presetCombo->clear(juce::dontSendNotification);

    ensurePresetDirectoryExists();
    auto presetFiles = getAvailablePresetFiles();

    for (int i = 0; i < presetFiles.size(); ++i)
    {
        juce::String presetName = presetFiles[i].getFileNameWithoutExtension();
        m_presetCombo->addItem(presetName, i + 1); // +1 because 0 is not used
    }

    m_isRefreshingPresetList = false;

    if (previousSelection.isNotEmpty())
        selectPreset(previousSelection);
}

void PresetManagerComponent::loadPresetFromCombo()
{
    if (m_presetCombo == nullptr || m_isRefreshingPresetList)
        return;

    const int selectedId = m_presetCombo->getSelectedId();
    const auto presetFiles = getAvailablePresetFiles();
    const int presetIndex = selectedId - 1;

    if (juce::isPositiveAndBelow(presetIndex, presetFiles.size()))
        applyPresetFile(presetFiles.getReference(presetIndex));
}

void PresetManagerComponent::savePreset()
{
    juce::ValueTree pluginState = m_pluginInterface.getPluginState();

    juce::Component::SafePointer<PresetManagerComponent> safeThis(this);

    auto presetDir = getPresetDirectory();
    ensurePresetDirectoryExists();

    juce::FileChooser fc("Save Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToSave(true))
    {
        juce::File presetFile = fc.getResult();

        if (!presetFile.hasFileExtension(".nxtpreset"))
            presetFile = presetFile.withFileExtension(".nxtpreset");

        juce::String presetName = presetFile.getFileNameWithoutExtension();
        juce::String safePresetName = juce::File::createLegalFileName(presetName);

        if (safePresetName.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Preset Name", "The chosen preset name is invalid or contains only illegal characters.");
            return;
        }

        if (safePresetName != presetName)
            presetFile = presetFile.getSiblingFile(safePresetName + ".nxtpreset");

        pluginState.setProperty("name", safePresetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginState.createXml()))
        {
            xml->writeTo(presetFile, {});

            if (safeThis == nullptr)
                return;

            refreshPresetList();
            selectPreset(safePresetName);
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

        applyPresetFile(fc.getResult());
    }
}

void PresetManagerComponent::applyPresetFile(const juce::File &presetFile)
{
    if (!presetFile.existsAsFile())
        return;

    if (auto xml = parsePresetXml(presetFile))
    {
        if (!xml->hasTagName("PLUGIN"))
        {
            GUIHelpers::log("PresetManagerComponent: Root element is not <PLUGIN> in " + presetFile.getFileName());
            return;
        }

        auto presetState = juce::ValueTree::fromXml(*xml);
        if (!m_pluginInterface.applyPresetState(presetState))
        {
            GUIHelpers::log("PresetManagerComponent: Preset type mismatch or invalid format in " + presetFile.getFileName());
            return;
        }

        m_pluginInterface.setLastLoadedPresetName(presetFile.getFileNameWithoutExtension());
        m_pluginInterface.setInitialPresetLoaded(true);
        selectPreset(presetFile.getFileNameWithoutExtension());
        return;
    }

    GUIHelpers::log("PresetManagerComponent: Failed to parse XML in " + presetFile.getFullPathName());
}

juce::Array<juce::File> PresetManagerComponent::getAvailablePresetFiles()
{
    juce::Array<juce::File> searchDirs;
    searchDirs.add(getPresetDirectory());
    searchDirs.addArray(m_pluginInterface.getAdditionalPresetSearchDirectories());

    juce::Array<juce::File> presetFiles;
    juce::StringArray seenNames;

    for (const auto &dir : searchDirs)
    {
        if (!dir.exists())
            continue;

        juce::Array<juce::File> dirFiles;
        dir.findChildFiles(dirFiles, juce::File::findFiles, false, "*.nxtpreset");
        dirFiles.sort(presetFileNameComparator, true);

        for (const auto &file : dirFiles)
        {
            const auto presetName = file.getFileNameWithoutExtension();
            if (containsPresetNameIgnoreCase(seenNames, presetName))
                continue;

            if (!isPresetFileCompatible(file, m_pluginInterface))
                continue;

            presetFiles.add(file);
            seenNames.add(presetName);
        }
    }

    presetFiles.sort(presetFileNameComparator, true);
    return presetFiles;
}

juce::File PresetManagerComponent::getPresetDirectory() { return PresetHelpers::getPresetDirectory(m_pluginInterface); }

void PresetManagerComponent::ensurePresetDirectoryExists()
{
    auto presetDir = getPresetDirectory();
    if (!presetDir.exists())
        presetDir.createDirectory();
}

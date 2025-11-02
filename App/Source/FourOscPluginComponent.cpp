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

#include "FourOscPluginComponent.h"


void SimpleFourOscPluginComponent::savePreset()
{
    auto presetDir = juce::File(m_editViewState.m_applicationState.m_presetDir).getChildFile("FourOSC");
    // Ensure directory exists
    if (!presetDir.exists())
    {
        presetDir.createDirectory();
    }

    juce::FileChooser fc ("Save Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToSave (true))
    {
        juce::File presetFile = fc.getResult();
        // Ensure the file has the correct extension
        if (!presetFile.hasFileExtension(".nxtpreset"))
        {
            presetFile = presetFile.withFileExtension(".nxtpreset");
        }

        juce::String presetName = presetFile.getFileNameWithoutExtension();

        // Get the plugin's current state
        juce::ValueTree pluginState = m_fourOscPlugin->state;
        pluginState.setProperty("name", presetName, nullptr);

        if (auto xml = std::unique_ptr<juce::XmlElement>(pluginState.createXml()))
        {
            xml->writeTo(presetFile, {});
        }
    }
}

void SimpleFourOscPluginComponent::loadPreset()
{
    auto presetDir = juce::File(m_editViewState.m_applicationState.m_presetDir).getChildFile("FourOSC");
    // Ensure directory exists
    if (!presetDir.exists())
    {
        presetDir.createDirectory();
    }

    juce::FileChooser fc ("Load Preset", presetDir, "*.nxtpreset");

    if (fc.browseForFileToOpen())
    {
        juce::File presetFile = fc.getResult();
        if (presetFile.existsAsFile())
        {
            if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
            {
                juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

                // Check if it's a valid FourOsc preset
                if (presetState.hasType(m_fourOscPlugin->state.getType()))                {
                    // Use the proper restore method that handles all parameters correctly
                    m_fourOscPlugin->restorePluginStateFromValueTree(presetState);
                }
            }
        }
    }
}

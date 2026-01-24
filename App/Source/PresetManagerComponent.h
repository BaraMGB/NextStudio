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

#pragma once

#include "PluginPresetInterface.h"

/**
 * Reusable preset management component for all plugins
 * Provides save/load functionality with a combo box showing all presets
 */
class PresetManagerComponent : public juce::Component
{
public:
    // Constructor requires a valid interface reference. No nullptr possible.
    explicit PresetManagerComponent(PluginPresetInterface& pluginInterface);
    ~PresetManagerComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    PluginPresetInterface& m_pluginInterface; // Reference instead of pointer

    std::unique_ptr<juce::ComboBox> m_presetCombo;
    std::unique_ptr<juce::TextButton> m_saveButton;
    std::unique_ptr<juce::TextButton> m_loadButton;

    void refreshPresetList();
    void loadPresetFromCombo();
    void savePreset();
    void loadPreset();
    void selectPreset(const juce::String& name);

    juce::File getPresetDirectory();
    void ensurePresetDirectoryExists();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManagerComponent)
};

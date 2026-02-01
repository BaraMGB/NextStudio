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

#include "Utilities/ApplicationViewState.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Abstract interface for plugin preset management
 * All plugin components that want to use the PresetManagerComponent
 * must implement this interface
 */
class PluginPresetInterface
{
public:
    virtual ~PluginPresetInterface() = default;

    /**
     * Get the current plugin state as ValueTree
     * This should return the complete state that needs to be saved
     */
    virtual juce::ValueTree getPluginState() = 0;

    /**
     * Restore plugin state from a ValueTree
     * This should properly restore all plugin parameters
     */
    virtual juce::ValueTree getFactoryDefaultState() = 0;
    virtual bool getInitialPresetLoaded() = 0;
    virtual void setInitialPresetLoaded(bool loaded) = 0;
    virtual juce::String getLastLoadedPresetName() = 0;
    virtual void setLastLoadedPresetName(const juce::String &name) = 0;
    virtual void restorePluginState(const juce::ValueTree &state) = 0;

    /**
     * Get the subfolder name for this plugin's presets
     * e.g., "FourOSC", "Reverb", etc.
     */
    virtual juce::String getPresetSubfolder() const = 0;

    /**
     * Get the plugin type name for validation
     * e.g., "FourOSC", "Reverb", etc.
     */
    virtual juce::String getPluginTypeName() const = 0;

    /**
     * Get access to the application view state for preset directory access
     */
    virtual ApplicationViewState &getApplicationViewState() = 0;
};

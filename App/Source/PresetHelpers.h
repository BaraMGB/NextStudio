/*
  ==============================================================================

    PresetHelpers.h
    Created: 24 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginPresetInterface.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

namespace PresetHelpers
{
/**
 * Calculates the full path to the preset directory for a specific plugin type.
 * Uses the ApplicationViewState from the interface to determine the root user folder.
 */
juce::File getPresetDirectory(PluginPresetInterface &interface);

/**
 * Returns the subfolder name for a given plugin instance.
 * Centralizes the mapping logic (e.g., "volume" -> "Volume", "4bandEq" -> "EQ").
 */
juce::String getPluginPresetFolder(te::Plugin &plugin);

/**
 * Tries to load the 'init.nxtpreset' file for the given plugin interface.
 * This should be called immediately after creating a new plugin instance to apply user defaults.
 *
 * @return true if an init preset was found and successfully loaded, false otherwise.
 */
bool tryLoadInitPreset(PluginPresetInterface &interface);
} // namespace PresetHelpers
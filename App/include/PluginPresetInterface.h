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

#include "ApplicationViewState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

class PluginPresetInterface
{
public:
    virtual ~PluginPresetInterface() = default;

    virtual juce::ValueTree getPluginState() = 0;
    virtual juce::ValueTree getFactoryDefaultState() = 0;
    virtual bool getInitialPresetLoaded() = 0;
    virtual void setInitialPresetLoaded(bool loaded) = 0;
    virtual juce::String getLastLoadedPresetName() = 0;
    virtual void setLastLoadedPresetName(const juce::String &name) = 0;
    virtual void restorePluginState(const juce::ValueTree &state) = 0;

    virtual bool applyPresetState(const juce::ValueTree &state)
    {
        if (!isPresetCompatible(state))
            return false;

        restorePluginState(state);
        return true;
    }

    virtual juce::String getPresetSubfolder() const = 0;
    virtual juce::String getPluginTypeName() const = 0;
    virtual ApplicationViewState &getApplicationViewState() = 0;

    /**
     * Returns additional directories that should be searched for compatible presets.
     * Used for legacy compatibility/migrations.
     */
    virtual juce::Array<juce::File> getAdditionalPresetSearchDirectories() const { return {}; }

    /**
     * Cheap filter used for preset list population.
     * Should avoid expensive engine/plugin instantiation work.
     */
    virtual bool isPresetVisibleInList(const juce::ValueTree &state) const
    {
        return state.hasType(juce::Identifier("PLUGIN")) && state.getProperty("type") == getPluginTypeName();
    }

    /**
     * Returns true if the supplied preset state can be applied to this target.
     * May perform deeper validation when the preset is actually loaded.
     */
    virtual bool isPresetCompatible(const juce::ValueTree &state) const
    {
        return isPresetVisibleInList(state);
    }
};

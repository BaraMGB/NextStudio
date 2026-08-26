/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "Utilities.h"

#include <vector>

struct PluginChainSectionSpec
{
    EngineHelpers::PluginChainRole role;
    juce::String title;
};

inline std::vector<PluginChainSectionSpec> getPluginChainSectionSpecs(const te::Track *track)
{
    if (track != nullptr && EngineHelpers::isMidiTrack(*track))
    {
        return {{EngineHelpers::PluginChainRole::midiEffect, "MIDI Plugins"}, {EngineHelpers::PluginChainRole::instrument, "Instrument"}, {EngineHelpers::PluginChainRole::audioEffect, "Audio Effects"}};
    }

    return {{EngineHelpers::PluginChainRole::audioEffect, "Audio Effects"}};
}

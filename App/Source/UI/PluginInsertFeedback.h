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

#include "Utilities/Utilities.h"

namespace UIHelpers
{
inline void showPluginInsertBlockedDialog(EngineHelpers::PluginInsertResult result)
{
    switch (result)
    {
    case EngineHelpers::PluginInsertResult::blockedTrackType:
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Plugin not allowed", "This track only allows audio effect plugins.");
        break;
    case EngineHelpers::PluginInsertResult::blockedInstrumentSlot:
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Instrument slot occupied", "This MIDI track already has an instrument. Remove it first before adding another instrument.");
        break;
    default:
        break;
    }
}
} // namespace UIHelpers

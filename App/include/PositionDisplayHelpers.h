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

#include <juce_core/juce_core.h>
#include <tracktion_core/tracktion_core.h>

#include <optional>
#include <utility>

namespace PositionDisplayHelpers
{
    std::optional<int> parseStrictInt(const juce::String &text);
    std::optional<double> parseStrictDouble(const juce::String &text);

    juce::String formatBpm(double bpm);
    juce::String formatTimeSignature(int numerator, int denominator);
    juce::String formatTime(tracktion::TimePosition position);
    juce::String formatBarsBeatsTicks(const tracktion::tempo::Sequence &tempoSequence,
                                      tracktion::TimePosition position,
                                      int ticksPerQuarterNote);

    std::optional<tracktion::TimePosition> parseTimeValue(const juce::String &text);
    std::optional<tracktion::TimePosition> parseBarsBeatsTicks(const tracktion::tempo::Sequence &tempoSequence,
                                                               const juce::String &text,
                                                               int ticksPerQuarterNote);
    tracktion::TimePosition clampPositionToRange(tracktion::TimePosition position,
                                                 tracktion::TimePosition maximum);
    std::optional<std::pair<int, int>> parseTimeSignatureValue(const juce::String &text);

    int getDenominatorIndex(int denominator);
    int getDenominatorForIndex(int index);
} // namespace PositionDisplayHelpers
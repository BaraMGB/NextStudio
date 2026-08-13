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

#include "PositionDisplayHelpers.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace PositionDisplayHelpers
{
std::optional<int> parseStrictInt(const juce::String &text)
{
    const auto trimmed = text.trim();

    if (trimmed.isEmpty())
        return {};

    const auto utf8 = trimmed.toRawUTF8();
    char *end = nullptr;
    errno = 0;
    const auto value = std::strtol(utf8, &end, 10);

    if (end == utf8 || errno == ERANGE
        || value < std::numeric_limits<int>::min()
        || value > std::numeric_limits<int>::max())
        return {};

    while (*end == ' ' || *end == '\t')
        ++end;

    if (*end != 0)
        return {};

    return static_cast<int>(value);
}

std::optional<double> parseStrictDouble(const juce::String &text)
{
    const auto trimmed = text.trim();

    if (trimmed.isEmpty())
        return {};

    const auto utf8 = trimmed.toRawUTF8();
    char *end = nullptr;
    const auto value = std::strtod(utf8, &end);

    if (end == utf8)
        return {};

    while (*end == ' ' || *end == '\t')
        ++end;

    if (*end != 0)
        return {};

    if (!std::isfinite(value))
        return {};

    return value;
}

juce::String formatBpm(double bpm)
{
    return juce::String(bpm, 2);
}

juce::String formatTimeSignature(int numerator, int denominator)
{
    return juce::String(numerator) + " / " + juce::String(denominator);
}

juce::String formatTime(tracktion::TimePosition position)
{
    auto totalMilliseconds = static_cast<int64_t>(std::llround(tracktion::abs(position).inSeconds() * 1000.0));
    const auto milliseconds = static_cast<int>(totalMilliseconds % 1000);
    const auto totalSeconds = static_cast<int>(totalMilliseconds / 1000);
    const auto seconds = totalSeconds % 60;
    const auto totalMinutes = static_cast<int>(totalSeconds / 60);
    const auto minutes = totalMinutes % 60;
    const auto hours = totalMinutes / 60;

    juce::String result;

    if (position < tracktion::TimePosition())
        result << "-";

    if (hours > 0)
        result << juce::String::formatted("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    else
        result << juce::String::formatted("%d:%02d.%03d", totalMinutes, seconds, milliseconds);

    return result;
}

juce::String formatBarsBeatsTicks(const tracktion::tempo::Sequence &tempoSequence,
                                  tracktion::TimePosition position,
                                  int ticksPerQuarterNote)
{
    const auto barsBeats = tempoSequence.toBarsAndBeats(position);
    const auto ticks = juce::jlimit(0,
                                    ticksPerQuarterNote - 1,
                                    static_cast<int>(barsBeats.getFractionalBeats().inBeats() * ticksPerQuarterNote));

    return juce::String::formatted("%d.%d.%03d",
                                   barsBeats.bars + 1,
                                   barsBeats.getWholeBeats() + 1,
                                   ticks);
}

std::optional<tracktion::TimePosition> parseTimeValue(const juce::String &text)
{
    auto trimmed = text.trim().replaceCharacter(',', '.');
    const auto isNegative = trimmed.startsWithChar('-');

    if (isNegative)
        trimmed = trimmed.substring(1).trim();

    if (trimmed.isEmpty() || trimmed.startsWithChar(':') || trimmed.endsWithChar(':') || trimmed.contains("::"))
        return {};

    auto timeParts = juce::StringArray::fromTokens(trimmed, ":", "");

    if (timeParts.isEmpty() || timeParts.size() > 3)
        return {};

    const auto parsedLastPart = parseStrictDouble(timeParts[timeParts.size() - 1]);
    if (!parsedLastPart.has_value() || *parsedLastPart < 0.0)
        return {};

    double totalSeconds = 0.0;

    if (timeParts.size() == 3)
    {
        const auto parsedHours = parseStrictInt(timeParts[0]);
        const auto parsedMinutes = parseStrictInt(timeParts[1]);

        if (!parsedHours.has_value() || !parsedMinutes.has_value())
            return {};

        if (*parsedHours < 0 || *parsedMinutes < 0 || *parsedMinutes > 59 || *parsedLastPart >= 60.0)
            return {};

        totalSeconds = (*parsedHours * 3600.0) + (*parsedMinutes * 60.0) + *parsedLastPart;
    }
    else if (timeParts.size() == 2)
    {
        const auto parsedMinutes = parseStrictInt(timeParts[0]);

        if (!parsedMinutes.has_value())
            return {};

        if (*parsedMinutes < 0 || *parsedLastPart >= 60.0)
            return {};

        totalSeconds = (*parsedMinutes * 60.0) + *parsedLastPart;
    }
    else
    {
        totalSeconds = *parsedLastPart;
    }

    if (!std::isfinite(totalSeconds) || totalSeconds < 0.0)
        return {};

    return tracktion::TimePosition::fromSeconds(isNegative ? -totalSeconds : totalSeconds);
}

std::optional<tracktion::TimePosition> parseBarsBeatsTicks(const tracktion::tempo::Sequence &tempoSequence,
                                                           const juce::String &text,
                                                           int ticksPerQuarterNote)
{
    const auto trimmed = text.trim();

    if (trimmed.isEmpty() || trimmed.startsWithChar('.') || trimmed.endsWithChar('.') || trimmed.contains(".."))
        return {};

    auto parts = juce::StringArray::fromTokens(trimmed, ".", "");
    if (parts.isEmpty() || parts.size() > 3)
        return {};

    const auto bar = parseStrictInt(parts[0]);
    const auto beat = parts.size() >= 2 ? parseStrictInt(parts[1]) : std::optional<int>(1);
    const auto tick = parts.size() >= 3 ? parseStrictInt(parts[2]) : std::optional<int>(0);

    if (!bar || !beat || !tick || *bar < 1 || *beat < 1 || *tick < 0 || *tick >= ticksPerQuarterNote)
        return {};

    tracktion::tempo::BarsAndBeats barsBeats;
    barsBeats.bars = *bar - 1;
    barsBeats.beats = tracktion::BeatDuration::fromBeats(
        (*beat - 1) + (*tick / static_cast<double>(ticksPerQuarterNote)));
    return tempoSequence.toTime(barsBeats);
}

tracktion::TimePosition clampPositionToRange(tracktion::TimePosition position,
                                             tracktion::TimePosition maximum)
{
    return juce::jlimit(tracktion::TimePosition(), maximum, position);
}

std::optional<std::pair<int, int>> parseTimeSignatureValue(const juce::String &text)
{
    auto parts = juce::StringArray::fromTokens(text.trim(), "/", "");

    if (parts.size() != 2)
        return {};

    const auto numerator = parseStrictInt(parts[0]);
    const auto denominator = parseStrictInt(parts[1]);

    if (!numerator.has_value() || !denominator.has_value())
        return {};

    if (*numerator < 1 || *numerator > 64)
        return {};

    static constexpr int validDenominators[] = {1, 2, 4, 8, 16, 32, 64};

    bool isValidDenominator = false;

    for (const auto candidate : validDenominators)
    {
        if (candidate == *denominator)
        {
            isValidDenominator = true;
            break;
        }
    }

    if (!isValidDenominator)
        return {};

    return std::make_pair(*numerator, *denominator);
}

int getDenominatorIndex(int denominator)
{
    static constexpr int denominators[] = {1, 2, 4, 8, 16, 32, 64};
    constexpr int numDenominators = static_cast<int>(sizeof(denominators) / sizeof(denominators[0]));

    for (int i = 0; i < numDenominators; ++i)
        if (denominators[i] >= denominator)
            return i;

    return numDenominators - 1;
}

int getDenominatorForIndex(int index)
{
    static constexpr int denominators[] = {1, 2, 4, 8, 16, 32, 64};
    constexpr int numDenominators = static_cast<int>(sizeof(denominators) / sizeof(denominators[0]));
    return denominators[juce::jlimit(0, numDenominators - 1, index)];
}
} // namespace PositionDisplayHelpers
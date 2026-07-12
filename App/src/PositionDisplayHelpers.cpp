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

std::optional<tracktion::TimePosition> parseTimeValue(const juce::String &text)
{
    auto trimmed = text.trim();
    const auto isNegative = trimmed.startsWithChar('-');

    if (isNegative)
        trimmed = trimmed.substring(1).trim();

    auto timeAndMilliseconds = juce::StringArray::fromTokens(trimmed, ".", "");

    if (timeAndMilliseconds.size() != 2)
        return {};

    const auto milliseconds = parseStrictInt(timeAndMilliseconds[1]);
    if (!milliseconds.has_value() || *milliseconds < 0 || *milliseconds > 999)
        return {};

    auto timeParts = juce::StringArray::fromTokens(timeAndMilliseconds[0], ":", "");

    if (timeParts.size() != 2 && timeParts.size() != 3)
        return {};

    int hours = 0;
    int minutes = 0;
    int seconds = 0;

    if (timeParts.size() == 3)
    {
        const auto parsedHours = parseStrictInt(timeParts[0]);
        const auto parsedMinutes = parseStrictInt(timeParts[1]);
        const auto parsedSeconds = parseStrictInt(timeParts[2]);

        if (!parsedHours.has_value() || !parsedMinutes.has_value() || !parsedSeconds.has_value())
            return {};

        if (*parsedHours < 0 || *parsedMinutes < 0 || *parsedMinutes > 59 || *parsedSeconds < 0 || *parsedSeconds > 59)
            return {};

        hours = *parsedHours;
        minutes = *parsedMinutes;
        seconds = *parsedSeconds;
    }
    else
    {
        const auto parsedMinutes = parseStrictInt(timeParts[0]);
        const auto parsedSeconds = parseStrictInt(timeParts[1]);

        if (!parsedMinutes.has_value() || !parsedSeconds.has_value())
            return {};

        if (*parsedMinutes < 0 || *parsedSeconds < 0 || *parsedSeconds > 59)
            return {};

        minutes = *parsedMinutes;
        seconds = *parsedSeconds;
    }

    const auto totalSeconds = (hours * 3600.0) + (minutes * 60.0) + seconds + (*milliseconds / 1000.0);

    if (totalSeconds < 0.0)
        return {};

    return tracktion::TimePosition::fromSeconds(isNegative ? -totalSeconds : totalSeconds);
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
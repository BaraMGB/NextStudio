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

#include <iostream>

namespace
{
int g_failures = 0;

#define REQUIRE(cond) \
    do { if (! (cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++g_failures; } } while (0)

#define REQUIRE_EQ(actual, expected) \
    do { if ((actual) != (expected)) { std::cerr << "FAIL: " << #actual << " == " << #expected << " (line " << __LINE__ << ")\n"; ++g_failures; } } while (0)

void testParseStrictInt()
{
    REQUIRE(PositionDisplayHelpers::parseStrictInt("42").has_value());
    REQUIRE_EQ(*PositionDisplayHelpers::parseStrictInt("42"), 42);
    REQUIRE_EQ(*PositionDisplayHelpers::parseStrictInt(" -7 "), -7);
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("abc").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("12a").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("2147483648").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("-2147483649").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictInt("999999999999999999999999").has_value());
}

void testParseStrictDouble()
{
    REQUIRE_EQ(*PositionDisplayHelpers::parseStrictDouble("120.00"), 120.0);
    REQUIRE(! PositionDisplayHelpers::parseStrictDouble("nan").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictDouble("").has_value());
    REQUIRE(! PositionDisplayHelpers::parseStrictDouble("1.2.3").has_value());
}

void testFormatBpm()
{
    REQUIRE_EQ(PositionDisplayHelpers::formatBpm(120.0), juce::String("120.00"));
    REQUIRE_EQ(PositionDisplayHelpers::formatBpm(123.456), juce::String("123.46"));
}

void testFormatTimeSignature()
{
    REQUIRE_EQ(PositionDisplayHelpers::formatTimeSignature(4, 4), juce::String("4 / 4"));
    REQUIRE_EQ(PositionDisplayHelpers::formatTimeSignature(7, 8), juce::String("7 / 8"));
}

void testFormatTime()
{
    REQUIRE_EQ(PositionDisplayHelpers::formatTime(tracktion::TimePosition::fromSeconds(0.0)), juce::String("0:00.000"));
    REQUIRE_EQ(PositionDisplayHelpers::formatTime(tracktion::TimePosition::fromSeconds(5.0)), juce::String("0:05.000"));
    REQUIRE_EQ(PositionDisplayHelpers::formatTime(tracktion::TimePosition::fromSeconds(65.25)), juce::String("1:05.250"));
    REQUIRE(PositionDisplayHelpers::formatTime(tracktion::TimePosition::fromSeconds(3700.0)).startsWith("01:01:40.000"));
}

void testParseTimeValue()
{
    REQUIRE(PositionDisplayHelpers::parseTimeValue("0:05.000").has_value());
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("0:05.000")->inSeconds(), 5.0);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("1:05.250")->inSeconds(), 65.25);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("0:00.500")->inSeconds(), 0.5);

    // Convenient input variants are accepted even when they do not exactly
    // match the display format.
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("0:05.9999")->inSeconds(), 5.9999);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("0:05")->inSeconds(), 5.0);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("5.25")->inSeconds(), 5.25);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("1:02:03.5")->inSeconds(), 3723.5);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeValue("0:05,25")->inSeconds(), 5.25);

    // Invalid component ranges must still be rejected, but a leading minus is accepted.
    REQUIRE(! PositionDisplayHelpers::parseTimeValue("0:60.000").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeValue("0:-5.000").has_value());
    REQUIRE(PositionDisplayHelpers::parseTimeValue("-0:05.000").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeValue("abc").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeValue("1:2:3:4.000").has_value());
}

void testPositionClamping()
{
    const auto parsed = PositionDisplayHelpers::parseTimeValue("-0:05.000");
    REQUIRE(parsed.has_value());
    REQUIRE_EQ(parsed->inSeconds(), -5.0);

    const auto maximum = tracktion::TimePosition::fromSeconds(48.0 * 60.0 * 60.0);
    REQUIRE_EQ(PositionDisplayHelpers::clampPositionToRange(*parsed, maximum).inSeconds(), 0.0);
    REQUIRE_EQ(PositionDisplayHelpers::clampPositionToRange(tracktion::TimePosition::fromSeconds(5.0), maximum).inSeconds(), 5.0);
}

void testParseTimeSignatureValue()
{
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeSignatureValue("4/4")->first, 4);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeSignatureValue("4/4")->second, 4);
    REQUIRE_EQ(PositionDisplayHelpers::parseTimeSignatureValue("7 / 8")->second, 8);

    REQUIRE(! PositionDisplayHelpers::parseTimeSignatureValue("0/4").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeSignatureValue("4/3").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeSignatureValue("65/4").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeSignatureValue("4").has_value());
    REQUIRE(! PositionDisplayHelpers::parseTimeSignatureValue("abc/def").has_value());
}

void testDenominatorHelpers()
{
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorIndex(1), 0);
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorIndex(4), 2);
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorIndex(64), 6);
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorForIndex(0), 1);
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorForIndex(2), 4);
    REQUIRE_EQ(PositionDisplayHelpers::getDenominatorForIndex(6), 64);
}
} // namespace

int main()
{
    testParseStrictInt();
    testParseStrictDouble();
    testFormatBpm();
    testFormatTimeSignature();
    testFormatTime();
    testParseTimeValue();
    testPositionClamping();
    testParseTimeSignatureValue();
    testDenominatorHelpers();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All PositionDisplayHelpers tests passed.\n";
    return 0;
}
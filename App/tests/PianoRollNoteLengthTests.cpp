/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "PianoRollNoteLength.h"

#include <cmath>
#include <iostream>

namespace
{
int g_failures = 0;

#define REQUIRE(cond) \
    do { if (! (cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++g_failures; } } while (0)

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 1.0e-9;
}

void testNoteValuesConvertToBeats()
{
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(1), 4.0));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(2), 2.0));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(4), 1.0));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(8), 0.5));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(16), 0.25));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(32), 0.125));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(64), 0.0625));
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(128), 0.03125));
}

void testModesResolveTheirOwnSource()
{
    REQUIRE(nearlyEqual(PianoRollNoteLength::resolve(PianoRollNoteLengthMode::adaptive,
                                                     4, 0.75, 0.125),
                        0.125));
    REQUIRE(nearlyEqual(PianoRollNoteLength::resolve(PianoRollNoteLengthMode::lastInserted,
                                                     4, 0.75, 0.125),
                        0.75));
    REQUIRE(nearlyEqual(PianoRollNoteLength::resolve(PianoRollNoteLengthMode::fixed,
                                                     32, 0.75, 0.125),
                        0.125));
}

void testInvalidLengthsUseDefault()
{
    REQUIRE(nearlyEqual(PianoRollNoteLength::noteValueToBeats(0),
                        PianoRollNoteLength::defaultLengthBeats));
    REQUIRE(nearlyEqual(PianoRollNoteLength::resolve(PianoRollNoteLengthMode::adaptive,
                                                     16, 1.0, 0.0),
                        PianoRollNoteLength::defaultLengthBeats));
    REQUIRE(nearlyEqual(PianoRollNoteLength::resolve(PianoRollNoteLengthMode::lastInserted,
                                                     16, -1.0, 1.0),
                        PianoRollNoteLength::defaultLengthBeats));
}

void testDrawLengthCannotShrinkBelowSelection()
{
    REQUIRE(nearlyEqual(PianoRollNoteLength::applyMinimum(2.0, 2.125, 0.25), 2.25));
    REQUIRE(nearlyEqual(PianoRollNoteLength::applyMinimum(2.0, 3.0, 0.25), 3.0));
    REQUIRE(nearlyEqual(PianoRollNoteLength::applyMinimum(2.0, 1.0, 0.0), 2.25));
}
} // namespace

int main()
{
    testNoteValuesConvertToBeats();
    testModesResolveTheirOwnSource();
    testInvalidLengthsUseDefault();
    testDrawLengthCannotShrinkBelowSelection();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All PianoRollNoteLength tests passed.\n";
    return 0;
}

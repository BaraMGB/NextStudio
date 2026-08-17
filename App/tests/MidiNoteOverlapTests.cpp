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

#include "MidiNoteOverlap.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
using Interval = MidiNoteOverlap::Interval;

int g_failures = 0;

#define REQUIRE(cond) \
    do { if (! (cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++g_failures; } } while (0)

#define REQUIRE_EQ(actual, expected) \
    do { if ((actual) != (expected)) { std::cerr << "FAIL: " << #actual << " == " << #expected << " (line " << __LINE__ << ")\n"; ++g_failures; } } while (0)

bool nearlyEqual (double a, double b)
{
    return std::abs (a - b) < 1e-9;
}

void requireIntervals (const std::vector<Interval>& actual,
                       const std::vector<Interval>& expected)
{
    if (actual.size() != expected.size())
    {
        std::cerr << "FAIL: interval count " << actual.size() << " == " << expected.size()
                  << " (line " << __LINE__ << ")\n";
        ++g_failures;
        return;
    }

    for (size_t i = 0; i < actual.size(); ++i)
    {
        if (! nearlyEqual (actual[i].startBeat, expected[i].startBeat) ||
            ! nearlyEqual (actual[i].endBeat, expected[i].endBeat))
        {
            std::cerr << "FAIL: interval[" << i << "] = [" << actual[i].startBeat << ", "
                      << actual[i].endBeat << "] == [" << expected[i].startBeat << ", "
                      << expected[i].endBeat << "] (line " << __LINE__ << ")\n";
            ++g_failures;
        }
    }
}

void testNoIntersection()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{2.0, 3.0}}),
                      {{0.0, 1.0}});
}

void testTouchingBoundaries()
{
    // Clear starts exactly where the note ends.
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{1.0, 2.0}}),
                      {{0.0, 1.0}});

    // Clear ends exactly where the note starts.
    requireIntervals (MidiNoteOverlap::subtractIntervals ({1.0, 2.0}, {{0.0, 1.0}}),
                      {{1.0, 2.0}});
}

void testClearContainsNote()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{-1.0, 2.0}}),
                      {});
}

void testExactEquality()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{0.0, 1.0}}),
                      {});
}

void testSplit()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 2.0}, {{0.5, 1.5}}),
                      {{0.0, 0.5}, {1.5, 2.0}});
}

void testTrimEnd()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{0.5, 1.5}}),
                      {{0.0, 0.5}});
}

void testTrimStart()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{-0.5, 0.5}}),
                      {{0.5, 1.0}});
}

void testInsertedRangeWinsAgainstRightNote()
{
    // The inserted range [1, 3) must trim the overlapping right note [2, 4)
    // to [3, 4), never cover or shorten the inserted note itself.
    requireIntervals (MidiNoteOverlap::subtractIntervals ({2.0, 4.0}, {{1.0, 3.0}}),
                      {{3.0, 4.0}});
}

void testMultipleClears()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 4.0}, {{1.0, 2.0}, {3.0, 4.0}}),
                      {{0.0, 1.0}, {2.0, 3.0}});
}

void testOverlappingClears()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 4.0}, {{1.0, 3.0}, {2.0, 4.0}}),
                      {{0.0, 1.0}});
}

void testAdjacentClears()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 4.0}, {{1.0, 2.0}, {2.0, 3.0}}),
                      {{0.0, 1.0}, {3.0, 4.0}});
}

void testClearOutsideNote()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({1.0, 2.0}, {{0.0, 0.5}, {3.0, 4.0}}),
                      {{1.0, 2.0}});
}

void testSubEpsilonPiecesDropped()
{
    // A clear that leaves a sub-epsilon head removes the whole note.
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{0.000005, 1.0}}),
                      {});

    // A clear that leaves a sub-epsilon tail removes the whole note.
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{0.0, 0.999995}}),
                      {});
}

void testEmptyInputs()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 0.0}, {{0.0, 1.0}}),
                      {});

    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {}),
                      {{0.0, 1.0}});

    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 1.0}, {{0.5, 0.5}}),
                      {{0.0, 1.0}});
}

void testFullCoverageByMultipleClears()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 2.0}, {{0.0, 1.0}, {1.0, 2.0}}),
                      {});
}

void testUnsortedClears()
{
    requireIntervals (MidiNoteOverlap::subtractIntervals ({0.0, 4.0}, {{3.0, 4.0}, {1.0, 2.0}}),
                      {{0.0, 1.0}, {2.0, 3.0}});
}
} // namespace

int main()
{
    testNoIntersection();
    testTouchingBoundaries();
    testClearContainsNote();
    testExactEquality();
    testSplit();
    testTrimEnd();
    testTrimStart();
    testInsertedRangeWinsAgainstRightNote();
    testMultipleClears();
    testOverlappingClears();
    testAdjacentClears();
    testClearOutsideNote();
    testSubEpsilonPiecesDropped();
    testEmptyInputs();
    testFullCoverageByMultipleClears();
    testUnsortedClears();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All MidiNoteOverlap tests passed.\n";
    return 0;
}

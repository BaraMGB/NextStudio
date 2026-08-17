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

#include "MidiPendingPaste.h"

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

void testInactiveStateDoesNothing()
{
    MidiPendingPaste::State state;
    REQUIRE(state.confirm().action == MidiPendingPaste::Action::none);
    REQUIRE(state.finishOnDeselect().action == MidiPendingPaste::Action::none);
    REQUIRE(state.cancel().action == MidiPendingPaste::Action::none);
    REQUIRE(!state.nudge(1.0, 1));
}

void testDeselectWithoutMovementCancels()
{
    MidiPendingPaste::State state;
    state.begin();

    const auto result = state.finishOnDeselect();
    REQUIRE(result.action == MidiPendingPaste::Action::cancel);
    REQUIRE(nearlyEqual(result.beatDelta, 0.0));
    REQUIRE(result.pitchDelta == 0);
    REQUIRE(!state.isActive());
}

void testDeselectAfterMovementCommits()
{
    MidiPendingPaste::State state;
    state.begin();
    REQUIRE(state.nudge(0.25, 2));

    const auto result = state.finishOnDeselect();
    REQUIRE(result.action == MidiPendingPaste::Action::commit);
    REQUIRE(nearlyEqual(result.beatDelta, 0.25));
    REQUIRE(result.pitchDelta == 2);
    REQUIRE(!state.isActive());
}

void testEnterConfirmsWithoutMovement()
{
    MidiPendingPaste::State state;
    state.begin();

    const auto result = state.confirm();
    REQUIRE(result.action == MidiPendingPaste::Action::commit);
    REQUIRE(nearlyEqual(result.beatDelta, 0.0));
    REQUIRE(result.pitchDelta == 0);
    REQUIRE(!state.isActive());
}

void testEnterConfirmsAccumulatedMovement()
{
    MidiPendingPaste::State state;
    state.begin();
    state.nudge(0.25, 1);
    state.nudge(0.5, -2);

    const auto result = state.confirm();
    REQUIRE(result.action == MidiPendingPaste::Action::commit);
    REQUIRE(nearlyEqual(result.beatDelta, 0.75));
    REQUIRE(result.pitchDelta == -1);
}

void testEscapeCancelsMovement()
{
    MidiPendingPaste::State state;
    state.begin();
    state.nudge(1.0, 12);

    const auto result = state.cancel();
    REQUIRE(result.action == MidiPendingPaste::Action::cancel);
    REQUIRE(nearlyEqual(result.beatDelta, 1.0));
    REQUIRE(result.pitchDelta == 12);
    REQUIRE(!state.isActive());
}

void testZeroNudgeDoesNotMarkMovement()
{
    MidiPendingPaste::State state;
    state.begin();
    REQUIRE(!state.nudge(0.0, 0));
    REQUIRE(!state.hasMoved());
    REQUIRE(state.finishOnDeselect().action == MidiPendingPaste::Action::cancel);
}

void testBeginResetsExistingState()
{
    MidiPendingPaste::State state;
    state.begin();
    state.nudge(2.0, 7);
    state.begin();

    REQUIRE(state.isActive());
    REQUIRE(!state.hasMoved());
    REQUIRE(nearlyEqual(state.getBeatDelta(), 0.0));
    REQUIRE(state.getPitchDelta() == 0);
}

void testReturningToOriginStillCountsAsMovement()
{
    MidiPendingPaste::State state;
    state.begin();
    state.nudge(1.0, 2);
    state.nudge(-1.0, -2);

    const auto result = state.finishOnDeselect();
    REQUIRE(result.action == MidiPendingPaste::Action::commit);
    REQUIRE(nearlyEqual(result.beatDelta, 0.0));
    REQUIRE(result.pitchDelta == 0);
}
} // namespace

int main()
{
    testInactiveStateDoesNothing();
    testDeselectWithoutMovementCancels();
    testDeselectAfterMovementCommits();
    testEnterConfirmsWithoutMovement();
    testEnterConfirmsAccumulatedMovement();
    testEscapeCancelsMovement();
    testZeroNudgeDoesNotMarkMovement();
    testBeginResetsExistingState();
    testReturningToOriginStillCountsAsMovement();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All MidiPendingPaste tests passed.\n";
    return 0;
}

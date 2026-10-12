/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "PianoRollMouseActions.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

void testPrimaryClicksReachEveryTool()
{
    REQUIRE(resolvePianoRollMouseDownAction(false, false, true) == PianoRollMouseDownAction::forwardToTool);
    REQUIRE(resolvePianoRollMouseDownAction(false, true, true) == PianoRollMouseDownAction::forwardToTool);
    REQUIRE(resolvePianoRollMouseDownAction(false, false, false) == PianoRollMouseDownAction::forwardToTool);
    REQUIRE(resolvePianoRollMouseDownAction(false, true, false) == PianoRollMouseDownAction::forwardToTool);
}

void testSecondaryClickDeletesOnlyAHitInEnabledTools()
{
    REQUIRE(resolvePianoRollMouseDownAction(true, true, true) == PianoRollMouseDownAction::deleteHitNote);
    REQUIRE(resolvePianoRollMouseDownAction(true, false, true) == PianoRollMouseDownAction::ignore);
}

void testOtherToolsRetainSecondaryClicks()
{
    REQUIRE(resolvePianoRollMouseDownAction(true, true, false) == PianoRollMouseDownAction::forwardToTool);
    REQUIRE(resolvePianoRollMouseDownAction(true, false, false) == PianoRollMouseDownAction::forwardToTool);
}
} // namespace

int main()
{
    testPrimaryClicksReachEveryTool();
    testSecondaryClickDeletesOnlyAHitInEnabledTools();
    testOtherToolsRetainSecondaryClicks();

    if (failures != 0)
    {
        std::cerr << failures << " piano roll mouse action test(s) failed.\n";
        return 1;
    }

    std::cout << "All PianoRollMouseActions tests passed.\n";
    return 0;
}

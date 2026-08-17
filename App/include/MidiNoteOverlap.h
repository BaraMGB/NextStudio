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

#include <vector>

namespace MidiNoteOverlap
{
    /** A half-open beat interval [startBeat, endBeat). */
    struct Interval
    {
        double startBeat = 0.0;
        double endBeat = 0.0;

        double length() const { return endBeat - startBeat; }
        bool isEmpty() const { return endBeat <= startBeat; }
    };

    /** Subtracts all clear intervals from the note interval.

        Returns the remaining intervals sorted by start beat. Pieces whose
        length does not exceed epsilon are dropped, matching the behaviour of
        the Piano Roll's overlap clearing.

        The function is pure and has no Tracktion or JUCE dependency so it can
        be unit-tested in isolation.
    */
    std::vector<Interval> subtractIntervals (Interval note,
                                             const std::vector<Interval>& clears,
                                             double epsilon = 0.00001);
} // namespace MidiNoteOverlap

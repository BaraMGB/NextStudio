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

#include <algorithm>

namespace MidiNoteOverlap
{
    std::vector<Interval> subtractIntervals (Interval note,
                                             const std::vector<Interval>& clears,
                                             double epsilon)
    {
        if (note.isEmpty())
            return {};

        // Clamp every clear interval to the note and keep only real overlaps.
        std::vector<Interval> relevant;
        for (const auto& c : clears)
        {
            if (c.isEmpty())
                continue;

            const double s = std::max (note.startBeat, c.startBeat);
            const double e = std::min (note.endBeat, c.endBeat);

            if (e - s > epsilon)
                relevant.push_back ({s, e});
        }

        if (relevant.empty())
            return {note};

        // Sort by start and merge overlapping or adjacent clear intervals.
        std::sort (relevant.begin(), relevant.end(),
                   [] (const Interval& a, const Interval& b) { return a.startBeat < b.startBeat; });

        std::vector<Interval> merged;
        for (const auto& c : relevant)
        {
            if (merged.empty() || c.startBeat > merged.back().endBeat + epsilon)
                merged.push_back (c);
            else
                merged.back().endBeat = std::max (merged.back().endBeat, c.endBeat);
        }

        // Walk the merged clears and emit the remaining pieces.
        std::vector<Interval> result;
        double cursor = note.startBeat;

        for (const auto& c : merged)
        {
            if (c.startBeat - cursor > epsilon)
                result.push_back ({cursor, c.startBeat});

            cursor = std::max (cursor, c.endBeat);
        }

        if (note.endBeat - cursor > epsilon)
            result.push_back ({cursor, note.endBeat});

        return result;
    }
} // namespace MidiNoteOverlap

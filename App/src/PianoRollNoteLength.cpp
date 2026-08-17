/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "PianoRollNoteLength.h"

#include <algorithm>

namespace PianoRollNoteLength
{
namespace
{
double validOrDefault(double length)
{
    return length > 0.0 ? length : defaultLengthBeats;
}
} // namespace

double noteValueToBeats(int denominator)
{
    return denominator > 0 ? 4.0 / denominator : defaultLengthBeats;
}

double resolve(PianoRollNoteLengthMode mode,
               int denominator,
               double lastInsertedBeats,
               double adaptiveBeats)
{
    switch (mode)
    {
    case PianoRollNoteLengthMode::adaptive:
        return validOrDefault(adaptiveBeats);
    case PianoRollNoteLengthMode::lastInserted:
        return validOrDefault(lastInsertedBeats);
    case PianoRollNoteLengthMode::fixed:
        return noteValueToBeats(denominator);
    }

    return defaultLengthBeats;
}

double applyMinimum(double startBeat, double attemptedEndBeat, double minimumLengthBeats)
{
    return std::max(attemptedEndBeat, startBeat + validOrDefault(minimumLengthBeats));
}
} // namespace PianoRollNoteLength

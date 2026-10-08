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
#include <cmath>

namespace PianoRollNoteLength
{
namespace
{
double validOrDefault(double length)
{
    return std::isfinite(length) && length > 0.0 ? length : defaultLengthBeats;
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

double constrainEnd(double startBeat, double attemptedEndBeat)
{
    return std::isfinite(attemptedEndBeat) ? std::max(attemptedEndBeat, startBeat + minimumLengthBeats)
                                         : startBeat + minimumLengthBeats;
}
} // namespace PianoRollNoteLength

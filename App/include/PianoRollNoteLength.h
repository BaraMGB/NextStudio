/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

enum class PianoRollNoteLengthMode
{
    adaptive,
    lastInserted,
    fixed
};

namespace PianoRollNoteLength
{
constexpr double defaultLengthBeats = 0.25;

double noteValueToBeats(int denominator);
double resolve(PianoRollNoteLengthMode mode,
               int denominator,
               double lastInsertedBeats,
               double adaptiveBeats);
double applyMinimum(double startBeat, double attemptedEndBeat, double minimumLengthBeats);
} // namespace PianoRollNoteLength

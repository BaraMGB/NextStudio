/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

/** A model-independent destination for a selected note property edit. */
struct MidiNotePropertyEdit
{
    te::MidiClip *clip{};
    te::MidiNote *sourceNote{};
    tracktion::BeatPosition startBeat;
    tracktion::BeatDuration length;
    int noteNumber{};
    int velocity{};
    juce::ValueTree sourceState;
};

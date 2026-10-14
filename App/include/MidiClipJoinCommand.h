/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

namespace MidiClipJoining
{
struct Selection
{
    bool valid = false;
    juce::String error;
    juce::Array<te::MidiClip *> clips;
    te::ClipTrack *track = nullptr;
};

struct Result
{
    bool succeeded = false;
    juce::String error;
    te::MidiClip::Ptr mergedClip;
};

/** Validates that the selected clips form one adjacent MIDI-clip block on one track. */
Selection inspectSelection(const te::SelectionManager &);

/** Joins the validated selection as one undoable edit using Tracktion's MIDI merge semantics. */
Result joinSelectedClips(te::Edit &, te::SelectionManager &);

} // namespace MidiClipJoining

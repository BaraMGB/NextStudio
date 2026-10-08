#include "MidiNoteCreation.h"
#include "MidiNoteOverlap.h"
#include "PianoRollNoteLength.h"
#include <algorithm>
#include <cmath>

namespace MidiNoteCreation
{
void clear(const tracktion_engine::MidiClip* clip, int pitch, const juce::Array<tracktion::BeatRange>& ranges,
           const BeforeRemoval& beforeRemoval)
{
    if (clip == nullptr || ranges.isEmpty())
        return;
    std::vector<MidiNoteOverlap::Interval> clears;
    for (const auto& range : ranges)
        if (!range.isEmpty())
            clears.push_back({range.getStart().inBeats(), range.getEnd().inBeats()});
    if (clears.empty())
        return;
    auto& undo = clip->edit.getUndoManager();
    auto& sequence = clip->getSequence();
    const auto originalNotes = sequence.getNotes();
    for (auto* note : originalNotes)
    {
        if (note->getNoteNumber() != pitch)
            continue;
        const auto remaining = MidiNoteOverlap::subtractIntervals({note->getStartBeat().inBeats(), note->getEndBeat().inBeats()}, clears);
        if (remaining.empty())
        {
            if (beforeRemoval)
                beforeRemoval(note);
            sequence.removeNote(*note, &undo);
            continue;
        }
        note->setStartAndLength(tracktion::BeatPosition::fromBeats(remaining.front().startBeat),
                               tracktion::BeatDuration::fromBeats(remaining.front().length()), &undo);
        for (size_t i = 1; i < remaining.size(); ++i)
            sequence.addNote(tracktion_engine::MidiNote(tracktion_engine::MidiNote::createNote(*note,
                tracktion::BeatPosition::fromBeats(remaining[i].startBeat), tracktion::BeatDuration::fromBeats(remaining[i].length()))), &undo);
    }
}
tracktion_engine::MidiNote* add(const tracktion_engine::MidiClip* clip, int pitch, double start, double length, int velocity,
                               const BeforeRemoval& beforeRemoval, const std::function<void(double)>& rememberLength)
{
    if (clip == nullptr || !clip->state.getParent().isValid() || !std::isfinite(start)
        || !std::isfinite(length) || length <= 0)
        return nullptr;
    start = std::max(0.0, start);
    length = std::max(PianoRollNoteLength::minimumLengthBeats, length);
    auto& undo = clip->edit.getUndoManager();
    undo.beginNewTransaction("Add MIDI Note");
    juce::Array<tracktion::BeatRange> ranges;
    ranges.add({tracktion::BeatPosition::fromBeats(start), tracktion::BeatDuration::fromBeats(length)});
    clear(clip, pitch, ranges, beforeRemoval);
    auto* note = clip->getSequence().addNote(pitch, tracktion::BeatPosition::fromBeats(start),
                                            tracktion::BeatDuration::fromBeats(length), velocity, 111, &undo);
    if (note != nullptr && rememberLength)
        rememberLength(note->getLengthBeats().inBeats());
    return note;
}
}

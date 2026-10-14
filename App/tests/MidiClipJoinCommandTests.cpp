/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "MidiClipJoinCommand.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace te = tracktion_engine;
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}

bool closeTo(double actual, double expected, double tolerance = 1.0e-7)
{
    return std::abs(actual - expected) < tolerance;
}

struct Fixture
{
    Fixture()
        : engine("NextStudioMidiClipJoinTests"),
          edit(te::Edit::createSingleTrackEdit(engine)),
          selection(engine)
    {
        track = te::getFirstAudioTrack(*edit);
        require(track != nullptr, "initial track missing");
    }

    te::MidiClip::Ptr add(te::AudioTrack &target, double startBeat, double endBeat,
                          const juce::String &name)
    {
        auto clip = target.insertMIDIClip(
            name,
            {edit->tempoSequence.toTime(tracktion::BeatPosition::fromBeats(startBeat)),
             edit->tempoSequence.toTime(tracktion::BeatPosition::fromBeats(endBeat))},
            nullptr);
        require(clip != nullptr, "MIDI clip insertion failed");
        return clip;
    }

    te::MidiClip::Ptr add(double startBeat, double endBeat, const juce::String &name)
    {
        return add(*track, startBeat, endBeat, name);
    }

    void select(std::initializer_list<te::MidiClip *> clips)
    {
        selection.deselectAll();
        for (auto *clip : clips)
            selection.addToSelection(clip);
    }

    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    te::SelectionManager selection;
    te::AudioTrack *track = nullptr;
};

void testSelectionMustBeOneAdjacentBlock()
{
    Fixture f;
    auto first = f.add(0.0, 1.0, "first");
    auto middle = f.add(2.0, 3.0, "middle");
    auto last = f.add(4.0, 5.0, "last");

    f.select({first.get(), last.get()});
    auto nonAdjacent = MidiClipJoining::inspectSelection(f.selection);
    require(!nonAdjacent.valid, "non-neighbouring clips were accepted");

    f.select({first.get(), middle.get(), last.get()});
    auto adjacent = MidiClipJoining::inspectSelection(f.selection);
    require(adjacent.valid, "an adjacent block of more than two clips was rejected");
    require(adjacent.clips.size() == 3, "the adjacent block lost clips");
}

void testSelectionMustStayOnOneTrack()
{
    Fixture f;
    f.edit->ensureNumberOfAudioTracks(2);
    auto *secondTrack = te::getAudioTracks(*f.edit).getLast();
    require(secondTrack != nullptr && secondTrack != f.track, "second track missing");

    auto first = f.add(0.0, 1.0, "first");
    auto second = f.add(*secondTrack, 1.0, 2.0, "second");
    f.select({first.get(), second.get()});

    require(!MidiClipJoining::inspectSelection(f.selection).valid,
            "clips on different tracks were accepted");
}

void testSelectionRejectsDifferentPlaybackSettings()
{
    Fixture f;
    auto first = f.add(0.0, 1.0, "first");
    auto second = f.add(1.0, 2.0, "second");

    first->setMidiChannel(te::MidiChannel(1));
    second->setMidiChannel(te::MidiChannel(2));
    f.select({first.get(), second.get()});

    const auto selection = MidiClipJoining::inspectSelection(f.selection);
    require(!selection.valid, "different MIDI channels were accepted");
    require(selection.error.containsIgnoreCase("channels"),
            "different MIDI channels did not produce a useful error");
    require(f.track->getClips().size() == 2, "validation changed the source clips");
}

void testJoinUsesTempoAwareSourceOffset()
{
    Fixture f;
    f.edit->tempoSequence.getTempo(0)->setBpm(60.0);
    f.edit->tempoSequence.insertTempo(tracktion::BeatPosition::fromBeats(4.0), 120.0, 0.0f);

    auto offsetClip = f.add(6.0, 8.0, "offset");
    auto followingClip = f.add(8.0, 9.0, "following");
    offsetClip->setOffset(tracktion::TimeDuration::fromSeconds(2.0));
    offsetClip->getSequence().addNote(60,
                                      tracktion::BeatPosition::fromBeats(3.5),
                                      tracktion::BeatDuration::fromBeats(0.25),
                                      100, 0, nullptr);
    followingClip->getSequence().addNote(62,
                                         tracktion::BeatPosition::fromBeats(0.25),
                                         tracktion::BeatDuration::fromBeats(0.25),
                                         100, 0, nullptr);

    f.select({offsetClip.get(), followingClip.get()});
    auto result = MidiClipJoining::joinSelectedClips(*f.edit, f.selection);
    require(result.succeeded && result.mergedClip != nullptr,
            "joining clips across a tempo change failed");

    const auto &notes = result.mergedClip->getSequence().getNotes();
    require(notes.size() == 2, "tempo-aware source trimming lost a visible note");
    require(notes[0]->getNoteNumber() == 60
                && closeTo(notes[0]->getStartBeat().inBeats(), 0.5),
            "source offset crossing a tempo change was converted at one endpoint tempo");
    require(notes[1]->getNoteNumber() == 62
                && closeTo(notes[1]->getStartBeat().inBeats(), 2.25),
            "following clip moved while joining across a tempo change");
}

void testJoinExpandsLoopsTrimsOverhangsAndUsesOneUndoStep()
{
    Fixture f;
    auto looped = f.add(0.0, 2.0, "looped");
    auto overhanging = f.add(3.0, 5.0, "overhanging");
    auto finalClip = f.add(5.0, 6.0, "final");

    for (auto *clip : {looped.get(), overhanging.get(), finalClip.get()})
    {
        clip->setMidiChannel(te::MidiChannel(4));
        clip->setMPEMode(true);
        clip->getQuantisation().setType("1/8 beat");
        clip->getQuantisation().setProportion(0.42f);
        clip->getQuantisation().setIsQuantisingNoteOffs(false);
        clip->setGrooveTemplate("Test Groove");
        clip->setGrooveStrength(0.37f);
        clip->setVolumeDb(-6.0f);
        clip->setMuted(true);
        clip->setSendingBankChanges(false);
    }

    looped->getSequence().addNote(60,
                                  tracktion::BeatPosition::fromBeats(0.75),
                                  tracktion::BeatDuration::fromBeats(0.5),
                                  100, 0, nullptr);
    looped->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0.0),
                               tracktion::BeatPosition::fromBeats(1.0)});

    overhanging->getSequence().addNote(62,
                                       tracktion::BeatPosition::fromBeats(0.0),
                                       tracktion::BeatDuration::fromBeats(3.0),
                                       100, 0, nullptr);
    finalClip->getSequence().addNote(64,
                                     tracktion::BeatPosition::fromBeats(0.25),
                                     tracktion::BeatDuration::fromBeats(0.25),
                                     100, 0, nullptr);

    f.select({looped.get(), overhanging.get(), finalClip.get()});
    looped = nullptr;
    overhanging = nullptr;
    finalClip = nullptr;

    auto &undoManager = f.edit->getUndoManager();
    undoManager.clearUndoHistory();

    auto result = MidiClipJoining::joinSelectedClips(*f.edit, f.selection);
    require(result.succeeded && result.mergedClip != nullptr, "joining valid clips failed");
    require(f.track->getClips().size() == 1, "source clips were not replaced by one clip");
    require(!result.mergedClip->isLooping(), "joined clip retained a loop");
    require(result.mergedClip->getMidiChannel() == te::MidiChannel(4),
            "joined clip lost the common MIDI channel");
    require(result.mergedClip->getMPEMode(), "joined clip lost the common MPE mode");
    require(result.mergedClip->getQuantisation().getType(false) == "1/8 beat"
                && closeTo(result.mergedClip->getQuantisation().getProportion(), 0.42f)
                && !result.mergedClip->getQuantisation().isQuantisingNoteOffs(),
            "joined clip lost the common quantisation settings");
    require(result.mergedClip->getGrooveTemplate() == "Test Groove"
                && closeTo(result.mergedClip->getGrooveStrength(), 0.37f),
            "joined clip lost the common groove settings");
    require(closeTo(result.mergedClip->getVolumeDb(), -6.0f),
            "joined clip lost the common volume");
    require(result.mergedClip->isMuted(), "joined clip lost the common mute state");
    require(!result.mergedClip->isSendingBankChanges(),
            "joined clip lost the common bank-change setting");
    require(closeTo(result.mergedClip->getStartBeat().inBeats(), 0.0),
            "joined clip has the wrong start");
    require(closeTo(result.mergedClip->getEndBeat().inBeats(), 6.0),
            "joined clip has the wrong end");

    const auto &notes = result.mergedClip->getSequence().getNotes();
    require(notes.size() == 4, "loop repetitions were not expanded into the joined clip");
    require(notes[0]->getNoteNumber() == 60
                && closeTo(notes[0]->getStartBeat().inBeats(), 0.75)
                && closeTo(notes[0]->getLengthBeats().inBeats(), 0.25),
            "first loop repetition was not trimmed at its boundary");
    require(notes[1]->getNoteNumber() == 60
                && closeTo(notes[1]->getStartBeat().inBeats(), 1.75)
                && closeTo(notes[1]->getLengthBeats().inBeats(), 0.25),
            "second loop repetition was not expanded and trimmed");
    require(notes[2]->getNoteNumber() == 62
                && closeTo(notes[2]->getStartBeat().inBeats(), 3.0)
                && closeTo(notes[2]->getLengthBeats().inBeats(), 2.0),
            "note overhang was not trimmed to its source clip");
    require(notes[3]->getNoteNumber() == 64
                && closeTo(notes[3]->getStartBeat().inBeats(), 5.25),
            "the final neighbouring clip was not merged");

    const auto selected = f.selection.getItemsOfType<te::MidiClip>();
    require(selected.size() == 1 && selected.getFirst() == result.mergedClip.get(),
            "joined clip was not selected exclusively");

    result.mergedClip = nullptr;
    require(undoManager.undo(), "joining could not be undone");
    require(f.track->getClips().size() == 3, "one undo did not restore all source clips");
    require(!undoManager.canUndo(), "joining created more than one undo transaction");
}

void run()
{
    testSelectionMustBeOneAdjacentBlock();
    testSelectionMustStayOnOneTrack();
    testSelectionRejectsDifferentPlaybackSettings();
    testJoinUsesTempoAwareSourceOffset();
    testJoinExpandsLoopsTrimsOverhangsAndUsesOneUndoStep();
}
} // namespace

int main()
{
    try
    {
        run();
        std::cout << "All MIDI clip join tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "MIDI clip join test failed: " << error.what() << "\n";
        return 1;
    }
}

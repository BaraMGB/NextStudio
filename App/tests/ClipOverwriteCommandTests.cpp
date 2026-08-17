/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "ClipOverwriteCommand.h"

#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

using namespace tracktion;

bool closeTo(double actual, double expected)
{
    return std::abs(actual - expected) < 1.0e-9;
}

struct Fixture
{
    Fixture()
        : engine("NextStudioClipOverwriteTests"),
          edit(te::Edit::createSingleTrackEdit(engine)),
          selection(engine)
    {
        track = te::getFirstAudioTrack(*edit);
        track->state.setProperty(::IDs::isMidiTrack, true, nullptr);
    }

    te::MidiClip::Ptr add(double start, double end, const juce::String &name = "clip")
    {
        auto clip = track->insertMIDIClip(name,
                                          {TimePosition::fromSeconds(start), TimePosition::fromSeconds(end)},
                                          nullptr);
        REQUIRE(clip != nullptr);
        return clip;
    }

    ClipEditing::Placement move(te::Clip::Ptr clip, double start, double end)
    {
        ClipEditing::Placement p;
        p.mode = ClipEditing::PlacementMode::move;
        p.source = clip;
        p.destination = track;
        p.finalPosition = {{TimePosition::fromSeconds(start), TimePosition::fromSeconds(end)}, clip->getPosition().offset};
        return p;
    }

    ClipEditing::Placement copy(te::Clip::Ptr clip, double start, double end)
    {
        auto p = move(clip, start, end);
        p.mode = ClipEditing::PlacementMode::copy;
        return p;
    }

    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    te::SelectionManager selection;
    te::AudioTrack *track = nullptr;
};

void testInsertSplitsVictim()
{
    Fixture f;
    f.add(0.0, 10.0, "victim");

    ClipEditing::Placement insert;
    insert.mode = ClipEditing::PlacementMode::insertMidi;
    insert.destination = f.track;
    insert.finalPosition = {{TimePosition::fromSeconds(3.0), TimePosition::fromSeconds(7.0)}, {}};
    insert.name = "winner";

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {insert});
    REQUIRE(result.succeeded);
    REQUIRE(f.track->getClips().size() == 3);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
    REQUIRE(f.selection.getItemsOfType<te::Clip>().size() == 1);
    REQUIRE(f.selection.getItemsOfType<te::Clip>().getFirst() == result.clips.getFirst().get());

    bool left = false, winner = false, right = false;
    for (auto *clip : f.track->getClips())
    {
        const auto start = clip->getPosition().getStart().inSeconds();
        const auto end = clip->getPosition().getEnd().inSeconds();
        left |= closeTo(start, 0.0) && closeTo(end, 3.0);
        winner |= closeTo(start, 3.0) && closeTo(end, 7.0);
        right |= closeTo(start, 7.0) && closeTo(end, 10.0);
    }
    REQUIRE(left && winner && right);
}

void testMoveKeepsIdentityAndTrimsDestination()
{
    Fixture f;
    auto moving = f.add(0.0, 2.0, "moving");
    f.add(4.0, 8.0, "victim");
    const auto originalID = moving->itemID;

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.move(moving, 5.0, 7.0)});
    REQUIRE(result.succeeded);
    REQUIRE(result.clips.size() == 1);
    REQUIRE(result.clips.getFirst()->itemID == originalID);
    REQUIRE(result.clips.getFirst().get() == moving.get());
    REQUIRE(f.track->getClips().size() == 3);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testCopyOnSelfReplacesOriginal()
{
    Fixture f;
    auto source = f.add(0.0, 2.0);
    const auto sourceID = source->itemID;

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.copy(source, 0.0, 2.0)});
    REQUIRE(result.succeeded);
    REQUIRE(f.track->getClips().size() == 1);
    REQUIRE(result.clips.getFirst()->itemID != sourceID);
    REQUIRE(te::findClipForID(*f.edit, sourceID) == nullptr);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testMovingBlockProtectsAllSources()
{
    Fixture f;
    auto first = f.add(0.0, 2.0, "first");
    auto second = f.add(2.0, 4.0, "second");
    f.add(4.0, 10.0, "victim");

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.move(first, 4.0, 6.0),
                                                f.move(second, 6.0, 8.0)});
    REQUIRE(result.succeeded);
    REQUIRE(f.track->getClips().size() == 3);
    REQUIRE(closeTo(first->getPosition().getStart().inSeconds(), 4.0));
    REQUIRE(closeTo(second->getPosition().getStart().inSeconds(), 6.0));
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testCrossTrackMoveKeepsIdentity()
{
    Fixture f;
    f.edit->ensureNumberOfAudioTracks(2);
    auto *destination = te::getAudioTracks(*f.edit)[1];
    destination->state.setProperty(::IDs::isMidiTrack, true, nullptr);
    auto moving = f.add(0.0, 2.0);
    auto victim = destination->insertMIDIClip("victim",
                                              {TimePosition::fromSeconds(1.0), TimePosition::fromSeconds(5.0)},
                                              nullptr);
    REQUIRE(victim != nullptr);
    const auto originalID = moving->itemID;

    auto placement = f.move(moving, 2.0, 4.0);
    placement.destination = destination;
    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {placement});

    REQUIRE(result.succeeded);
    REQUIRE(result.clips.getFirst()->itemID == originalID);
    REQUIRE(moving->getClipTrack() == destination);
    REQUIRE(!ClipEditing::hasOverlaps(*destination));
}

void testWinnerConflictDoesNothing()
{
    Fixture f;
    auto first = f.add(0.0, 2.0);
    auto second = f.add(3.0, 5.0);

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.move(first, 6.0, 9.0),
                                                f.move(second, 8.0, 10.0)});
    REQUIRE(!result.succeeded);
    REQUIRE(closeTo(first->getPosition().getStart().inSeconds(), 0.0));
    REQUIRE(closeTo(second->getPosition().getStart().inSeconds(), 3.0));
}

void testStateInsertionAndSourceRemoval()
{
    Fixture f;
    auto source = f.add(1.0, 5.0, "source");

    ClipEditing::Placement placement;
    placement.mode = ClipEditing::PlacementMode::insertState;
    placement.state = source->state.createCopy();
    placement.destination = f.track;
    placement.finalPosition = {{TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)},
                               source->getPosition().offset + TimeDuration::fromSeconds(1.0)};
    placement.name = source->getName();

    ClipEditing::Options options;
    options.sourceRemovals.push_back({source,
                                      {TimePosition::fromSeconds(2.0), TimePosition::fromSeconds(4.0)}});
    options.destinationRemovals.push_back({f.track,
                                           {TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)}});

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {placement}, options);
    REQUIRE(result.succeeded);
    REQUIRE(f.track->getClips().size() == 3);
    REQUIRE(closeTo(result.clips.getFirst()->getPosition().getStart().inSeconds(), 6.0));
    REQUIRE(closeTo(result.clips.getFirst()->getPosition().getEnd().inSeconds(), 8.0));
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testMultipleRemovalMasks()
{
    Fixture f;
    auto victim = f.add(0.0, 10.0);

    ClipEditing::Options options;
    options.sourceRemovals.push_back({victim, {TimePosition::fromSeconds(2.0), TimePosition::fromSeconds(3.0)}});
    options.sourceRemovals.push_back({victim, {TimePosition::fromSeconds(5.0), TimePosition::fromSeconds(6.0)}});

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {}, options);
    REQUIRE(result.succeeded);
    REQUIRE(f.track->getClips().size() == 3);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testUndoRedoIsAtomic()
{
    Fixture f;
    auto moving = f.add(0.0, 2.0);
    f.add(4.0, 8.0);
    f.edit->getUndoManager().clearUndoHistory();

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.move(moving, 5.0, 7.0)});
    REQUIRE(result.succeeded);
    REQUIRE(closeTo(moving->getPosition().getStart().inSeconds(), 5.0));

    REQUIRE(f.edit->getUndoManager().undo());
    auto *restored = te::findClipForID(*f.edit, moving->itemID);
    REQUIRE(restored != nullptr);
    REQUIRE(closeTo(restored->getPosition().getStart().inSeconds(), 0.0));
    REQUIRE(f.edit->getUndoManager().redo());
    REQUIRE(closeTo(te::findClipForID(*f.edit, moving->itemID)->getPosition().getStart().inSeconds(), 5.0));
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testInsertSplitsVictim();
    testMoveKeepsIdentityAndTrimsDestination();
    testCopyOnSelfReplacesOriginal();
    testMovingBlockProtectsAllSources();
    testCrossTrackMoveKeepsIdentity();
    testWinnerConflictDoesNothing();
    testStateInsertionAndSourceRemoval();
    testMultipleRemovalMasks();
    testUndoRedoIsAtomic();

    if (failures != 0)
        std::cerr << failures << " ClipOverwriteCommand test(s) failed\n";
    return failures == 0 ? 0 : 1;
}

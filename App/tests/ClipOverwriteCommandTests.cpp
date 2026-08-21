/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "ClipOverwriteCommand.h"

#include <chrono>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

using namespace tracktion;

bool closeTo(double actual, double expected, double tolerance = 1.0e-9)
{
    return std::abs(actual - expected) < tolerance;
}

class TemporaryWaveFile
{
public:
    TemporaryWaveFile()
        : file(juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getNonexistentChildFile("NextStudioClipOverwrite", ".wav", false))
    {
        juce::AudioBuffer<float> buffer(1, 4410);
        buffer.clear();
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            juce::WavAudioFormat().createWriterFor(file.createOutputStream().release(),
                                                    44100.0, 1, 16, {}, 0));
        REQUIRE(writer != nullptr);
        if (writer != nullptr)
            REQUIRE(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()));
    }

    ~TemporaryWaveFile() { file.deleteFile(); }

    juce::File file;
};

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

    te::AudioTrack *addMidiTrack()
    {
        edit->ensureNumberOfAudioTracks(te::getAudioTracks(*edit).size() + 1);
        auto *newTrack = te::getAudioTracks(*edit).getLast();
        newTrack->state.setProperty(::IDs::isMidiTrack, true, nullptr);
        return newTrack;
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

void testAudioCopyPreservesFadesTakesAndPlugins()
{
    Fixture f;
    f.track->state.setProperty(::IDs::isMidiTrack, false, nullptr);
    TemporaryWaveFile waveFile;
    ClipEditing::Placement insert;
    insert.mode = ClipEditing::PlacementMode::insertWaveFile;
    insert.destination = f.track;
    insert.finalPosition = {{TimePosition::fromSeconds(0.0), TimePosition::fromSeconds(2.0)}, {}};
    insert.name = "audio";
    insert.sourceFile = waveFile.file;
    auto inserted = ClipEditing::applyOverwrite(*f.edit, f.selection, {insert});
    REQUIRE(inserted.succeeded && !inserted.clips.isEmpty());
    auto source = te::WaveAudioClip::Ptr(dynamic_cast<te::WaveAudioClip *>(inserted.clips.getFirst().get()));
    REQUIRE(source != nullptr);
    if (source == nullptr)
        return;
    source->setFadeIn(TimeDuration::fromSeconds(0.2));
    source->setFadeOut(TimeDuration::fromSeconds(0.3));
    source->setFadeInType(te::AudioFadeCurve::concave);
    source->setFadeOutType(te::AudioFadeCurve::convex);
    source->addTake(waveFile.file);
    auto sourcePlugin = dynamic_cast<te::VolumeAndPanPlugin *>(
        source->getPluginList()->insertPlugin(te::VolumeAndPanPlugin::create(), 0).get());
    REQUIRE(sourcePlugin != nullptr);
    if (sourcePlugin != nullptr)
        sourcePlugin->setVolumeDb(-6.0f);

    ClipEditing::Placement placement;
    placement.mode = ClipEditing::PlacementMode::copy;
    placement.source = source;
    placement.destination = f.track;
    placement.finalPosition = {{TimePosition::fromSeconds(3.0), TimePosition::fromSeconds(5.0)}, {}};
    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {placement});

    REQUIRE(result.succeeded);
    auto copied = te::WaveAudioClip::Ptr(dynamic_cast<te::WaveAudioClip *>(result.clips.getFirst().get()));
    REQUIRE(copied != nullptr);
    REQUIRE(closeTo(copied->getFadeIn().inSeconds(), 0.2));
    REQUIRE(closeTo(copied->getFadeOut().inSeconds(), 0.3));
    REQUIRE(copied->getFadeInType() == te::AudioFadeCurve::concave);
    REQUIRE(copied->getFadeOutType() == te::AudioFadeCurve::convex);
    REQUIRE(copied->hasAnyTakes());
    auto copiedPlugin = copied->getPluginList()->findFirstPluginOfType<te::VolumeAndPanPlugin>();
    REQUIRE(copiedPlugin != nullptr);
    if (copiedPlugin != nullptr)
        REQUIRE(closeTo(copiedPlugin->getVolumeDb(), -6.0, 1.0e-4));
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

te::AutomatableParameter::Ptr firstAutomatableParameter(te::Track &track)
{
    const auto parameters = track.getAllAutomatableParams();
    if (parameters.isEmpty())
        return {};
    return te::AutomatableParameter::Ptr(parameters.getFirst());
}

te::AutomatableParameter::Ptr matchingParameter(te::Track &track, const te::AutomatableParameter &source)
{
    for (auto parameter : track.getAllAutomatableParams())
        if (parameter->getPluginAndParamName() == source.getPluginAndParamName())
            return parameter;
    return {};
}

void testAutomationMovesHorizontallyAndVertically()
{
    Fixture f;
    auto moving = f.add(0.0, 2.0);
    auto sourceParameter = firstAutomatableParameter(*f.track);
    REQUIRE(sourceParameter != nullptr);
    if (sourceParameter == nullptr)
        return;
    sourceParameter->getCurve().addPoint(TimePosition::fromSeconds(0.5), 0.75f, 0.0f);

    auto sourceOnlyPlugin = f.edit->getPluginCache().createNewPlugin(te::ReverbPlugin::xmlTypeName, {});
    REQUIRE(sourceOnlyPlugin != nullptr);
    if (sourceOnlyPlugin == nullptr)
        return;
    f.track->pluginList.insertPlugin(sourceOnlyPlugin, 0, nullptr);
    auto sourceOnlyParameter = sourceOnlyPlugin->getAutomatableParameter(0);
    REQUIRE(sourceOnlyParameter != nullptr);
    if (sourceOnlyParameter == nullptr)
        return;
    sourceOnlyParameter->getCurve().addPoint(TimePosition::fromSeconds(0.75), 0.5f, 0.0f);

    ClipEditing::Options horizontalOptions;
    horizontalOptions.moveAutomation = true;
    horizontalOptions.automationOffset = TimeDuration::fromSeconds(3.0);
    auto horizontal = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                                   {f.move(moving, 3.0, 5.0)},
                                                   horizontalOptions);
    REQUIRE(horizontal.succeeded);
    REQUIRE(!sourceParameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(3.0), TimePosition::fromSeconds(5.0)}).isEmpty());

    auto *destination = f.addMidiTrack();
    auto destinationParameter = matchingParameter(*destination, *sourceParameter);
    REQUIRE(destinationParameter != nullptr);
    if (destinationParameter == nullptr)
        return;

    ClipEditing::Placement verticalPlacement = f.move(moving, 6.0, 8.0);
    verticalPlacement.destination = destination;
    ClipEditing::Options verticalOptions;
    verticalOptions.moveAutomation = true;
    verticalOptions.automationOffset = TimeDuration::fromSeconds(3.0);
    auto vertical = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                                 {verticalPlacement}, verticalOptions);
    REQUIRE(vertical.succeeded);
    REQUIRE(!destinationParameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)}).isEmpty());
    REQUIRE(!sourceOnlyParameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(3.0), TimePosition::fromSeconds(5.0)}).isEmpty());
}

void testResizeAndTimeStretchPlacementsOverwriteDestination()
{
    Fixture f;
    auto resizing = f.add(0.0, 2.0, "resize");
    f.add(2.0, 5.0, "victim");
    auto resized = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                                {f.move(resizing, 0.0, 3.0)});
    REQUIRE(resized.succeeded);
    REQUIRE(closeTo(resizing->getPosition().getEnd().inSeconds(), 3.0));
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));

    bool finalised = false;
    auto stretchPlacement = f.move(resizing, 0.0, 4.0);
    stretchPlacement.finalize = [&finalised](te::Clip &)
    {
        finalised = true;
        return true;
    };
    auto stretched = ClipEditing::applyOverwrite(*f.edit, f.selection, {stretchPlacement});
    REQUIRE(stretched.succeeded);
    REQUIRE(finalised);
    REQUIRE(closeTo(resizing->getPosition().getEnd().inSeconds(), 4.0));
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
}

void testMultiTrackTimeRangeMove()
{
    Fixture f;
    auto *secondTrack = f.addMidiTrack();
    auto firstSource = f.add(0.0, 4.0, "first");
    auto secondSource = secondTrack->insertMIDIClip("second",
                                                    {TimePosition::fromSeconds(0.0), TimePosition::fromSeconds(4.0)},
                                                    nullptr);
    REQUIRE(secondSource != nullptr);

    std::vector<ClipEditing::Placement> placements;
    for (auto pair : {std::pair<te::MidiClip::Ptr, te::ClipTrack *>{firstSource, f.track},
                      std::pair<te::MidiClip::Ptr, te::ClipTrack *>{secondSource, secondTrack}})
    {
        ClipEditing::Placement placement;
        placement.mode = ClipEditing::PlacementMode::insertState;
        placement.state = pair.first->state.createCopy();
        placement.destination = pair.second;
        placement.finalPosition = {{TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)},
                                   pair.first->getPosition().offset + TimeDuration::fromSeconds(1.0)};
        placements.push_back(std::move(placement));
    }

    ClipEditing::Options options;
    options.sourceRemovals.push_back({firstSource,
                                      {TimePosition::fromSeconds(1.0), TimePosition::fromSeconds(3.0)}});
    options.sourceRemovals.push_back({secondSource,
                                      {TimePosition::fromSeconds(1.0), TimePosition::fromSeconds(3.0)}});
    options.destinationRemovals.push_back({f.track,
                                           {TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)}});
    options.destinationRemovals.push_back({secondTrack,
                                           {TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)}});

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, std::move(placements), options);
    REQUIRE(result.succeeded);
    REQUIRE(result.clips.size() == 2);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
    REQUIRE(!ClipEditing::hasOverlaps(*secondTrack));
}

void testCommitFailureRollsBackModelAndSelection()
{
    Fixture f;
    auto moving = f.add(0.0, 2.0, "moving");
    auto victim = f.add(4.0, 8.0, "victim");
    f.selection.selectOnly(moving.get());
    f.edit->getUndoManager().clearUndoHistory();

    auto placement = f.move(moving, 5.0, 7.0);
    placement.finalize = [](te::Clip &) { return false; };
    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, {placement});

    REQUIRE(!result.succeeded);
    auto *restoredMoving = te::findClipForID(*f.edit, moving->itemID);
    auto *restoredVictim = te::findClipForID(*f.edit, victim->itemID);
    REQUIRE(restoredMoving != nullptr && restoredVictim != nullptr);
    REQUIRE(closeTo(restoredMoving->getPosition().getStart().inSeconds(), 0.0));
    REQUIRE(closeTo(restoredVictim->getPosition().getStart().inSeconds(), 4.0));
    REQUIRE(closeTo(restoredVictim->getPosition().getEnd().inSeconds(), 8.0));
    REQUIRE(f.selection.isSelected(restoredMoving));
}

void testValidationForFrozenTracksBoundsAndDuplicateMoves()
{
    Fixture f;
    auto source = f.add(0.0, 2.0);

    auto negative = f.move(source, -1.0, 1.0);
    REQUIRE(!ClipEditing::applyOverwrite(*f.edit, f.selection, {negative}).succeeded);
    REQUIRE(closeTo(source->getPosition().getStart().inSeconds(), 0.0));

    f.track->setFrozen(true, te::Track::groupFreeze);
    REQUIRE(f.track->isFrozen(te::Track::anyFreeze));
    REQUIRE(!ClipEditing::applyOverwrite(*f.edit, f.selection,
                                          {f.move(source, 3.0, 5.0)}).succeeded);
    f.track->setFrozen(false, te::Track::groupFreeze);

    REQUIRE(!ClipEditing::applyOverwrite(*f.edit, f.selection,
                                          {f.move(source, 3.0, 5.0),
                                           f.move(source, 6.0, 8.0)}).succeeded);
    REQUIRE(closeTo(source->getPosition().getStart().inSeconds(), 0.0));
}

class TwoClipLimitBehaviour final : public te::EngineBehaviour
{
public:
    te::EditLimits getEditLimits() override
    {
        auto limits = te::EngineBehaviour::getEditLimits();
        limits.maxClipsInTrack = 2;
        return limits;
    }
};

void testClipLimitFailureRollsBack()
{
    te::Engine engine("NextStudioClipLimitTests", nullptr,
                      std::make_unique<TwoClipLimitBehaviour>());
    auto edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager selection(engine);
    auto *track = te::getFirstAudioTrack(*edit);
    track->state.setProperty(::IDs::isMidiTrack, true, nullptr);
    auto source = track->insertMIDIClip("source",
                                        {TimePosition::fromSeconds(0.0), TimePosition::fromSeconds(2.0)},
                                        nullptr);
    auto other = track->insertMIDIClip("other",
                                       {TimePosition::fromSeconds(3.0), TimePosition::fromSeconds(5.0)},
                                       nullptr);
    REQUIRE(source != nullptr && other != nullptr);

    ClipEditing::Placement placement;
    placement.mode = ClipEditing::PlacementMode::copy;
    placement.source = source;
    placement.destination = track;
    placement.finalPosition = {{TimePosition::fromSeconds(6.0), TimePosition::fromSeconds(8.0)}, {}};
    auto result = ClipEditing::applyOverwrite(*edit, selection, {placement});

    REQUIRE(!result.succeeded);
    REQUIRE(track->getClips().size() == 2);
    REQUIRE(te::findClipForID(*edit, source->itemID) != nullptr);
    REQUIRE(te::findClipForID(*edit, other->itemID) != nullptr);
}

void testGroupedCopiesKeepSharedRemappedGroup()
{
    Fixture f;
    auto first = f.add(0.0, 2.0);
    auto second = f.add(2.0, 4.0);
    const auto originalGroup = f.edit->createNewItemID();
    first->setGroup(originalGroup);
    second->setGroup(originalGroup);

    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.copy(first, 6.0, 8.0),
                                                f.copy(second, 8.0, 10.0)});
    REQUIRE(result.succeeded);
    REQUIRE(result.clips.size() == 2);
    REQUIRE(result.clips[0]->isGrouped() && result.clips[1]->isGrouped());
    REQUIRE(result.clips[0]->getGroupID() == result.clips[1]->getGroupID());
    REQUIRE(result.clips[0]->getGroupID() != originalGroup);
}

void testArrangementRecordingMode()
{
    Fixture f;
    auto &deviceManager = f.engine.getDeviceManager();
    const auto deviceName = "ClipOverwriteRecordingTest-"
                            + juce::String(juce::Time::currentTimeMillis());
    REQUIRE(deviceManager.createVirtualMidiDevice(deviceName).wasOk());
    juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    deviceManager.dispatchPendingUpdates();
    auto devices = deviceManager.getMidiInDevices();
    std::shared_ptr<te::MidiInputDevice> device;
    for (auto candidate : devices)
        if (candidate->getName() == deviceName)
            device = std::move(candidate);
    REQUIRE(device != nullptr);
    if (device == nullptr)
        return;

    device->mergeRecordings = true;
    device->replaceExistingClips = false;
    ClipEditing::configureArrangementRecordingDevice(*device);
    REQUIRE(!device->mergeRecordings);
    REQUIRE(device->replaceExistingClips);

    if (auto *virtualDevice = dynamic_cast<te::VirtualMidiInputDevice *>(device.get()))
        deviceManager.deleteVirtualMidiDevice(*virtualDevice);
}

void testBulkMoveRegression()
{
    Fixture f;
    constexpr int clipCount = 200;
    std::vector<ClipEditing::Placement> placements;
    placements.reserve(clipCount);
    for (int i = 0; i < clipCount; ++i)
    {
        const auto start = static_cast<double>(i * 2);
        auto clip = f.add(start, start + 1.0);
        placements.push_back(f.move(clip, start + 1000.0, start + 1001.0));
    }

    const auto started = std::chrono::steady_clock::now();
    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection, std::move(placements));
    const auto elapsed = std::chrono::steady_clock::now() - started;
    REQUIRE(result.succeeded);
    REQUIRE(result.clips.size() == clipCount);
    REQUIRE(!ClipEditing::hasOverlaps(*f.track));
    REQUIRE(elapsed < std::chrono::seconds(10));
}

void testUndoRedoIsAtomic()
{
    Fixture f;
    auto moving = f.add(0.0, 2.0);
    f.add(4.0, 8.0);
    f.edit->getUndoManager().clearUndoHistory();

    auto parameter = firstAutomatableParameter(*f.track);
    REQUIRE(parameter != nullptr);
    if (parameter == nullptr)
        return;

    ClipEditing::Options options;
    options.additionalEdit = [parameter]
    {
        parameter->getCurve().addPoint(TimePosition::fromSeconds(10.0), 0.25f, 0.0f);
        return true;
    };
    auto result = ClipEditing::applyOverwrite(*f.edit, f.selection,
                                               {f.move(moving, 5.0, 7.0)}, options);
    REQUIRE(result.succeeded);
    REQUIRE(closeTo(moving->getPosition().getStart().inSeconds(), 5.0));
    REQUIRE(!parameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(9.0), TimePosition::fromSeconds(11.0)}).isEmpty());

    REQUIRE(f.edit->getUndoManager().undo());
    auto *restored = te::findClipForID(*f.edit, moving->itemID);
    REQUIRE(restored != nullptr);
    REQUIRE(closeTo(restored->getPosition().getStart().inSeconds(), 0.0));
    REQUIRE(parameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(9.0), TimePosition::fromSeconds(11.0)}).isEmpty());
    REQUIRE(f.edit->getUndoManager().redo());
    REQUIRE(closeTo(te::findClipForID(*f.edit, moving->itemID)->getPosition().getStart().inSeconds(), 5.0));
    REQUIRE(!parameter->getCurve().getPointsInRegion(
                 {TimePosition::fromSeconds(9.0), TimePosition::fromSeconds(11.0)}).isEmpty());
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
    testAudioCopyPreservesFadesTakesAndPlugins();
    testAutomationMovesHorizontallyAndVertically();
    testResizeAndTimeStretchPlacementsOverwriteDestination();
    testMultiTrackTimeRangeMove();
    testCommitFailureRollsBackModelAndSelection();
    testValidationForFrozenTracksBoundsAndDuplicateMoves();
    testClipLimitFailureRollsBack();
    testGroupedCopiesKeepSharedRemappedGroup();
    testArrangementRecordingMode();
    testBulkMoveRegression();
    testUndoRedoIsAtomic();

    if (failures != 0)
        std::cerr << failures << " ClipOverwriteCommand test(s) failed\n";
    return failures == 0 ? 0 : 1;
}

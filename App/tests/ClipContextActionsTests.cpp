/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "ApplicationViewState.h"
#include "ClipContextActions.h"

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

struct TempDirectory
{
    TempDirectory()
        : file(juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getNonexistentChildFile("nextstudio-clip-context-tests", {}, false))
    {
        require(file.createDirectory(), "temporary directory could not be created");
    }

    ~TempDirectory() { file.deleteRecursively(); }

    juce::File file;
};

struct Fixture
{
    Fixture()
        : engine("NextStudioClipContextActionsTests"),
          edit(te::Edit::createSingleTrackEdit(engine)),
          selection(engine)
    {
        firstTrack = te::getFirstAudioTrack(*edit);
        require(firstTrack != nullptr, "initial track missing");
        edit->ensureNumberOfAudioTracks(2);
        secondTrack = te::getAudioTracks(*edit).getLast();
        require(secondTrack != nullptr && secondTrack != firstTrack, "second track missing");
    }

    te::MidiClip::Ptr add(te::AudioTrack &track, double start, double end,
                          const juce::String &name)
    {
        auto clip = track.insertMIDIClip(name,
                                         {tracktion::TimePosition::fromSeconds(start),
                                          tracktion::TimePosition::fromSeconds(end)},
                                         nullptr);
        require(clip != nullptr, "clip insertion failed");
        return clip;
    }

    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    te::SelectionManager selection;
    te::AudioTrack *firstTrack = nullptr;
    te::AudioTrack *secondTrack = nullptr;
};

void testRenameValidationAndUndo()
{
    Fixture fixture;
    auto clip = fixture.add(*fixture.firstTrack, 1.0, 3.0, "Original");
    fixture.edit->getUndoManager().clearUndoHistory();

    require(!ClipContextActions::renameClip(*fixture.edit, clip, "   "),
            "empty clip name was accepted");
    require(clip->getName() == "Original", "invalid rename changed the clip");
    require(!fixture.edit->getUndoManager().canUndo(),
            "invalid rename created an undo transaction");

    require(ClipContextActions::renameClip(*fixture.edit, clip, "  Renamed  "),
            "valid rename failed");
    require(clip->getName() == "Renamed", "rename did not trim or apply the name");
    require(fixture.edit->getUndoManager().undo(), "rename undo failed");
    require(clip->getName() == "Original", "rename undo did not restore the name");
    require(fixture.edit->getUndoManager().redo(), "rename redo failed");
    require(clip->getName() == "Renamed", "rename redo did not restore the new name");
}

void testDeleteAffectsOnlyClickedClipAndItsAutomation()
{
    Fixture fixture;
    auto clicked = fixture.add(*fixture.firstTrack, 1.0, 3.0, "Clicked");
    auto selected = fixture.add(*fixture.firstTrack, 4.0, 5.0, "Selected");
    fixture.selection.addToSelection(selected.get());

    const auto parameters = fixture.firstTrack->getAllAutomatableParams();
    require(!parameters.isEmpty(), "track has no automatable parameter");
    auto parameter = te::AutomatableParameter::Ptr(parameters.getFirst());
    parameter->getCurve().addPoint(tracktion::TimePosition::fromSeconds(0.5), 0.2f, 0.0f);
    parameter->getCurve().addPoint(tracktion::TimePosition::fromSeconds(2.0), 0.5f, 0.0f);
    parameter->getCurve().addPoint(tracktion::TimePosition::fromSeconds(3.5), 0.8f, 0.0f);
    fixture.edit->getUndoManager().clearUndoHistory();

    require(ClipContextActions::deleteClip(*fixture.edit, clicked), "clip deletion failed");
    require(fixture.firstTrack->getClips().size() == 1,
            "clip deletion removed an unexpected number of clips");
    require(fixture.firstTrack->getClips().getFirst() == selected.get(),
            "clip deletion removed the selected clip instead of the clicked clip");
    require(parameter->getCurve().getPointsInRegion(
                {tracktion::TimePosition::fromSeconds(1.0),
                 tracktion::TimePosition::fromSeconds(3.0)}).isEmpty(),
            "clip deletion retained automation in the clip range");
    require(parameter->getCurve().getNumPoints() == 2,
            "clip deletion removed automation outside the clip range");

    require(fixture.edit->getUndoManager().undo(), "clip deletion undo failed");
    require(fixture.firstTrack->getClips().size() == 2,
            "clip deletion undo did not restore the clip");
    require(parameter->getCurve().getNumPoints() == 3,
            "clip deletion undo did not restore automation");
}

void testRenderPlanUsesExactClipAndTrackBit()
{
    Fixture fixture;
    auto first = fixture.add(*fixture.firstTrack, 0.0, 2.0, "First");
    auto target = fixture.add(*fixture.secondTrack, 1.0, 4.0, "Target");

    const auto plan = ClipContextActions::createRenderPlan(*fixture.edit, target);
    require(plan.isValid(), "valid clip did not create a render plan");
    require(plan.range == target->getEditTimeRange(), "render plan range differs from clip range");
    const auto allTracks = te::getAllTracks(*fixture.edit);
    const auto firstTrackIndex = allTracks.indexOf(fixture.firstTrack);
    const auto targetTrackIndex = allTracks.indexOf(fixture.secondTrack);
    require(firstTrackIndex >= 0 && targetTrackIndex >= 0 && firstTrackIndex != targetTrackIndex,
            "fixture track indices are invalid");
    require(!plan.tracksToRender[firstTrackIndex] && plan.tracksToRender[targetTrackIndex],
            "render plan did not select the target track index");
    require(plan.tracksToRender.countNumberOfSetBits() == 1,
            "render plan selected more than one track");
    require(plan.clipsToRender.size() == 1 && plan.clipsToRender.getFirst() == target.get(),
            "render plan did not isolate the clicked clip");
    require(plan.clipsToRender.getFirst() != first.get(),
            "render plan included an unrelated clip");

    target->removeFromParent();
    require(!ClipContextActions::createRenderPlan(*fixture.edit, target).isValid(),
            "detached clip produced a valid render plan");
}

void testWaveSaveDirectoryIsSessionOnly()
{
    const TempDirectory temp;
    const auto settings = temp.file.getChildFile("settings/AppSettings.xml");
    const auto workspace = temp.file.getChildFile("workspace");
    const auto samples = workspace.getChildFile("Samples");
    const auto chosen = temp.file.getChildFile("exports");
    require(samples.createDirectory(), "Samples directory could not be created");
    require(chosen.createDirectory(), "export directory could not be created");

    {
        ApplicationViewState state(settings);
        state.setRootFolder(workspace);
        require(state.getWaveClipSaveDirectory() == samples,
                "first Wave save did not fall back to Samples");
        state.setWaveClipSaveDirectory(chosen);
        require(state.getWaveClipSaveDirectory() == chosen,
                "Wave save directory was not remembered in the session");
        state.saveState();
    }

    ApplicationViewState restored(settings);
    require(restored.getWaveClipSaveDirectory() == samples,
            "Wave save directory was persisted across sessions");
}
} // namespace

int main()
{
    try
    {
        testRenameValidationAndUndo();
        testDeleteAffectsOnlyClickedClipAndItsAutomation();
        testRenderPlanUsesExactClipAndTrackBit();
        testWaveSaveDirectoryIsSessionOnly();
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All clip context action tests passed.\n";
    return 0;
}

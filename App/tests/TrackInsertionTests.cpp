#include "TrackInsertion.h"

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

juce::Array<te::Track *> getMovableTopLevelTracks(te::Edit &edit)
{
    juce::Array<te::Track *> result;
    for (auto *track : te::getTopLevelTracks(edit))
        if (track != nullptr && track->isMovable())
            result.add(track);
    return result;
}

void requireOrder(const juce::Array<te::Track *> &actual, std::initializer_list<te::Track *> expected,
                  const char *message)
{
    require(actual.size() == static_cast<int>(expected.size()), message);

    int index = 0;
    for (auto *track : expected)
        require(actual[index++] == track, message);
}

void moveTracksToEnd(te::Edit &edit, const juce::Array<te::Track *> &tracks)
{
    auto destination = TrackInsertion::atEndOfEditExcluding(edit, tracks);
    for (int i = tracks.size(); --i >= 0;)
        edit.moveTrack(tracks.getUnchecked(i), destination);
}

void run()
{
    te::Engine engine("NextStudioTrackInsertionTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    auto *first = te::getAudioTracks(*edit).getFirst();
    require(first != nullptr, "initial track missing");

    auto last = edit->insertNewAudioTrack(TrackInsertion::atEndOfEdit(*edit), nullptr);
    require(last != nullptr, "end track insertion failed");

    auto belowFirst = edit->insertNewAudioTrack(TrackInsertion::afterTrack(*first), nullptr);
    require(belowFirst != nullptr, "track insertion below sibling failed");
    requireOrder(getMovableTopLevelTracks(*edit), {first, belowFirst.get(), last.get()},
                 "track below sibling has wrong top-level order");

    auto folder = edit->insertNewFolderTrack(TrackInsertion::atEndOfEdit(*edit), nullptr, true);
    require(folder != nullptr, "folder insertion failed");

    auto firstChild = edit->insertNewAudioTrack(TrackInsertion::atEndOfFolder(*folder), nullptr);
    auto lastChild = edit->insertNewAudioTrack(TrackInsertion::atEndOfFolder(*folder), nullptr);
    require(firstChild != nullptr && lastChild != nullptr, "folder child insertion failed");
    require(firstChild->getParentFolderTrack() == folder.get(), "first child has wrong parent");
    require(lastChild->getParentFolderTrack() == folder.get(), "last child has wrong parent");
    requireOrder(folder->getAllSubTracks(false), {firstChild.get(), lastChild.get()},
                 "folder children have wrong append order");

    auto betweenChildren = edit->insertNewAudioTrack(TrackInsertion::afterTrack(*firstChild), nullptr);
    require(betweenChildren != nullptr, "nested sibling insertion failed");
    require(betweenChildren->getParentFolderTrack() == folder.get(), "nested sibling escaped its folder");
    requireOrder(folder->getAllSubTracks(false), {firstChild.get(), betweenChildren.get(), lastChild.get()},
                 "nested sibling has wrong order");

    auto belowFolder = edit->insertNewFolderTrack(TrackInsertion::afterTrack(*folder), nullptr, true);
    require(belowFolder != nullptr, "folder sibling insertion failed");
    require(belowFolder->getParentFolderTrack() == nullptr, "folder sibling was inserted as a child");
    requireOrder(getMovableTopLevelTracks(*edit), {first, belowFirst.get(), last.get(), folder.get(), belowFolder.get()},
                 "folder sibling has wrong top-level order");

    auto nestedFolder = edit->insertNewFolderTrack(TrackInsertion::atEndOfFolder(*folder), nullptr, true);
    require(nestedFolder != nullptr, "nested folder insertion failed");
    require(nestedFolder->getParentFolderTrack() == folder.get(), "nested folder has wrong parent");

    auto &undoManager = edit->getUndoManager();
    undoManager.clearUndoHistory();
    undoManager.beginNewTransaction("Add child for undo test");
    auto undoChild = edit->insertNewAudioTrack(TrackInsertion::atEndOfFolder(*folder), nullptr);
    require(undoChild != nullptr, "undo child insertion failed");
    const auto undoChildID = undoChild->itemID;
    undoChild = nullptr;

    require(undoManager.undo(), "track insertion undo failed");
    require(te::findTrackForID(*edit, undoChildID) == nullptr, "undo retained inserted track");
    require(undoManager.redo(), "track insertion redo failed");
    auto *redone = te::findTrackForID(*edit, undoChildID);
    require(redone != nullptr, "redo did not restore inserted track");
    require(redone->getParentFolderTrack() == folder.get(), "redo restored track under wrong parent");

    auto moveEdit = te::Edit::createSingleTrackEdit(engine);
    auto *moveFirst = te::getAudioTracks(*moveEdit).getFirst();
    auto moveMiddle = moveEdit->insertNewAudioTrack(TrackInsertion::atEndOfEdit(*moveEdit), nullptr);
    auto moveLast = moveEdit->insertNewAudioTrack(TrackInsertion::atEndOfEdit(*moveEdit), nullptr);
    require(moveFirst != nullptr && moveMiddle != nullptr && moveLast != nullptr, "move test setup failed");

    moveTracksToEnd(*moveEdit, {moveMiddle.get(), moveLast.get()});
    requireOrder(getMovableTopLevelTracks(*moveEdit), {moveFirst, moveMiddle.get(), moveLast.get()},
                 "moving a trailing block to the end changed its order");

    moveTracksToEnd(*moveEdit, {moveFirst, moveLast.get()});
    requireOrder(getMovableTopLevelTracks(*moveEdit), {moveMiddle.get(), moveFirst, moveLast.get()},
                 "moving a discontiguous block to the end changed its order");
}
} // namespace

int main()
{
    try
    {
        run();
        std::cout << "All track insertion tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Track insertion test failed: " << error.what() << "\n";
        return 1;
    }
}

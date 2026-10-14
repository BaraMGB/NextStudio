/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "MidiClipJoinCommand.h"

namespace MidiClipJoining
{
namespace
{
juce::Result mergeMidiClipsTempoAware(juce::Array<te::MidiClip *> clips,
                                      te::SelectionManager &selectionManager)
{
    te::TrackItem::sortByTime(clips);

    auto *first = clips.getFirst();
    auto *track = first != nullptr ? first->getClipTrack() : nullptr;
    if (track == nullptr)
        return juce::Result::fail("No clips to merge");

    auto newClip = track->insertMIDIClip(first->getName(), first->getPosition().time,
                                         &selectionManager);
    if (newClip == nullptr)
        return juce::Result::fail("Could not create the joined MIDI clip");

    auto &destinationQuantisation = newClip->getQuantisation();
    const auto &sourceQuantisation = first->getQuantisation();
    destinationQuantisation.setType(sourceQuantisation.getType(false));
    destinationQuantisation.setProportion(sourceQuantisation.getProportion());
    destinationQuantisation.setIsQuantisingNoteOffs(
        sourceQuantisation.isQuantisingNoteOffs());
    newClip->setGrooveTemplate(first->getGrooveTemplate());
    newClip->setMidiChannel(first->getMidiChannel());

    auto startBeat = tracktion::BeatPosition::fromBeats(1.0e10);
    auto startTime = tracktion::TimePosition::fromSeconds(1.0e10);
    auto endTime = tracktion::TimePosition();

    for (auto *clip : clips)
    {
        startBeat = juce::jmin(startBeat, clip->getStartBeat());
        startTime = juce::jmin(startTime, clip->getPosition().getStart());
        endTime = juce::jmax(endTime, clip->getPosition().getEnd());
    }

    te::MidiList destinationList;
    destinationList.setMidiChannel(newClip->getMidiChannel());

    for (auto *clip : clips)
    {
        te::MidiList sourceList;
        sourceList.copyFrom(clip->getSequenceLooped(), nullptr);

        // A source offset may cross tempo changes, so its beat duration must be
        // derived from both timeline endpoints rather than the tempo at one end.
        const auto offset = clip->getStartBeat() - clip->getContentStartBeat();
        sourceList.trimOutside(tracktion::BeatPosition::fromBeats(offset.inBeats()),
                               tracktion::BeatPosition::fromBeats(
                                   (offset + clip->getLengthInBeats()).inBeats()),
                               nullptr);
        sourceList.moveAllBeatPositions(clip->getStartBeat() - startBeat - offset, nullptr);
        destinationList.addFrom(sourceList, nullptr);
    }

    newClip->setPosition({{startTime, endTime}, tracktion::TimeDuration()});
    newClip->getSequence().addFrom(destinationList, &track->edit.getUndoManager());

    for (int index = clips.size(); --index >= 0;)
        clips.getUnchecked(index)->removeFromParent();

    return juce::Result::ok();
}

juce::String getIncompatiblePlaybackSettingsError(const te::MidiClip &reference,
                                                   const te::MidiClip &candidate)
{
    if (candidate.getMidiChannel() != reference.getMidiChannel())
        return "Selected MIDI clips use different MIDI channels";

    if (candidate.getMPEMode() != reference.getMPEMode())
        return "Selected MIDI clips use different MPE modes";

    if (!(candidate.getQuantisation() == reference.getQuantisation()))
        return "Selected MIDI clips use different quantisation settings";

    if (candidate.getGrooveTemplate() != reference.getGrooveTemplate()
        || candidate.getGrooveStrength() != reference.getGrooveStrength())
        return "Selected MIDI clips use different groove settings";

    if (candidate.getVolumeDb() != reference.getVolumeDb())
        return "Selected MIDI clips use different clip volumes";

    if (candidate.isMuted() != reference.isMuted())
        return "Selected MIDI clips use different mute states";

    if (candidate.isSendingBankChanges() != reference.isSendingBankChanges())
        return "Selected MIDI clips use different bank-change settings";

    return {};
}

void restoreSelection(te::Edit &edit, te::SelectionManager &selectionManager,
                      const juce::Array<te::EditItemID> &clipIDs)
{
    selectionManager.deselectAll();

    for (const auto clipID : clipIDs)
        if (auto *clip = te::findClipForID(edit, clipID))
            selectionManager.addToSelection(clip);
}
} // namespace

Selection inspectSelection(const te::SelectionManager &selectionManager)
{
    Selection result;
    const auto selectedClips = selectionManager.getItemsOfType<te::Clip>();

    if (selectedClips.size() < 2)
    {
        result.error = "Select at least two adjacent MIDI clips";
        return result;
    }

    result.clips = selectionManager.getItemsOfType<te::MidiClip>();
    if (result.clips.size() != selectedClips.size())
    {
        result.error = "Only MIDI clips can be joined";
        result.clips.clear();
        return result;
    }

    result.track = result.clips.getFirst()->getClipTrack();
    if (result.track == nullptr)
    {
        result.error = "The selected clips are not on a track";
        result.clips.clear();
        return result;
    }

    for (auto *clip : result.clips)
        if (clip == nullptr || clip->getClipTrack() != result.track)
        {
            result.error = "The selected MIDI clips must be on the same track";
            result.clips.clear();
            result.track = nullptr;
            return result;
        }

    if (result.track->isFrozen(te::Track::anyFreeze))
    {
        result.error = "Unable to join clips on a frozen track";
        result.clips.clear();
        result.track = nullptr;
        return result;
    }

    auto orderedClips = result.track->getClips();
    te::TrackItem::sortByTime(orderedClips);

    int firstSelectedIndex = orderedClips.size();
    int lastSelectedIndex = -1;
    for (auto *clip : result.clips)
    {
        const auto index = orderedClips.indexOf(clip);
        if (index < 0)
        {
            result.error = "A selected MIDI clip is no longer available";
            result.clips.clear();
            result.track = nullptr;
            return result;
        }

        firstSelectedIndex = juce::jmin(firstSelectedIndex, index);
        lastSelectedIndex = juce::jmax(lastSelectedIndex, index);
    }

    if (lastSelectedIndex - firstSelectedIndex + 1 != result.clips.size())
    {
        result.error = "Selected MIDI clips must be direct neighbours";
        result.clips.clear();
        result.track = nullptr;
        return result;
    }

    for (int index = firstSelectedIndex; index <= lastSelectedIndex; ++index)
        if (!result.clips.contains(dynamic_cast<te::MidiClip *>(orderedClips[index])))
        {
            result.error = "Selected MIDI clips must be direct neighbours";
            result.clips.clear();
            result.track = nullptr;
            return result;
        }

    te::TrackItem::sortByTime(result.clips);

    const auto *reference = result.clips.getFirst();
    for (int index = 1; index < result.clips.size(); ++index)
        if (auto error = getIncompatiblePlaybackSettingsError(*reference, *result.clips[index]);
            error.isNotEmpty())
        {
            result.error = error;
            result.clips.clear();
            result.track = nullptr;
            return result;
        }

    result.valid = true;
    return result;
}

Result joinSelectedClips(te::Edit &edit, te::SelectionManager &selectionManager)
{
    Result result;
    auto selection = inspectSelection(selectionManager);
    if (!selection.valid)
    {
        result.error = selection.error;
        return result;
    }

    juce::Array<te::EditItemID> selectedIDs;
    for (auto *clip : selection.clips)
        selectedIDs.add(clip->itemID);

    te::MidiClip::Ptr referenceClip = selection.clips.getFirst();
    auto &undoManager = edit.getUndoManager();
    undoManager.beginNewTransaction("Join MIDI clips");
    te::Edit::UndoTransactionInhibitor undoInhibitor(edit);
    te::TransportControl::ReallocationInhibitor graphInhibitor(edit.getTransport());

    const auto mergeResult = mergeMidiClipsTempoAware(selection.clips, selectionManager);
    if (mergeResult.failed())
    {
        undoManager.undoCurrentTransactionOnly();
        restoreSelection(edit, selectionManager, selectedIDs);
        result.error = mergeResult.getErrorMessage();
        return result;
    }

    const auto selectedAfter = selectionManager.getItemsOfType<te::MidiClip>();
    if (selectedAfter.size() != 1
        || selectedAfter.getFirst()->getClipTrack() != selection.track
        || selectedIDs.contains(selectedAfter.getFirst()->itemID))
    {
        undoManager.undoCurrentTransactionOnly();
        restoreSelection(edit, selectionManager, selectedIDs);
        result.error = "Could not create the joined MIDI clip";
        return result;
    }

    result.mergedClip = selectedAfter.getFirst();
    result.mergedClip->setMPEMode(referenceClip->getMPEMode());
    result.mergedClip->setGrooveStrength(referenceClip->getGrooveStrength());
    result.mergedClip->setVolumeDb(referenceClip->getVolumeDb());
    result.mergedClip->setMuted(referenceClip->isMuted());
    result.mergedClip->setSendingBankChanges(referenceClip->isSendingBankChanges());
    result.succeeded = true;
    return result;
}

} // namespace MidiClipJoining

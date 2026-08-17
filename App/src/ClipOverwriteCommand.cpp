/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ClipOverwriteCommand.h"

#include "ScopedSaveLock.h"

#include <algorithm>
#include <map>
#include <set>

namespace ClipEditing
{
namespace
{
using TimeRange = tracktion::TimeRange;

struct Victim
{
    te::Clip::Ptr clip;
    std::vector<TimeRange> masks;
};

bool isAttached(const te::Clip::Ptr &clip)
{
    return clip != nullptr && clip->getClipTrack() != nullptr;
}

bool accepts(const Placement &placement)
{
    auto *audioTrack = dynamic_cast<te::AudioTrack *>(placement.destination);
    if (audioTrack == nullptr || audioTrack->isFrozen(te::Track::anyFreeze))
        return false;

    const bool targetIsMidi = static_cast<bool>(audioTrack->state.getProperty(IDs::isMidiTrack));

    if (placement.source != nullptr)
        return placement.source->isMidi() == targetIsMidi;

    auto type = placement.type;
    if (placement.mode == PlacementMode::insertMidi)
        type = te::TrackItem::Type::midi;
    else if (placement.mode == PlacementMode::insertWaveFile)
        type = te::TrackItem::Type::wave;
    else if (placement.state.isValid())
        type = te::TrackItem::xmlTagToType(placement.state.getType().toString());

    const bool sourceIsMidi = type == te::TrackItem::Type::midi || type == te::TrackItem::Type::step;
    const bool sourceIsAudio = type == te::TrackItem::Type::wave || type == te::TrackItem::Type::edit;
    return (sourceIsMidi && targetIsMidi) || (sourceIsAudio && !targetIsMidi);
}

void mergeMasks(std::vector<TimeRange> &ranges)
{
    std::sort(ranges.begin(), ranges.end(), [](const auto &a, const auto &b)
    {
        return a.getStart() < b.getStart();
    });

    std::vector<TimeRange> merged;
    for (const auto &range : ranges)
    {
        if (merged.empty() || merged.back().getEnd() < range.getStart())
        {
            merged.push_back(range);
            continue;
        }

        merged.back() = {merged.back().getStart(), std::max(merged.back().getEnd(), range.getEnd())};
    }
    ranges = std::move(merged);
}

void removeMasksFromVictim(Victim &victim)
{
    juce::Array<te::Clip::Ptr> fragments;
    fragments.add(victim.clip);

    for (const auto &mask : victim.masks)
    {
        juce::Array<te::Clip::Ptr> next;

        for (auto fragment : fragments)
        {
            if (!isAttached(fragment))
                continue;

            if (!fragment->getPosition().time.overlaps(mask))
            {
                next.add(fragment);
                continue;
            }

            auto created = te::deleteRegion(*fragment, mask);
            if (isAttached(fragment))
                next.add(fragment);

            for (auto *newClip : created)
                if (newClip != nullptr && newClip->getClipTrack() != nullptr)
                    next.add(newClip);
        }

        fragments = std::move(next);
    }
}

void restoreClipSelection(te::Edit &edit, te::SelectionManager &selectionManager,
                          const juce::Array<te::EditItemID> &ids)
{
    selectionManager.deselectAll();
    for (auto id : ids)
        if (auto *clip = te::findClipForID(edit, id))
            selectionManager.addToSelection(clip);
}

bool placementIsExisting(const Placement &placement)
{
    return placement.mode == PlacementMode::move;
}

bool placementIsCopy(const Placement &placement)
{
    return placement.mode == PlacementMode::copy;
}

te::Clip::Ptr insertPlacement(te::Edit &edit, Placement &placement,
                              std::map<te::EditItemID, te::EditItemID> &remappedIDs)
{
    if (placement.destination == nullptr)
        return {};

    if (placement.mode == PlacementMode::insertMidi)
        return placement.destination->insertMIDIClip(placement.name, placement.finalPosition.time, nullptr);

    if (placement.mode == PlacementMode::insertWaveFile)
        return placement.destination->insertWaveClip(placement.name, placement.sourceFile,
                                                      placement.finalPosition, false);

    auto state = placement.state.createCopy();
    if (!state.isValid())
        return {};

    te::EditItemID::remapIDs(state, nullptr, edit, &remappedIDs);
    auto newClip = te::Clip::Ptr(placement.destination->insertClipWithState(state));
    if (newClip != nullptr)
    {
        newClip->setPosition(placement.finalPosition);
        if (placement.name.isNotEmpty())
            newClip->setName(placement.name);
    }
    return newClip;
}

juce::Array<te::EditItemID> getSelectedClipIDs(te::SelectionManager &selectionManager)
{
    juce::Array<te::EditItemID> result;
    for (auto *clip : selectionManager.getItemsOfType<te::Clip>())
        result.add(clip->itemID);
    return result;
}

} // namespace

bool hasOverlaps(const te::ClipTrack &track)
{
    std::vector<te::Clip *> clips;
    for (auto *clip : track.getClips())
        clips.push_back(clip);

    std::sort(clips.begin(), clips.end(), [](const auto *a, const auto *b)
    {
        return a->getPosition().getStart() < b->getPosition().getStart();
    });

    for (size_t i = 0; i < clips.size(); ++i)
        for (size_t j = i + 1; j < clips.size(); ++j)
        {
            if (clips[j]->getPosition().getStart() >= clips[i]->getPosition().getEnd())
                break;
            if (clips[i]->getPosition().time.overlaps(clips[j]->getPosition().time))
                return true;
        }

    return false;
}

juce::String describeOverlaps(const te::ClipTrack &track)
{
    juce::String result;
    const auto &clips = track.getClips();
    for (int i = 0; i < clips.size(); ++i)
        for (int j = i + 1; j < clips.size(); ++j)
            if (clips[i]->getPosition().time.overlaps(clips[j]->getPosition().time))
                result << clips[i]->itemID.toString() << " overlaps "
                       << clips[j]->itemID.toString() << "; ";
    return result;
}

Result applyOverwrite(te::Edit &edit, te::SelectionManager &selectionManager,
                      std::vector<Placement> placements, const Options &options)
{
    Result result;
    if (placements.empty() && options.sourceRemovals.empty() && options.destinationRemovals.empty())
    {
        result.error = "No clip placements or removals";
        return result;
    }

    std::set<te::EditItemID> protectedIDs;
    std::set<te::EditItemID> movedSourceIDs;
    std::map<te::ClipTrack *, std::vector<TimeRange>> masksByTrack;
    std::set<te::ClipTrack *> affectedTracks;

    for (auto &placement : placements)
    {
        if (placement.destination == nullptr || &placement.destination->edit != &edit
            || !accepts(placement))
        {
            result.error = "Invalid or incompatible destination track";
            return result;
        }

        if (placement.finalPosition.getLength() <= tracktion::TimeDuration()
            || placement.finalPosition.getStart() < tracktion::TimePosition()
            || placement.finalPosition.getEnd() > te::Edit::getMaximumEditEnd())
        {
            result.error = "Invalid clip position";
            return result;
        }

        if (placementIsExisting(placement))
        {
            if (!isAttached(placement.source) || &placement.source->edit != &edit)
            {
                result.error = "A source clip is no longer available";
                return result;
            }
            if (!movedSourceIDs.insert(placement.source->itemID).second)
            {
                result.error = "A source clip is moved more than once";
                return result;
            }
            protectedIDs.insert(placement.source->itemID);
        }
        else if (placementIsCopy(placement))
        {
            if (!isAttached(placement.source) || &placement.source->edit != &edit)
            {
                result.error = "A source clip is no longer available";
                return result;
            }
            placement.state = placement.source->state.createCopy();
            placement.name = placement.source->getName();
        }
        else if (placement.mode == PlacementMode::insertState && !placement.state.isValid())
        {
            result.error = "Missing clip state";
            return result;
        }
        else if (placement.mode == PlacementMode::insertWaveFile && !placement.sourceFile.existsAsFile())
        {
            result.error = "Missing audio file";
            return result;
        }

        masksByTrack[placement.destination].push_back(placement.finalPosition.time);
        affectedTracks.insert(placement.destination);
        if (placement.source != nullptr && placement.source->getClipTrack() != nullptr)
            affectedTracks.insert(placement.source->getClipTrack());
    }

    for (const auto &removal : options.destinationRemovals)
    {
        if (removal.track == nullptr || &removal.track->edit != &edit || removal.range.isEmpty())
        {
            result.error = "Invalid destination removal";
            return result;
        }
        masksByTrack[removal.track].push_back(removal.range);
        affectedTracks.insert(removal.track);
    }

    for (size_t i = 0; i < placements.size(); ++i)
        for (size_t j = i + 1; j < placements.size(); ++j)
            if (placements[i].destination == placements[j].destination
                && placements[i].finalPosition.time.overlaps(placements[j].finalPosition.time))
            {
                result.error = "Placed clips would overlap each other";
                return result;
            }

    for (auto &[track, masks] : masksByTrack)
        mergeMasks(masks);

    std::map<te::EditItemID, Victim> victimsByID;
    for (const auto &[track, masks] : masksByTrack)
        for (auto *clip : track->getClips())
        {
            if (protectedIDs.contains(clip->itemID))
                continue;

            for (const auto &mask : masks)
                if (clip->getPosition().time.overlaps(mask))
                {
                    auto it = victimsByID.try_emplace(clip->itemID, Victim{clip, {}}).first;
                    it->second.masks.push_back(mask);
                }
        }

    for (const auto &removal : options.sourceRemovals)
    {
        if (!isAttached(removal.clip) || &removal.clip->edit != &edit)
        {
            result.error = "A source removal clip is no longer available";
            return result;
        }

        auto it = victimsByID.try_emplace(removal.clip->itemID, Victim{removal.clip, {}}).first;
        it->second.masks.push_back(removal.range);
        affectedTracks.insert(removal.clip->getClipTrack());
    }

    std::vector<Victim> victims;
    victims.reserve(victimsByID.size());
    for (auto &entry : victimsByID)
    {
        mergeMasks(entry.second.masks);
        victims.push_back(std::move(entry.second));
    }

    juce::Array<te::TrackAutomationSection> automationSections;
    if (options.moveAutomation)
        for (const auto &placement : placements)
            if (placement.source != nullptr)
            {
                te::TrackAutomationSection section(*placement.source);
                section.dst = placement.destination;
                automationSections.add(std::move(section));
            }

    const auto selectedBefore = getSelectedClipIDs(selectionManager);
    auto &undoManager = edit.getUndoManager();
    undoManager.beginNewTransaction(options.undoName);

    te::Edit::UndoTransactionInhibitor undoInhibitor(edit);
    te::TransportControl::ReallocationInhibitor graphInhibitor(edit.getTransport());

    auto rollback = [&](const juce::String &error)
    {
        undoManager.undoCurrentTransactionOnly();
        restoreClipSelection(edit, selectionManager, selectedBefore);
        result.clips.clear();
        result.error = error;
        result.succeeded = false;
    };

    for (auto &victim : victims)
        removeMasksFromVictim(victim);

    std::map<te::EditItemID, te::EditItemID> remappedIDs;
    for (auto &placement : placements)
    {
        te::Clip::Ptr placedClip;

        if (placementIsExisting(placement))
        {
            placedClip = placement.source;
            if (placedClip->getClipTrack() != placement.destination
                && !placedClip->moveTo(*placement.destination))
            {
                rollback("Could not move a clip to its destination track");
                return result;
            }
            placedClip->setPosition(placement.finalPosition);
        }
        else
        {
            placedClip = insertPlacement(edit, placement, remappedIDs);
            if (placedClip == nullptr)
            {
                rollback("Could not insert a clip at its destination");
                return result;
            }
        }

        if (placement.finalize && !placement.finalize(*placedClip))
        {
            rollback("Could not finalise a placed clip");
            return result;
        }

        result.clips.add(placedClip);
    }

    if (options.moveAutomation && !automationSections.isEmpty())
        te::moveAutomation(automationSections, options.automationOffset, options.copyAutomation);

    for (auto *track : affectedTracks)
        if (track != nullptr && hasOverlaps(*track))
        {
            const auto details = describeOverlaps(*track);
            rollback("Clip overlap invariant failed: " + details);
            return result;
        }

    selectionManager.deselectAll();
    for (auto clip : result.clips)
        selectionManager.addToSelection(clip.get());

    result.succeeded = true;
    return result;
}

Result applyOverwrite(EditViewState &evs, std::vector<Placement> placements, const Options &options)
{
    ScopedSaveLock saveLock(evs);
    return applyOverwrite(evs.m_edit, evs.m_selectionManager, std::move(placements), options);
}

} // namespace ClipEditing


/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "Utilities/TrackHeightManager.h"
#include "Utilities/EditViewState.h"

TrackHeightManager::TrackHeightManager(const juce::Array<tracktion_engine::Track *> &allTracks) { regenerateTrackHeightsFromStates(allTracks); }

juce::Array<tracktion::EditItemID> TrackHeightManager::getShowedTracks(tracktion::Edit &edit)
{
    juce::Array<tracktion::EditItemID> showedTracks;

    for (auto t : tracktion_engine::getAllTracks(edit))
        if (isTrackShowable(t))
            if (getTrackHeight(t, true) > 0)
                showedTracks.add(t->itemID);

    return showedTracks;
}

bool TrackHeightManager::isTrackShowable(tracktion_engine::Track::Ptr track)
{
    if (track->isChordTrack() || track->isTempoTrack() || track->isMarkerTrack() || track->isArrangerTrack() || track->isAutomationTrack() || track->isMasterTrack())
    {
        return false;
    }

    return true;
}
int TrackHeightManager::getTrackHeight(tracktion_engine::Track *track, bool withAutomation)
{
    if (track == nullptr)
        return 0;

    TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return 0;

    if (isTrackInMinimizedFolderRecursive(track))
        return 0;

    if (trackInfo->isMinimized)
        return trackMinimizedHeight;

    int totalHeight = trackInfo->baseHeight;

    if (trackInfo->type == TrackType::Folder)
        totalHeight = folderTrackHeight;

    if (withAutomation)
    {
        juce::Array<te::AutomatableParameter *> params;
        for (const auto &[ap, height] : trackInfo->automationParameterHeights)
            params.add(ap);

        // Sort the parameters by their ID string to ensure a consistent order
        std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

        for (const auto &ap : params)
            totalHeight += trackInfo->automationParameterHeights[ap];
    }

    return totalHeight;
}

tracktion_engine::Track *TrackHeightManager::getTrackForY(int y, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto *info : trackInfos)
    {
        int trackHeight = info->baseHeight;

        if (y >= currentY && y < currentY + trackHeight)
        {
            return info->track;
        }

        int paramY = currentY + trackHeight;

        juce::Array<te::AutomatableParameter *> params;
        for (const auto &[param, height] : info->automationParameterHeights)
            params.add(param);

        std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

        for (const auto &param : params)
        {
            const int height = info->automationParameterHeights[param];
            if (y >= paramY && y < paramY + height)
            {
                return nullptr;
            }
            paramY += height;
        }

        currentY += getTrackHeight(info->track, true);
    }

    return nullptr;
}

int TrackHeightManager::getAutomationHeight(tracktion_engine::AutomatableParameter *ap)
{
    if (ap == nullptr)
        return 0;

    tracktion_engine::Track *track = ap->getTrack();
    if (track == nullptr)
        return 0;

    const TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return 0;

    auto it = trackInfo->automationParameterHeights.find(ap);
    if (it != trackInfo->automationParameterHeights.end())
        return it->second;

    return 0;
}

tracktion_engine::AutomatableParameter::Ptr TrackHeightManager::getAutomatableParameterForY(int y, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto *info : trackInfos)
    {
        const int trackHeightWithAutomation = getTrackHeight(info->track, true);
        const int trackEndY = currentY + trackHeightWithAutomation;

        // Early continue if y is not in current track range
        if (y < currentY || y >= trackEndY)
        {
            currentY = trackEndY;
            continue;
        }

        // Track found, check parameters
        int paramY = currentY + info->baseHeight;

        juce::Array<te::AutomatableParameter *> params;
        for (const auto &[ap, height] : info->automationParameterHeights)
            params.add(ap);

        // Sort the parameters by their ID string to ensure a consistent order
        std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

        for (const auto &ap : params)
        {
            const int height = info->automationParameterHeights[ap];
            const int paramEndY = paramY + height;

            if (y >= paramY && y < paramEndY)
                return ap;

            paramY = paramEndY;
        }

        // No parameter found but track matched, break search
        break;
    }

    return nullptr;
}

int TrackHeightManager::getYForAutomatableParameter(tracktion_engine::Track *track, const tracktion::AutomatableParameter::Ptr ap, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto *info : trackInfos)
    {
        if (info->track == track)
        {
            int paramY = currentY + info->baseHeight;

            juce::Array<te::AutomatableParameter *> params;
            for (const auto &[param, height] : info->automationParameterHeights)
                params.add(param);

            std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

            for (const auto &storedParam : params)
            {
                if (storedParam == ap.get())
                {
                    return paramY;
                }
                paramY += info->automationParameterHeights[storedParam];
            }
        }

        currentY += getTrackHeight(info->track, true);
    }

    return -1;
}
int TrackHeightManager::getYForTrack(tracktion_engine::Track *track, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto *info : trackInfos)
    {
        if (info->track == track)
        {
            return currentY;
        }
        currentY += getTrackHeight(info->track, true);
    }

    return -1; // Track nicht gefunden
}

bool TrackHeightManager::isTrackMinimized(tracktion_engine::Track *track)
{
    TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return false;

    return trackInfo->isMinimized;
}

void TrackHeightManager::setMinimized(tracktion_engine::Track *track, bool minimized)
{
    TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    trackInfo->isMinimized = minimized;

    triggerFlashState();
}

bool TrackHeightManager::isTrackInMinimizedFolderRecursive(tracktion_engine::Track *track)
{
    auto *parentFolder = track->getParentFolderTrack();
    while (parentFolder != nullptr)
    {
        if (isTrackMinimized(parentFolder))
            return true;

        parentFolder = parentFolder->getParentFolderTrack();
    }

    return false;
}

void TrackHeightManager::setAutomationHeight(const tracktion_engine::AutomatableParameter::Ptr ap, int height)
{
    if (ap == nullptr)
        return;

    tracktion_engine::Track *track = ap->getTrack();
    if (track == nullptr)
        return;

    TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    trackInfo->automationParameterHeights[ap] = height;

    GUIHelpers::log("ParameterHeight set to: ", height);
    triggerFlashState();
}
tracktion::Track::Ptr TrackHeightManager::getTrackFromID(tracktion_engine::Edit &edit, const tracktion_engine::EditItemID &id)
{
    tracktion::Track::Ptr foundTrack;

    edit.visitAllTracks(
        [&](tracktion_engine::Track &track)
        {
            if (track.itemID == id)
            {
                foundTrack = track;
                return false;
            }

            return true;
        },
        true);

    return foundTrack;
}
void TrackHeightManager::addTrackInfo(TrackHeightInfo *info) { trackInfos.add(info); }

TrackHeightManager::TrackHeightInfo *TrackHeightManager::getTrackInfoForTrack(tracktion_engine::Track *track) const
{
    for (auto *info : trackInfos)
    {
        if (info->track == track)
            return info;
    }
    return nullptr;
}

tracktion_engine::AutomatableParameter::Ptr TrackHeightManager::findAutomatableParameterByID(tracktion_engine::Track *track, const juce::String &paramID)
{
    for (auto *parameter : track->getAllAutomatableParams())
    {
        parameter->getPlugin()->getIdentifierString();
        if (parameter->paramID == paramID)
        {
            return tracktion::AutomatableParameter::Ptr(parameter);
        }
    }

    return nullptr;
}

void TrackHeightManager::flashStateFromTrackInfos()
{
    GUIHelpers::log("Update State from TrackInfos");
    // Sort TrackInfos by hierarchy depth so parent folders are processed first
    TrackHeightInfoComparator comparator;
    trackInfos.sort(comparator);

    // First pass: Set individual track states
    for (const auto *info : trackInfos)
    {
        auto *track = info->track;
        if (!track)
            continue;

        // Update the minimized state
        track->state.setProperty(IDs::isTrackMinimized, info->isMinimized, nullptr);

        if (info->type == TrackType::Folder)
        {
            // FolderTracks always have a fixed height
            track->state.setProperty(tracktion_engine::IDs::height, folderTrackHeight, nullptr);
        }
        else if (info->type == TrackType::Audio)
        {
            track->state.setProperty(tracktion_engine::IDs::height, info->baseHeight, nullptr);
        }

        // Update individual automation parameter heights
        for (const auto &[ap, height] : info->automationParameterHeights)
        {
            if (ap)
            {
                GUIHelpers::log("SAVE HEIGHT TO STATE/  HEIGHT: ", height);
                ap->getCurve().state.setProperty(tracktion_engine::IDs::height, height, nullptr);
            }
        }
    }
}
void TrackHeightManager::setTrackHeight(tracktion_engine::Track *track, int height)
{
    if (track == nullptr)
        return;

    TrackHeightInfo *trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    height = juce::jlimit(30, 300, height);
    trackInfo->baseHeight = height;

    triggerFlashState();
}
void TrackHeightManager::triggerFlashState()
{
    sendChangeMessage();
    GUIHelpers::log("TrackHeightManager: flash state triggered");
    pendingFlashState = true;
    startTimer(100);
}
void TrackHeightManager::timerCallback()
{
    stopTimer();

    GUIHelpers::log("TrackHeightManager: timer callback");
    if (pendingFlashState)
    {
        GUIHelpers::log("TrackHeightManager: pending state: true");
        flashStateFromTrackInfos();
        pendingFlashState = false;
    }
}

int TrackHeightManager::calculateHierarchyDepth(tracktion_engine::Track *track)
{
    int depth = -1;
    while (track)
    {
        track = track->getParentFolderTrack();
        depth++;
    }
    return depth;
}

void TrackHeightManager::regenerateTrackHeightsFromStates(const juce::Array<tracktion_engine::Track *> &allTracks)
{
    GUIHelpers::log("Update TrackInfo from State");
    trackInfos.clear();

    for (auto *track : allTracks)
    {
        if (track == nullptr)
            continue;
        if (!(track->isAudioTrack() || track->isFolderTrack()))
            continue;

        auto *info = new TrackHeightManager::TrackHeightInfo();
        info->track = track;
        info->type = track->isFolderTrack() ? TrackType::Folder : TrackType::Audio;
        info->isMinimized = track->state.getProperty(IDs::isTrackMinimized, false);
        info->baseHeight = track->isFolderTrack() ? folderTrackHeight : static_cast<int>(track->state.getProperty(tracktion_engine::IDs::height, 50));
        info->hierarchyDepth = calculateHierarchyDepth(track);
        info->parentFolder = track->getParentFolderTrack();

        for (auto *ap : track->getAllAutomatableParams())
        {
            if (ap->getCurve().getNumPoints() == 0)
                continue;

            int height = static_cast<int>(ap->getCurve().state.getProperty(tracktion_engine::IDs::height, 50));
            info->automationParameterHeights[ap] = height;
        }

        addTrackInfo(info);
    }
}
bool TrackHeightManager::isAutomationVisible(const tracktion_engine::AutomatableParameter &ap)
{
    if (ap.getCurve().getNumPoints() == 0)
        return false;

    if (isTrackMinimized(ap.getTrack()))
        return false;

    std::function<bool(tracktion_engine::Track *)> isTrackInMinimizedFolder = [&](tracktion_engine::Track *track) -> bool
    {
        if (track->isPartOfSubmix())
        {
            auto folderTrack = track->getParentFolderTrack();

            if (folderTrack->state.getProperty(IDs::isTrackMinimized))
                return true;

            return isTrackInMinimizedFolder(folderTrack);
        }
        return false;
    };

    return !isTrackInMinimizedFolder(ap.getTrack());
}

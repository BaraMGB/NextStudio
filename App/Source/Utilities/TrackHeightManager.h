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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
enum class TrackType
{
    Folder,
    Audio,
    Master,
    Other // For future extensions
};

constexpr int folderTrackHeight = 30;
constexpr int trackMinimizedHeight = 30;
constexpr int trackMinHeight = 50;
constexpr int trackMaxHeight = 300;

//-------------------------------------------------------------------------------------------
//
//   TrackHeightManager is an handy object, that keeps track of the tracks height.
//   When a track height is changed it will asychron update the state of the track
//   in the edit state. For a performant value change we use this object for get the
//   tracks height in the Song Editor and in the TracksListComponent. We can very fast
//   change the height and update the painting in the Song Editor. The edit state is
//   only saved when the value is not changing anymore.
//-------------------------------------------------------------------------------------------
class TrackHeightManager
    : public juce::Timer
    , public juce::ChangeBroadcaster
{
public:
    const int getTrackMinimizedHeight() { return trackMinimizedHeight; }

    struct TrackHeightInfo
    {
        tracktion_engine::Track *track = nullptr;
        TrackType type = TrackType::Audio;
        bool isMinimized = false;
        int baseHeight = 50;                                                            // Base height without considering automation lanes
        std::map<tracktion::AutomatableParameter::Ptr, int> automationParameterHeights; // Map of parameter ID to height
        int hierarchyDepth = 0;                                                         // Hierarchy level (0 = root)
        tracktion_engine::Track *parentFolder = nullptr;

        // Helper methods for quick checks
        bool isInFolder() const { return parentFolder != nullptr; }
        bool isVisible() const { return !isMinimized && hierarchyDepth >= 0; }
    };

    TrackHeightManager(const juce::Array<tracktion_engine::Track *> &allTracks);

    void flashStateFromTrackInfos();
    void regenerateTrackHeightsFromStates(const juce::Array<tracktion_engine::Track *> &allTracks);
    void regenerateTrackHeightsFromEdit(tracktion_engine::Edit &edit);

    TrackHeightInfo *getTrackInfoForTrack(tracktion_engine::Track *track) const;
    tracktion_engine::AutomatableParameter::Ptr findAutomatableParameterByID(tracktion_engine::Track *track, const juce::String &paramID);

    int getTrackHeight(tracktion_engine::Track *track, bool withAutomation);
    void setTrackHeight(tracktion_engine::Track *track, int height);
    tracktion_engine::Track *getTrackForY(int y, int scrollOffsetY);
    int getYForTrack(tracktion_engine::Track *track, int scrollOffsetY);

    int getAutomationHeight(tracktion_engine::AutomatableParameter *ap);
    void setAutomationHeight(const tracktion_engine::AutomatableParameter::Ptr ap, int height);
    tracktion_engine::AutomatableParameter::Ptr getAutomatableParameterForY(int y, int scrollOffsetY);
    int getYForAutomatableParameter(tracktion_engine::Track *track, const tracktion::AutomatableParameter::Ptr ap, int scrollOffsetY);

    juce::Array<tracktion::EditItemID> getShowedTracks(tracktion::Edit &edit);
    bool isTrackShowable(tracktion_engine::Track::Ptr track);
    bool isTrackMinimized(tracktion_engine::Track *track);
    void setMinimized(tracktion_engine::Track *track, bool minimized);
    bool isTrackInMinimizedFolderRecursive(tracktion_engine::Track *track);
    tracktion::Track::Ptr getTrackFromID(tracktion_engine::Edit &edit, const tracktion_engine::EditItemID &id);

    bool isAutomationVisible(const tracktion_engine::AutomatableParameter &ap);

    void triggerFlashState();
    void timerCallback() override;

private:
    void addTrackInfo(TrackHeightInfo *info);
    int calculateHierarchyDepth(tracktion_engine::Track *track);

    juce::Array<TrackHeightInfo *> trackInfos;
    bool pendingFlashState = false;

    class TrackHeightInfoComparator
    {
    public:
        int compareElements(TrackHeightInfo *first, TrackHeightInfo *second) const
        {
            if (first->hierarchyDepth < second->hierarchyDepth)
                return -1;
            if (first->hierarchyDepth > second->hierarchyDepth)
                return 1;
            return 0;
        }
    };
};

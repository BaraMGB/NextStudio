/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
enum class TrackType {
    Folder,
    Audio,
    Other  // For future extensions
};



constexpr int folderTrackHeight = 30;
constexpr int trackMinimizedHeight = 30;
constexpr int trackMinHeight = 50;
constexpr int trackMaxHeight = 300;

class TrackHeightManager : public juce::Timer
{
public:
    const int getTrackMinimizedHeight() { return trackMinimizedHeight; }

    struct TrackHeightInfo {
        tracktion_engine::Track* track = nullptr;
        TrackType type = TrackType::Audio;
        bool isMinimized = false;
        int baseHeight = 50;        // Base height without considering parent folders
        std::map<juce::String, int> automationParameterHeights; // Map of parameter ID to height
        int hierarchyDepth = 0;     // Hierarchy level (0 = root)
        tracktion_engine::Track* parentFolder = nullptr;

        // Helper methods for quick checks
        bool isInFolder() const { return parentFolder != nullptr; }
        bool isVisible() const { return !isMinimized && hierarchyDepth >= 0; }
    };

    TrackHeightManager(const juce::Array<tracktion_engine::Track*>& allTracks);
    ~TrackHeightManager() override;

    void flashStateFromTrackInfos();
    void regenerateTrackHeightsFromStates(const juce::Array<tracktion_engine::Track*>& allTracks);

    TrackHeightInfo* getTrackInfoForTrack(tracktion_engine::Track* track) const;
    tracktion_engine::AutomatableParameter* findAutomatableParameterByID(tracktion_engine::Track* track, const juce::String& paramID);

    int getTrackHeight(tracktion_engine::Track* track, bool withAutomation);
    tracktion_engine::Track* getTrackForY(int y, int scrollOffsetY);
    int getYForTrack(tracktion_engine::Track* track, int scrollOffsetY);

    tracktion_engine::AutomatableParameter* getAutomatableParameterForY(int y, int scrollOffsetY);
    int getYForAutomatableParameter(tracktion_engine::Track* track, const juce::String& paramID, int scrollOffsetY);

    bool isTrackMinimized(tracktion_engine::Track* track);
    void setMinimized(tracktion_engine::Track* track, bool minimized);

    void setAutomationHeight(const tracktion_engine::AutomatableParameter* ap, int height);
    void setTrackHeight(tracktion_engine::Track* track, int height);


    void triggerFlashState();
    void timerCallback() override;
private:

    void addTrackInfo(TrackHeightInfo* info);
    int calculateHierarchyDepth(tracktion_engine::Track* track);

    juce::Array<TrackHeightInfo*> trackInfos;
    bool pendingFlashState = false;

    class TrackHeightInfoComparator 
    {
    public:
        int compareElements(TrackHeightInfo* first, TrackHeightInfo* second) const 
        {
            if (first->hierarchyDepth < second->hierarchyDepth) return -1;
            if (first->hierarchyDepth > second->hierarchyDepth) return 1;
            return 0;
        }
    };
};

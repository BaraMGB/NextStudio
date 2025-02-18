#include "TrackHeightManager.h"
#include "EditViewState.h"




TrackHeightManager::TrackHeightManager(const juce::Array<tracktion_engine::Track*>& allTracks)
{
    GUIHelpers::log("TrackHeightManager: Constructor called");
    regenerateTrackHeightsFromStates(allTracks);
}

TrackHeightManager::~TrackHeightManager()
{
    GUIHelpers::log("TrackHeightManager: Destructor called");
}
int TrackHeightManager::getTrackHeight(tracktion_engine::Track* track, bool withAutomation)
{
    if (track == nullptr)
        return 0;

    TrackHeightInfo* trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return 0;

    if (trackInfo->isMinimized)
        return trackMinimizedHeight;

    int totalHeight = trackInfo->baseHeight;

    if (trackInfo->type == TrackType::Folder)
        totalHeight = folderTrackHeight;

    if (withAutomation)
        for (const auto& paramHeight : trackInfo->automationParameterHeights)
            totalHeight += paramHeight.second;

    return totalHeight;
}

tracktion_engine::Track* TrackHeightManager::getTrackForY(int y, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto* info : trackInfos)
    {
        int trackHeight = info->baseHeight;

        if (y >= currentY && y < currentY + trackHeight)
        {
            return info->track;
        }

        int paramY = currentY + trackHeight;
        for (const auto& [paramID, height] : info->automationParameterHeights)
        {
            if (y >= paramY && y < paramY + height)
            {
                return nullptr; // y liegt in einem Automationsbereich
            }
            paramY += height;
        }

        currentY += getTrackHeight(info->track, true);
    }

    return nullptr;
}

tracktion_engine::AutomatableParameter* TrackHeightManager::getAutomatableParameterForY(int y, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto* info : trackInfos)
    {
        int trackHeight = info->baseHeight;

        if (y >= currentY && y < currentY + trackHeight)
        {
            int paramY = currentY + trackHeight;

            for (const auto& [paramID, height] : info->automationParameterHeights)
            {
                if (y >= paramY && y < paramY + height)
                {
                    return findAutomatableParameterByID(info->track, paramID);
                }
                paramY += height;
            }
        }

        currentY += getTrackHeight(info->track, true);
    }

    return nullptr;
}
int TrackHeightManager::getYForAutomatableParameter(tracktion_engine::Track* track, const juce::String& paramID, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto* info : trackInfos)
    {
        int trackHeight = info->baseHeight;

        if (info->track == track)
        {
            int paramY = currentY + trackHeight;

            for (const auto& [storedParamID, height] : info->automationParameterHeights)
            {
                if (storedParamID == paramID)
                {
                    return paramY;
                }
                paramY += height;
            }
        }

        currentY += getTrackHeight(info->track, true);
    }

    return -1;
}
int TrackHeightManager::getYForTrack(tracktion_engine::Track* track, int scrollOffsetY)
{
    int currentY = -scrollOffsetY;

    for (auto* info : trackInfos)
    {
        if (info->track == track)
        {
            return currentY;
        }
        currentY += getTrackHeight(info->track, true);
    }

    return -1; // Track nicht gefunden
}

bool TrackHeightManager::isTrackMinimized(tracktion_engine::Track* track)
{
    TrackHeightInfo* trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return false;

    return trackInfo->isMinimized;
}

void TrackHeightManager::setMinimized(tracktion_engine::Track* track, bool minimized)
{
    TrackHeightInfo* trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    juce::String minimizedStr = minimized ? "true" : "false";
    GUIHelpers::log("TrackHeightManager: track is minimized now? " + minimizedStr);
    trackInfo->isMinimized = minimized;

    triggerFlashState();
}
void TrackHeightManager::setAutomationHeight(const tracktion_engine::AutomatableParameter* ap, int height)
{
    if (ap == nullptr)
        return;

    tracktion_engine::Track* track = ap->getTrack();
    if (track == nullptr)
        return;

    TrackHeightInfo* trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    trackInfo->automationParameterHeights[ap->paramID] = height;

    triggerFlashState();
}

void TrackHeightManager::addTrackInfo(TrackHeightInfo* info)

{
    trackInfos.add(info);
}

TrackHeightManager::TrackHeightInfo* TrackHeightManager:: getTrackInfoForTrack(tracktion_engine::Track* track) const
{
    for (auto* info : trackInfos)
    {
        if (info->track == track)
            return info;
    }
    return nullptr;
}

tracktion_engine::AutomatableParameter* TrackHeightManager::findAutomatableParameterByID(tracktion_engine::Track* track, const juce::String& paramID) 
{
    for (auto* parameter : track->getAllAutomatableParams()) 
    {
        if (parameter->paramID == paramID) 
        {
            return parameter;
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
    for (const auto* info : trackInfos)
    {
        auto* track = info->track;
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
        for (const auto& [paramID, height] : info->automationParameterHeights)
        {
            tracktion_engine::AutomatableParameter* parameter = findAutomatableParameterByID(track, paramID);
            if (parameter)
            {
                GUIHelpers::log("SAVE HEIGHT TO STATE" + parameter->getFullName());
                parameter->getCurve().state.setProperty(tracktion_engine::IDs::height, height, nullptr);
            }
        }
    }

    for (const auto* info : trackInfos)
    {
        auto* track = info->track;
        if (!track || info->type == TrackType::Folder)
            continue;

        bool isInMinimizedFolder = false;
        tracktion_engine::Track* parent = info->parentFolder;
        while (parent)
        {
            if (parent->state.getProperty(IDs::isTrackMinimized, false))
            {
                isInMinimizedFolder = true;
                break;
            }
            parent = parent->getParentFolderTrack();
        }

        if (isInMinimizedFolder)
        {
            // track->state.setProperty(tracktion_engine::IDs::height, 0, nullptr);
        }
    }
}
void TrackHeightManager::setTrackHeight(tracktion_engine::Track* track, int height)
{
    if (track == nullptr)
        return;

    TrackHeightInfo* trackInfo = getTrackInfoForTrack(track);
    if (trackInfo == nullptr)
        return;

    height = juce::jlimit(30, 300, height);  // Begrenze die Höhe zwischen 30 und 300
    trackInfo->baseHeight = height;

    triggerFlashState();
}
void TrackHeightManager::triggerFlashState()
{
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

int TrackHeightManager::calculateHierarchyDepth(tracktion_engine::Track* track)
{
    int depth = -1;
    while (track)
    {
        track = track->getParentFolderTrack();
        depth++;
    }
    return depth;
}

void TrackHeightManager::regenerateTrackHeightsFromStates(const juce::Array<tracktion_engine::Track*>& allTracks)
{
    GUIHelpers::log("Update TrackINfo from State");
    trackInfos.clear();

    for (auto* track : allTracks)
    {
        if (track == nullptr)
            continue;
        if (!(track->isAudioTrack() || track->isFolderTrack()))
            continue;

        auto* info = new TrackHeightManager::TrackHeightInfo();
        info->track = track;
        info->type = track->isFolderTrack() ? TrackType::Folder : TrackType::Audio;
        info->isMinimized = track->state.getProperty(IDs::isTrackMinimized, false);
        info->baseHeight = static_cast<int>(track->state.getProperty(tracktion_engine::IDs::height, 50));
        info->hierarchyDepth = calculateHierarchyDepth(track);
        info->parentFolder = track->getParentFolderTrack();

        for (auto* ap : track->getAllAutomatableParams())
        {
            if (GUIHelpers::isAutomationVisible(*ap))
            {
                int height = static_cast<int>(ap->getCurve().state.getProperty(tracktion_engine::IDs::height, 50));
                GUIHelpers::log("SAVE HEIGHT INTO INFO: ", height);
                info->automationParameterHeights[ap->paramID] = height;
            }
        }

        addTrackInfo(info);
    }
}

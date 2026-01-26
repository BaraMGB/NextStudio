
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

#include "TrackLaneComponent.h"

TrackLaneComponent::TrackLaneComponent(EditViewState &evs, te::Track::Ptr track, juce::String timelineID)
    : m_editViewState(evs), m_track(track), m_timeLineID(timelineID)
{
    GUIHelpers::log("TrackLane gebaut!");
    setInterceptsMouseClicks(false, true);
    buildAutomationLanes();
}
void TrackLaneComponent::paint(juce::Graphics &g)
{
    if (m_track == nullptr)
        return;

    auto visibleTimeRange = m_editViewState.getVisibleTimeRange(m_timeLineID, getWidth());
    if (auto clipTrack = dynamic_cast<te::ClipTrack *>(m_track.get())) {
        float clipTrackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        auto clipArea = getLocalBounds().removeFromTop(clipTrackHeight).toFloat();
        GUIHelpers::drawTrack(g, *this, m_editViewState, clipArea, clipTrack, visibleTimeRange);
    }
    else if (m_track->isFolderTrack() || m_track->isMasterTrack()) {
        float trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        auto area = getLocalBounds().removeFromTop(trackHeight).toFloat();
        auto x1beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        auto x2beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour());
        g.fillRect(area);
        GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, x1beats, x2beats, area);
    }
}

void TrackLaneComponent::resized()
{
    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    const int minimizedHeigth = 30;

    auto rect = getLocalBounds();
    float trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
    rect.removeFromTop(trackHeight);

    for (auto *al : m_automationLanes) {
        auto ap = al->getParameter();
        int automationHeight = trackInfo->automationParameterHeights[ap];
        automationHeight = automationHeight < minimizedHeigth ? minimizedHeigth : automationHeight;
        automationHeight = trackInfo->isMinimized ? 0 : automationHeight;
        al->setBounds(rect.removeFromTop(automationHeight));
    }
}

void TrackLaneComponent::buildAutomationLanes()
{
    m_automationLanes.clear(true);

    m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromStates(tracktion::getAllTracks(m_track->edit));

    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    juce::Array<te::AutomatableParameter *> params;
    for (const auto &[ap, height] : trackInfo->automationParameterHeights)
        if (ap && ap->getCurve().getNumPoints() > 0)
            params.add(ap);

    // Sort the parameters by their ID string to ensure a consistent order
    std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

    for (auto *ap : params) {
        m_automationLanes.add(new AutomationLaneComponent(m_editViewState, ap, m_timeLineID));
        addAndMakeVisible(m_automationLanes.getLast());
    }

    resized();
}

AutomationLaneComponent *TrackLaneComponent::getAutomationLane(tracktion::AutomatableParameter::Ptr ap)
{
    for (auto al : m_automationLanes)
        if (al->getParameter() == ap)
            return al;

    return nullptr;
}

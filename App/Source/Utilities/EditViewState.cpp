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

#include "Utilities/EditViewState.h"

EditViewState::EditViewState(te::Edit &e, te::SelectionManager &s, ApplicationViewState &avs)
    : m_edit(e),
      m_selectionManager(s),
      m_applicationState(avs)
{
    m_trackHeightManager = std::make_unique<TrackHeightManager>(tracktion::getAllTracks(e));
    m_trackHeightManager->regenerateTrackHeightsFromEdit(m_edit);
    m_thumbNailManager = std::make_unique<ThumbNailManager>(m_edit.engine);
    m_state = m_edit.state.getOrCreateChildWithName(IDs::EDITVIEWSTATE, nullptr);
    m_viewDataTree = m_edit.state.getOrCreateChildWithName(IDs::viewData, nullptr);
    m_pluginPresetManagerUIStates = m_state.getOrCreateChildWithName(IDs::pluginPresetManagerUIStates, nullptr);
    m_trackRackViewState = m_state.getOrCreateChildWithName(IDs::trackRackViewState, nullptr);

    auto um = &m_edit.getUndoManager();

    m_showGlobalTrack.referTo(m_state, IDs::showGlobalTrack, um, false);
    m_showMarkerTrack.referTo(m_state, IDs::showMarkerTrack, um, false);
    m_showChordTrack.referTo(m_state, IDs::showChordTrack, um, false);
    m_showArrangerTrack.referTo(m_state, IDs::showArranger, um, false);
    m_showMasterTrack.referTo(m_state, IDs::showMaster, um, false);
    m_drawWaveforms.referTo(m_state, IDs::drawWaveforms, um, /* false);*/ true);
    m_showHeaders.referTo(m_state, IDs::showHeaders, um, false); // true);
    m_showFooters.referTo(m_state, IDs::showFooters, um, false);
    m_showMidiDevices.referTo(m_state, IDs::showMidiDevices, um, false);
    m_showWaveDevices.referTo(m_state, IDs::showWaveDevices, um, true);
    m_automationFollowsClip.referTo(m_state, IDs::automationFollowsClip, um, true);

    m_trackHeightMinimized.referTo(m_state, IDs::trackMinimized, um, 30);
    m_isAutoArmed.referTo(m_state, IDs::isAutoArmed, um, true);
    m_trackDefaultHeight.referTo(m_state, IDs::headerHeight, um, 50);
    m_trackHeaderWidth.referTo(m_state, IDs::headerWidth, um, 290);
    m_folderTrackHeight.referTo(m_state, IDs::folderTrackHeight, um, 30);
    m_footerBarHeight.referTo(m_state, IDs::footerBarHeight, um, 20);
    m_lowerRangeView.referTo(m_state, IDs::lowerRangeView, um, 0);
    m_midiEditorHeight.referTo(m_state, IDs::pianorollHeight, um, 400);
    m_lastNoteLength.referTo(m_state, IDs::lastNoteLenght, um, 0);
    m_snapType.referTo(m_state, IDs::snapType, um, 9);
    m_playHeadStartTime.referTo(m_state, IDs::playHeadStartTime, um, 0.0);
    m_followPlayhead.referTo(m_state, IDs::followsPlayhead, um, true);
    m_followModeVal.referTo(m_state, IDs::followMode, um, 1); // Default to Page (1)
    m_timeLineHeight.referTo(m_state, IDs::timeLineHeight, um, 50);
    m_editName.referTo(m_state, IDs::name, um, "unknown");
    m_timeLineZoomUnit.referTo(m_state, IDs::timeLineZoomUnit, um, 50);
    m_zoomMode.referTo(m_state, IDs::zoomMode, um, "B");
    m_velocityEditorHeight.referTo(m_state, IDs::velocityEditorHeight, um, 100);
    m_lastVelocity.referTo(m_state, IDs::lastVelocity, um, 100);
    m_keyboardWidth.referTo(m_state, IDs::pianoRollKeyboardWidth, um, 120);
    m_clipHeaderHeight.referTo(m_state, IDs::clipHeaderHeight, um, 20);
    m_syncAutomation.referTo(m_state, IDs::syncAutomation, um, true);
    m_snapToGrid.referTo(m_state, IDs::snapToGrid, um, true);
    m_editNotesOutsideClipRange.referTo(m_state, IDs::editNoteOutsideOfClipRange, um, false);
}

EditViewState::~EditViewState()
{
    if (m_state.getParent().isValid())
        m_state.getParent().removeChild(m_state, nullptr);

    if (m_viewDataTree.getParent().isValid())
        m_viewDataTree.getParent().removeChild(m_viewDataTree, nullptr);
}

juce::ValueTree EditViewState::getPresetManagerUIStateForPlugin(const te::Plugin &plugin)
{
    // juce::Identifier allows only alphanumeric characters and _
    auto idStr = "p" + plugin.itemID.toString().replaceCharacters("-", "_");
    juce::Identifier id(idStr);
    return m_pluginPresetManagerUIStates.getOrCreateChildWithName(id, nullptr);
}

juce::ValueTree EditViewState::getTrackRackViewState(te::EditItemID trackID)
{
    auto idStr = "t" + trackID.toString().replaceCharacters("-", "_");
    juce::Identifier id(idStr);
    return m_trackRackViewState.getOrCreateChildWithName(id, nullptr);
}

void EditViewState::setTrackSelectedModifier(te::EditItemID trackID, te::EditItemID modifierID)
{
    auto state = getTrackRackViewState(trackID);
    state.setProperty(IDs::selectedModifier, modifierID.toString(), &m_edit.getUndoManager());
}

te::EditItemID EditViewState::getTrackSelectedModifier(te::EditItemID trackID)
{
    auto state = getTrackRackViewState(trackID);
    return te::EditItemID::fromVar(state.getProperty(IDs::selectedModifier));
}

float EditViewState::beatsToX(double beats, int width, double x1beats, double x2beats) const { return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats)); }

double EditViewState::xToBeats(float x, int width, double x1beats, double x2beats) const
{
    double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
    return beats;
}

float EditViewState::timeToX(double time, int width, double x1beats, double x2beats) const
{
    double beats = timeToBeat(time);
    return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats));
}

double EditViewState::xToTime(float x, int width, double x1beats, double x2beats) const
{
    double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
    return beatToTime(beats);
}

float EditViewState::beatsToX(double beats, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return beatsToX(beats, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToBeats(int x, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return xToBeats(x, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

float EditViewState::timeToX(double time, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return timeToX(time, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToTime(int x, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return xToTime(x, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::beatToTime(double b) const
{
    auto bp = tracktion::core::BeatPosition::fromBeats(b);
    auto &ts = m_edit.tempoSequence;
    return ts.toTime(bp).inSeconds();
}

double EditViewState::timeToBeat(double t) const
{
    auto tp = tracktion::core::TimePosition::fromSeconds(t);
    auto &ts = m_edit.tempoSequence;
    return ts.toBeats(tp).inBeats();
}

void EditViewState::setNewStartAndZoom(juce::String timeLineID, double startBeat, double beatsPerPixel)
{
    startBeat = juce::jmax(0.0, startBeat);

    auto node = m_viewDataTree.getOrCreateChildWithName(timeLineID, nullptr);
    if (node.isValid())
    {
        if (beatsPerPixel != -1)
            node.setProperty(IDs::beatsPerPixel, beatsPerPixel, nullptr);
        node.setProperty(IDs::viewX, startBeat, nullptr);
    }
}

void EditViewState::setNewBeatRange(juce::String timeLineID, tracktion::BeatRange beatRange, float width)
{
    auto node = m_viewDataTree.getOrCreateChildWithName(timeLineID, nullptr);
    if (node.isValid())
    {
        auto startBeat = beatRange.getStart().inBeats();
        auto endBeat = beatRange.getEnd().inBeats();
        auto beatsPerPixel = (endBeat - startBeat) / width;

        if (startBeat < 0)
        {
            startBeat = 0;
            endBeat = startBeat + (beatsPerPixel * width);
        }

        node.setProperty(IDs::viewX, startBeat, nullptr);
        node.setProperty(IDs::beatsPerPixel, beatsPerPixel, nullptr);
    }
}

void EditViewState::setNewTimeRange(juce::String timeLineID, tracktion::TimeRange timeRange, float width)
{
    auto node = m_viewDataTree.getOrCreateChildWithName(timeLineID, nullptr);
    if (node.isValid())
    {
        auto startBeat = timeToBeat(timeRange.getStart().inSeconds());
        auto endBeat = timeToBeat(timeRange.getEnd().inSeconds());
        auto beatsPerPixel = (endBeat - startBeat) / width;

        if (startBeat < 0)
        {
            startBeat = 0;
            endBeat = startBeat + (beatsPerPixel * width);
        }

        node.setProperty(IDs::viewX, startBeat, nullptr);
        node.setProperty(IDs::beatsPerPixel, beatsPerPixel, nullptr);
    }
}

tracktion::BeatRange EditViewState::getVisibleBeatRange(juce::String id, int width)
{
    auto node = m_viewDataTree.getChildWithName(id);
    if (node.isValid())
    {
        auto startBeat = static_cast<double>(node.getProperty(IDs::viewX, 0.0));
        auto beatsPerPixel = static_cast<double>(node.getProperty(IDs::beatsPerPixel, 0.1));
        auto endBeat = startBeat + (beatsPerPixel * width);

        return {tracktion::BeatPosition::fromBeats(startBeat), tracktion::BeatPosition::fromBeats(endBeat)};
    }
    return tracktion::BeatRange();
}

tracktion::TimeRange EditViewState::getVisibleTimeRange(juce::String id, int width)
{
    auto node = m_viewDataTree.getChildWithName(id);
    if (node.isValid())
    {
        auto startBeat = static_cast<double>(node.getProperty(IDs::viewX, 0.0));
        auto beatsPerPixel = static_cast<double>(node.getProperty(IDs::beatsPerPixel, 0.1));
        auto endBeat = startBeat + (beatsPerPixel * width);

        auto t1 = beatToTime(startBeat);
        auto t2 = beatToTime(endBeat);

        return {tracktion::TimePosition::fromSeconds(t1), tracktion::TimePosition::fromSeconds(t2)};
    }
    return tracktion::TimeRange();
}
[[nodiscard]] double EditViewState::getSnappedTime(double t, te::TimecodeSnapType snapType, bool downwards) const
{
    auto &temposequ = m_edit.tempoSequence;

    auto tp = tracktion::core::TimePosition::fromSeconds(t);
    return downwards ? snapType.roundTimeDown(tp, temposequ).inSeconds() : snapType.roundTimeNearest(tp, temposequ).inSeconds();
}

[[nodiscard]] te::TimecodeSnapType EditViewState::getBestSnapType(double beat1, double beat2, int width) const
{
    double x1time = beatToTime(beat1);
    double x2time = beatToTime(beat2);

    auto td = tracktion::core::TimeDuration::fromSeconds(x2time - x1time);

    auto pos = m_edit.getTransport().getPosition();
    te::TimecodeSnapType snaptype = m_edit.getTimecodeFormat().getBestSnapType(m_edit.tempoSequence.getTempoAt(pos), td / width, false);
    return snaptype;
}

[[nodiscard]] juce::String EditViewState::getSnapTypeDescription(int idx) const
{
    auto tp = m_edit.getTransport().getPosition();
    tracktion_engine::TempoSetting &tempo = m_edit.tempoSequence.getTempoAt(tp);
    return m_edit.getTimecodeFormat().getSnapType(idx).getDescription(tempo, false);
}

[[nodiscard]] double EditViewState::getEndScrollBeat() const { return timeToBeat(m_edit.getLength().inSeconds()) + (480); }

void EditViewState::followsPlayhead(bool shouldFollow) { m_followPlayhead = shouldFollow; }

void EditViewState::toggleFollowPlayhead() { m_followPlayhead = !m_followPlayhead; }
[[nodiscard]] bool EditViewState::viewFollowsPos() const { return m_followPlayhead; }

void EditViewState::setFollowMode(FollowMode mode)
{
    if (mode == FollowMode::Off)
    {
        m_followPlayhead = false;
    }
    else
    {
        m_followPlayhead = true;
        m_followModeVal = static_cast<int>(mode);
    }
}

EditViewState::FollowMode EditViewState::getFollowMode() const
{
    if (!m_followPlayhead)
        return FollowMode::Off;

    return static_cast<FollowMode>(m_followModeVal.get());
}

void EditViewState::updatePositionFollower(juce::String timeLineID, int width)
{
    auto mode = getFollowMode();
    if (mode == FollowMode::Off || !m_edit.getTransport().isPlaying())
    {
        m_isScrolling = false;
        return;
    }

    auto currentPos = m_edit.getTransport().getPosition().inSeconds();
    auto currentBeats = timeToBeat(currentPos);
    auto visibleRange = getVisibleBeatRange(timeLineID, width);
    auto startBeat = visibleRange.getStart().inBeats();
    auto endBeat = visibleRange.getEnd().inBeats();
    auto viewLength = endBeat - startBeat;

    // Safety check for invalid view
    if (viewLength <= 0)
        return;

    if (mode == FollowMode::Continuous)
    {
        // Center the playhead
        // Smoothness comes from calling this at 60fps
        setNewStartAndZoom(timeLineID, juce::jmax(0.0, currentBeats - viewLength / 2.0));
    }
    else if (mode == FollowMode::Page)
    {
        // Page Mode with Smooth Transition

        // Check if we need to scroll (Playhead moved out of view)
        if (!m_isScrolling)
        {
            // Margin of 10% of view width
            double margin = viewLength * 0.10;

            if (currentBeats >= endBeat - margin)
            {
                m_targetViewX = endBeat - margin;
                m_scrollStartViewX = startBeat;
                m_scrollProgress = 0.0;
                m_isScrolling = true;
            }
            else if (currentBeats < startBeat)
            {
                // Jumped back (loop or user click)
                m_targetViewX = juce::jmax(0.0, currentBeats - margin);
                m_scrollStartViewX = startBeat;
                m_scrollProgress = 0.0;
                m_isScrolling = true;
            }
        }

        if (m_isScrolling)
        {
            // Fixed duration scroll with Ease-In / Ease-Out
            // Increment progress (0.025 gives approx 40 frames = 0.66 seconds at 60Hz)
            m_scrollProgress += 0.025;

            if (m_scrollProgress >= 1.0)
            {
                setNewStartAndZoom(timeLineID, m_targetViewX);
                m_isScrolling = false;
            }
            else
            {
                // SmootherStep (Perlin): t^3 * (t * (t * 6 - 15) + 10)
                // Provides zero 1st and 2nd derivative at t=0 and t=1 for softer start/stop.
                double t = m_scrollProgress;
                double ease = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);

                double newStart = m_scrollStartViewX + (m_targetViewX - m_scrollStartViewX) * ease;
                setNewStartAndZoom(timeLineID, newStart);
            }
        }
    }
}

SimpleThumbnail *EditViewState::getOrCreateThumbnail(te::WaveAudioClip::Ptr wac) { return m_thumbNailManager->getOrCreateThumbnail(wac); }
void EditViewState::clearThumbnails() { m_thumbNailManager->clearThumbnails(); }
void EditViewState::removeThumbnail(te::EditItemID id) { m_thumbNailManager->removeThumbnail(id); }

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

#include "EditViewState.h"

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
    m_trackPluginChainViewState = m_state.getOrCreateChildWithName(IDs::trackPluginChainViewState, nullptr);

    // View and editor state should persist with the edit, but should not affect the user's undo history.
    juce::UndoManager *um = nullptr;

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
    m_playHeadStartTime.referTo(m_state, IDs::playHeadStartTime, nullptr, 0.0);
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
    m_pianoRollSnapMode.referTo(m_state, IDs::pianoRollSnapMode, um, static_cast<int>(PianoRollSnapMode::adaptive));
    m_pianoRollSnapDenominator.referTo(m_state, IDs::pianoRollSnapDenominator, um, 16);
    m_pianoRollNoteLengthMode.referTo(m_state, IDs::pianoRollNoteLengthMode, um, static_cast<int>(PianoRollNoteLengthMode::lastInserted));
    m_pianoRollNoteLengthDenominator.referTo(m_state, IDs::pianoRollNoteLengthDenominator, um, 16);
    m_clipSnapMode.referTo(m_state, IDs::clipSnapMode, um, static_cast<int>(PianoRollSnapMode::adaptive));
    m_clipSnapDenominator.referTo(m_state, IDs::clipSnapDenominator, um, 16);
    m_clipInsertLengthMode.referTo(m_state, IDs::clipInsertLengthMode, um, static_cast<int>(ClipInsertLengthMode::fixed));
    m_clipInsertLengthDenominator.referTo(m_state, IDs::clipInsertLengthDenominator, um, 1);
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

juce::ValueTree EditViewState::getTrackPluginChainViewState(te::EditItemID trackID)
{
    auto idStr = "t" + trackID.toString().replaceCharacters("-", "_");
    juce::Identifier id(idStr);
    return m_trackPluginChainViewState.getOrCreateChildWithName(id, nullptr);
}

void EditViewState::setTrackSelectedModifier(te::EditItemID trackID, te::EditItemID modifierID)
{
    auto state = getTrackPluginChainViewState(trackID);
    state.setProperty(IDs::selectedModifier, modifierID.toString(), nullptr);
}

te::EditItemID EditViewState::getTrackSelectedModifier(te::EditItemID trackID)
{
    auto state = getTrackPluginChainViewState(trackID);
    return te::EditItemID::fromVar(state.getProperty(IDs::selectedModifier));
}

float EditViewState::beatsToX(double beats, double width, double x1beats, double x2beats) const
{
    if (width <= 0 || x2beats <= x1beats)
        return 0;
    return static_cast<float>(TimelineViewGeometry::beatToX(beats, x1beats, (x2beats - x1beats) / width));
}

double EditViewState::xToBeats(float x, double width, double x1beats, double x2beats) const
{
    if (width <= 0)
        return x1beats;
    return TimelineViewGeometry::xToBeat(x, x1beats, (x2beats - x1beats) / width);
}

float EditViewState::timeToX(double time, double width, double x1beats, double x2beats) const { return beatsToX(timeToBeat(time), width, x1beats, x2beats); }

double EditViewState::xToTime(float x, double width, double x1beats, double x2beats) const { return beatToTime(xToBeats(x, width, x1beats, x2beats)); }

float EditViewState::beatsToX(double beats, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return beatsToX(beats, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToBeats(float x, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return xToBeats(x, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

float EditViewState::timeToX(double time, const juce::String &timeLineID, int width)
{
    auto visibleBeats = getVisibleBeatRange(timeLineID, width);
    return timeToX(time, width, visibleBeats.getStart().inBeats(), visibleBeats.getEnd().inBeats());
}

double EditViewState::xToTime(float x, const juce::String &timeLineID, int width)
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

TimelineViewGeometry::ViewportContext EditViewState::getTimelineViewport(const juce::String &id) const
{
    auto found = m_timelineContexts.find(id);
    auto context = found == m_timelineContexts.end() ? TimelineViewGeometry::ViewportContext{} : found->second;
    context.beatsPerBar = m_edit.tempoSequence.getTimeSigAt(tracktion::TimePosition()).numerator;
    return context;
}

uint64_t EditViewState::getTimelineRevision(const juce::String &id) const
{
    auto found = m_timelineRevisions.find(id);
    return found == m_timelineRevisions.end() ? 0 : found->second;
}

void EditViewState::writeTimelineView(const juce::String &id, double start, double b)
{
    if (!std::isfinite(start) || !std::isfinite(b) || b <= 0)
        return;
    auto node = m_viewDataTree.getOrCreateChildWithName(id, nullptr);
    start = juce::jmax(0.0, start);
    if (double(node.getProperty(IDs::viewX, -1.0)) == start && double(node.getProperty(IDs::beatsPerPixel, -1.0)) == b)
        return;
    ++m_timelineRevisions[id];
    // View listeners coalesce these notifications via AsyncUpdater; no edit undo.
    node.setProperty(IDs::beatsPerPixel, b, nullptr);
    node.setProperty(IDs::viewX, start, nullptr);
}

void EditViewState::configureTimelineViewport(const juce::String &id, double width, double rasterScale)
{
    if (!std::isfinite(width) || width <= 0 || !std::isfinite(rasterScale) || rasterScale <= 0)
        return;
    m_timelineContexts[id] = {width, rasterScale, getTimelineViewport(id).beatsPerBar};
    if (auto fit = m_pendingTimelineFits.find(id); fit != m_pendingTimelineFits.end())
    {
        const auto pending = fit->second;
        m_pendingTimelineFits.erase(fit);
        applyTimelineZoom(id, {pending.length / (width * 0.8), pending.start + pending.length / 2, width / 2, TimelineViewGeometry::ZoomPolicy::fit});
        return;
    }
    auto node = m_viewDataTree.getOrCreateChildWithName(id, nullptr);
    auto b = double(node.getProperty(IDs::beatsPerPixel, 0.1));
    if (!std::isfinite(b) || b <= 0)
        b = 0.1;
    auto start = double(node.getProperty(IDs::viewX, 0.0));
    if (!std::isfinite(start))
        start = 0;
    // Large fitted views must survive passive refresh/restore. Interactive
    // zoom still uses the bounded nearest policy when the user next zooms.
    const auto policy = b > TimelineViewGeometry::maximumVisibleBeats / width ? TimelineViewGeometry::ZoomPolicy::fit : TimelineViewGeometry::ZoomPolicy::nearest;
    applyTimelineZoom(id, {b, start, 0, policy});
}

void EditViewState::applyTimelineZoom(const juce::String &id, TimelineViewGeometry::ZoomRequest request)
{
    if (!std::isfinite(request.beatsPerPixel) || request.beatsPerPixel <= 0 || !std::isfinite(request.anchorBeat) || !std::isfinite(request.anchorX))
        return;
    m_pendingTimelineFits.erase(id);
    auto context = getTimelineViewport(id);
    if (context.width <= 0)
    {
        // An owner may request a view before layout. Normalize once its context exists.
        writeTimelineView(id, request.anchorBeat - request.anchorX * request.beatsPerPixel, request.beatsPerPixel);
        return;
    }
    if (auto view = TimelineViewGeometry::normalize(context, request))
        writeTimelineView(id, view->startBeat, view->beatsPerPixel);
}

void EditViewState::setNewStartAndZoom(juce::String id, double start, double b)
{
    if (b != -1)
        applyTimelineZoom(id, {b, start, 0});
    else
    {
        if (!std::isfinite(start))
            return;
        m_pendingTimelineFits.erase(id);
        auto node = m_viewDataTree.getOrCreateChildWithName(id, nullptr);
        writeTimelineView(id, start, double(node.getProperty(IDs::beatsPerPixel, 0.1)));
    }
}

void EditViewState::setNewBeatRange(juce::String id, tracktion::BeatRange range, float width)
{
    if (!std::isfinite(width) || width <= 0 || range.getLength().inBeats() <= 0)
        return;
    auto context = getTimelineViewport(id);
    m_timelineContexts[id] = {width, context.rasterScale, context.beatsPerBar};
    applyTimelineZoom(id, {range.getLength().inBeats() / width, range.getCentre().inBeats(), width / 2.0, TimelineViewGeometry::ZoomPolicy::fit});
}

void EditViewState::setNewTimeRange(juce::String id, tracktion::TimeRange range, float width) { setNewBeatRange(id, {tracktion::BeatPosition::fromBeats(timeToBeat(range.getStart().inSeconds())), tracktion::BeatPosition::fromBeats(timeToBeat(range.getEnd().inSeconds()))}, width); }

void EditViewState::fitTimelineToClip(const juce::String &id, double start, double length, double width)
{
    if (!std::isfinite(width) || width <= 0 || !std::isfinite(start) || !std::isfinite(length) || length <= 0)
        return;
    auto context = getTimelineViewport(id);
    if (context.width > 0)
        width = context.width;
    else
        m_timelineContexts[id] = {width, context.rasterScale, context.beatsPerBar};
    applyTimelineZoom(id, {length / (width * 0.8), start + length / 2, width / 2, TimelineViewGeometry::ZoomPolicy::fit});
    // Cached contexts also belong to inactive tracks and can be stale. Always
    // finish the latest fit once the owner supplies its post-layout context.
    // Explicit pan/zoom cancels this pending request via the shared setters.
    m_pendingTimelineFits[id] = {start, length};
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

void EditViewState::beginRecordCountIn()
{
    clearRecordCountIn();

    auto &transport = m_edit.getTransport();

    if (transport.isPlaying() || transport.isRecording() || m_edit.getNumCountInBeats() <= 0)
        return;

    m_recordCountIn.active = true;
    m_recordCountIn.punchInTime = transport.getPosition();

    const auto punchInBeat = m_edit.tempoSequence.toBeats(m_recordCountIn.punchInTime);
    m_recordCountIn.visibleStartTime = m_edit.tempoSequence.toTime(punchInBeat - tracktion::BeatDuration::fromBeats(m_edit.getNumCountInBeats()));
}

void EditViewState::clearRecordCountIn()
{
    m_recordCountIn = {};
}

bool EditViewState::isRecordCountingIn()
{
    updateRecordCountIn();
    return m_recordCountIn.active;
}

int EditViewState::getRecordCountInBeatsRemaining()
{
    updateRecordCountIn();

    const auto currentPosition = m_edit.getTransport().getPosition();

    if (!m_recordCountIn.active || currentPosition < m_recordCountIn.visibleStartTime || currentPosition >= m_recordCountIn.punchInTime)
        return 0;

    const auto currentBeat = m_edit.tempoSequence.toBeats(currentPosition);
    const auto punchInBeat = m_edit.tempoSequence.toBeats(m_recordCountIn.punchInTime);
    const auto beatsRemaining = punchInBeat - currentBeat;

    return juce::jmax(1, static_cast<int>(beatsRemaining.inBeats() + 0.999999));
}

juce::String EditViewState::getRecordCountInText()
{
    updateRecordCountIn();

    if (!m_recordCountIn.active)
        return {};

    const auto currentPosition = m_edit.getTransport().getPosition();

    if (currentPosition < m_recordCountIn.visibleStartTime || currentPosition >= m_recordCountIn.punchInTime)
        return {};

    const auto currentBeat = m_edit.tempoSequence.toBeats(currentPosition);
    const auto punchInBeat = m_edit.tempoSequence.toBeats(m_recordCountIn.punchInTime);
    const auto beatsRemaining = punchInBeat - currentBeat;

    return juce::String(juce::jmax(1, static_cast<int>(beatsRemaining.inBeats() + 0.999999)));
}

void EditViewState::updateRecordCountIn()
{
    if (!m_recordCountIn.active)
        return;

    auto &transport = m_edit.getTransport();

    if (!transport.isPlaying())
    {
        clearRecordCountIn();
        return;
    }

    if (transport.getPosition() >= m_recordCountIn.punchInTime)
    {
        clearRecordCountIn();
        return;
    }
}

SimpleThumbnail *EditViewState::getOrCreateThumbnail(te::WaveAudioClip::Ptr wac) { return m_thumbNailManager->getOrCreateThumbnail(wac); }
void EditViewState::clearThumbnails() { m_thumbNailManager->clearThumbnails(); }
void EditViewState::removeThumbnail(te::EditItemID id) { m_thumbNailManager->removeThumbnail(id); }

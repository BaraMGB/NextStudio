
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

/*
  ==============================================================================

    TimeLineComponent.cpp
    Created: 20 Feb 2020 12:06:14am
    Author:  Steffen Baranowsky

  ==============================================================================
*/

#include "TimeLineComponent.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "PianoRollNoteLength.h"
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"

TimeLineComponent::TimeLineComponent(EditViewState &evs, juce::String timeLineID, bool usePianoRollSnapSettings)
    : m_evs(evs),
      m_drawLoopCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Draw, evs.m_applicationState.m_mouseCursorScale)),
      m_usePianoRollSnapSettings(usePianoRollSnapSettings),
      m_isMouseDown(false),
      m_scaleNotifier(this, [this](float) { triggerAsyncUpdate(); })
{
    setTimeLineID(timeLineID);
    m_evs.m_edit.state.addListener(this);
}

TimeLineComponent::~TimeLineComponent()
{
    cancelPendingUpdate();
    m_evs.m_edit.state.removeListener(this);
}

void TimeLineComponent::resized() { triggerAsyncUpdate(); }
void TimeLineComponent::moved() { triggerAsyncUpdate(); }
void TimeLineComponent::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    if (tree.hasType(te::IDs::TEMPO) || tree.hasType(te::IDs::TIMESIG))
        ++m_musicalSnapRevision;
    if (tree.hasType(te::IDs::TIMESIG) || (tree == m_tree && (property == IDs::beatsPerPixel || property == IDs::viewX)))
        triggerAsyncUpdate();
}
void TimeLineComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &child)
{
    if (child.hasType(te::IDs::TEMPO) || child.hasType(te::IDs::TIMESIG) || child.hasType(te::IDs::TEMPOSEQUENCE))
        ++m_musicalSnapRevision;
    if (child.hasType(te::IDs::TIMESIG))
        triggerAsyncUpdate();
}
void TimeLineComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &child, int)
{
    if (child.hasType(te::IDs::TEMPO) || child.hasType(te::IDs::TIMESIG) || child.hasType(te::IDs::TEMPOSEQUENCE))
        ++m_musicalSnapRevision;
    if (child.hasType(te::IDs::TIMESIG))
        triggerAsyncUpdate();
}
void TimeLineComponent::parentHierarchyChanged() { triggerAsyncUpdate(); }
void TimeLineComponent::handleAsyncUpdate() { updateViewportContext(); }
void TimeLineComponent::updateViewportContext()
{
    const double platformScale = getPeer() != nullptr ? getPeer()->getPlatformScaleFactor() : 1.0;
    const double scale = platformScale * juce::Desktop::getInstance().getGlobalScaleFactor() * juce::Component::getApproximateScaleFactorForComponent(this);
    m_evs.configureTimelineViewport(m_timeLineID, getWidth(), scale);
}
void TimeLineComponent::zoomByFactor(double factor, double anchorX)
{
    if (getWidth() <= 0 || !std::isfinite(factor) || factor <= 0)
        return;
    updateViewportContext();
    const double actual = getBeatsPerPixel();
    const double start = getCurrentBeatRange().getStart().inBeats();
    if (m_zoomRevision != m_evs.getTimelineRevision(m_timeLineID) || start != m_zoomStart)
        m_zoomIntent.reset();
    const auto requested = m_zoomIntent.multiply(actual, factor, m_evs.getTimelineViewport(m_timeLineID), juce::Time::getMillisecondCounterHiRes() / 1000.0);
    m_evs.applyTimelineZoom(m_timeLineID, {requested, start + anchorX * actual, anchorX});
    m_zoomIntent.applied(getBeatsPerPixel());
    m_zoomRevision = m_evs.getTimelineRevision(m_timeLineID);
    m_zoomStart = getCurrentBeatRange().getStart().inBeats();
}

void TimeLineComponent::paint(juce::Graphics &g)
{
    g.setColour(m_evs.m_applicationState.getTimeLineBackGroundColour());
    g.fillAll();
    g.setFont(12);
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

    GUIHelpers::drawBarsAndBeatLines(g, m_evs, x1beats, x2beats, getLocalBounds().toFloat(), true);

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(getLocalBounds().removeFromBottom(getHeight() / 5));

    if (m_isMouseDown)
    {
        auto mouseDown = m_evs.beatsToX(m_cachedBeat, getWidth(), x1beats, x2beats);
        g.setColour(juce::Colours::white);
        auto rect = juce::Rectangle<float>(mouseDown, 1.f, 1.f, float(getHeight()) - 1.f);
        auto bounds = getLocalBounds().toFloat();
        if (bounds.contains(rect))
        {
            g.fillRect(rect);
        }
    }

    drawLoopRange(g);
}

void TimeLineComponent::mouseMove(const juce::MouseEvent &e)
{
    m_leftResized = false;
    m_rightResized = false;

    const auto loopRange = m_evs.m_edit.getTransport().getLoopRange();
    const auto loopRangeRect = getTimeRangeRect(loopRange);
    const auto loopZone = getLocalBounds().removeFromBottom(getHeight() / 5);

    if (!loopRange.isEmpty() && loopRangeRect.contains(e.position))
    {
        m_changeLoopRange = false;
        if (e.x > loopRangeRect.getHorizontalRange().getStart() && e.x < loopRangeRect.getHorizontalRange().getStart() + 10)
        {
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
            setTooltip(GUIHelpers::translate("set loop range start", m_evs.m_applicationState));
            m_leftResized = true;
        }
        else if (e.x > loopRangeRect.getHorizontalRange().getEnd() - 10 && e.x < loopRangeRect.getHorizontalRange().getEnd())
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            setTooltip(GUIHelpers::translate("set loop range end", m_evs.m_applicationState));
            m_rightResized = true;
        }
        else
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            setTooltip(GUIHelpers::translate("move loop range", m_evs.m_applicationState));
        }
    }
    else if (loopZone.contains(e.getPosition()))
    {
        setMouseCursor(m_drawLoopCursor);
        setTooltip(GUIHelpers::translate("draw loop range", m_evs.m_applicationState));
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        setTooltip({});
    }
}

void TimeLineComponent::mouseExit(const juce::MouseEvent &e)
{
    juce::ignoreUnused(e);
    if (m_loopGesture.active())
        return;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    setTooltip({});
    m_leftResized = false;
    m_rightResized = false;
}

void TimeLineComponent::mouseDown(const juce::MouseEvent &e)
{
    // init
    m_mouseInput.remember(e);
    mouseMove(e);
    m_loopGesture.reset();
    m_cachedFollowPlayhead = m_evs.m_followPlayhead;
    m_evs.followsPlayhead(false);
    m_changeLoopRange = false;
    m_loopRangeClicked = false;
    m_isSnapping = isSnappingEnabled() && !e.mods.isShiftDown();
    m_cachedLoopRange = m_evs.m_edit.getTransport().getLoopRange();
    m_newLoopRange = m_cachedLoopRange;
    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;
    updateViewportContext();
    m_zoomIntent.reset();
    m_dragRequestedZoom = getBeatsPerPixel();
    m_dragViewport = m_evs.getTimelineViewport(m_timeLineID);
    m_dragRevision = m_evs.getTimelineRevision(m_timeLineID);
    m_dragViewStart = getCurrentBeatRange().getStart().inBeats();

    auto loopRangeArea = getTimeRangeRect(m_evs.getVisibleTimeRange(m_timeLineID, getWidth()));
    auto loopRange = m_evs.m_edit.getTransport().getLoopRange();

    if (loopRangeArea.contains(e.position))
        m_changeLoopRange = true;

    if (getTimeRangeRect(loopRange).contains(e.position))
    {
        m_changeLoopRange = false;
        m_loopRangeClicked = true;
        const auto resolver = getMouseSnapResolver();
        m_loopGesture.begin(resolver.timeToBeat((m_rightResized ? loopRange.getEnd() : loopRange.getStart()).inSeconds()),
                            e.position.x, resolver);
    }
    else if (!m_changeLoopRange)
    {
        double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

        e.source.enableUnboundedMouseMovement(true, false);

        m_isMouseDown = true;
        m_playheadClickPending = e.mods.isLeftButtonDown();
        m_cachedBeat = m_evs.xToBeats(e.getMouseDownPosition().getX(), getWidth(), x1beats, x2beats);
    }
    if (m_changeLoopRange)
    {
        m_loopCreationStartBeat = xToBeatPos(e.position.x).inBeats();
        if (m_isSnapping)
            m_loopCreationStartBeat = getMouseSnapResolver().startAtOrBefore(m_loopCreationStartBeat);
        m_loopCreationStartBeat = std::max(0.0, m_loopCreationStartBeat);
    }
}

void TimeLineComponent::modifierKeysChanged(const juce::ModifierKeys& mods)
{
    if (m_loopRangeClicked || m_changeLoopRange)
        if (auto event = m_mouseInput.withModifiers(mods); event && event->mouseWasDraggedSinceMouseDown())
            mouseDrag(*event);
}

void TimeLineComponent::mouseDrag(const juce::MouseEvent &e)
{
    m_mouseInput.remember(e);
    m_isSnapping = isSnappingEnabled() && !e.mods.isShiftDown();

    if (m_loopRangeClicked)
    {
        const auto resolver = getMouseSnapResolver();
        auto edge = tracktion::TimePosition::fromSeconds(resolver.beatToTime(
            m_loopGesture.update(e.position.x, resolver, e.mods.isShiftDown())));
        edge = juce::jlimit(tracktion::TimePosition(), te::Edit::getMaximumEditEnd(), edge);
        if (m_leftResized)
            m_newLoopRange = {std::min(edge, m_cachedLoopRange.getEnd()), std::max(edge, m_cachedLoopRange.getEnd())};
        else if (m_rightResized)
            m_newLoopRange = {std::min(m_cachedLoopRange.getStart(), edge), std::max(m_cachedLoopRange.getStart(), edge)};
        else
        {
            edge = std::min(edge, te::Edit::getMaximumEditEnd() - m_cachedLoopRange.getLength());
            m_newLoopRange = m_cachedLoopRange.movedToStartAt(edge);
        }
        m_loopGesture.setDisplayedBeat(resolver.timeToBeat(edge.inSeconds()));
        repaint();
    }
    else if (m_changeLoopRange)
    {
        auto t1 = beatToTime(tracktion::BeatPosition::fromBeats(m_loopCreationStartBeat));
        auto t2 = xToTimePos(e.position.x);
        if (m_isSnapping)
            t2 = snapTimeForMouse(t2);
        t2 = juce::jlimit(tracktion::TimePosition(), te::Edit::getMaximumEditEnd(), t2);

        if (t1 < t2)
            m_newLoopRange = {t1, t2};
        else
            m_newLoopRange = {t2, t1};

        repaint();
    }
    else
    {
        updateViewRange(e);
    }
}

void TimeLineComponent::mouseUp(const juce::MouseEvent &event)
{
    if (event.mouseWasDraggedSinceMouseDown() && (m_loopRangeClicked || m_changeLoopRange))
        mouseDrag(event);
    m_loopGesture.reset();
    m_mouseInput.reset();
    m_evs.followsPlayhead(m_cachedFollowPlayhead);
    auto &t = m_evs.m_edit.getTransport();
    if (m_loopRangeClicked || m_changeLoopRange)
    {
        if (event.mouseWasDraggedSinceMouseDown() && !m_newLoopRange.isEmpty())
            t.setLoopRange(m_newLoopRange);
    }
    else if (m_playheadClickPending && !event.mouseWasDraggedSinceMouseDown())
    {
        auto position = snapTime(beatToTime(tracktion::BeatPosition::fromBeats(m_cachedBeat)));
        position = juce::jmax(tracktion::TimePosition(), position);
        m_evs.m_playHeadStartTime = position.inSeconds();
        t.setPosition(position);
    }

    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;
    m_leftResized = false;
    m_rightResized = false;
    m_loopRangeClicked = false;
    m_isMouseDown = false;
    m_changeLoopRange = false;
    m_playheadClickPending = false;
    m_newLoopRange = {};
    m_isSnapping = true;

    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}
void TimeLineComponent::updateViewRange(const juce::MouseEvent &e)
{
    if (getWidth() <= 0)
        return;
    updateViewportContext();
    const auto context = m_evs.getTimelineViewport(m_timeLineID);
    if (context != m_dragViewport || m_dragRevision != m_evs.getTimelineRevision(m_timeLineID) || m_dragViewStart != getCurrentBeatRange().getStart().inBeats())
        m_dragRequestedZoom = getBeatsPerPixel();
    const auto dragY = e.getDistanceFromDragStartY();
    const auto dragX = e.getDistanceFromDragStartX();
    if (dragY != m_oldDragDistanceY)
    {
        const double factor = dragY > m_oldDragDistanceY ? 1.03 : 0.97;
        m_dragRequestedZoom = juce::jlimit(TimelineViewGeometry::minimumVisibleBeats / getWidth(), TimelineViewGeometry::maximumVisibleBeats / getWidth(), m_dragRequestedZoom * factor);
        const double anchorX = beatsToX(m_cachedBeat);
        m_evs.applyTimelineZoom(m_timeLineID, {m_dragRequestedZoom, m_cachedBeat, anchorX});
    }
    // Horizontal movement uses the actual normalized scale. Pan alone never changes zoom.
    const double start = getCurrentBeatRange().getStart().inBeats();
    m_evs.setNewStartAndZoom(m_timeLineID, start + (m_oldDragDistanceX - dragX) * getBeatsPerPixel());
    m_oldDragDistanceX = dragX;
    m_oldDragDistanceY = dragY;
    m_dragViewport = context;
    m_dragRevision = m_evs.getTimelineRevision(m_timeLineID);
    m_dragViewStart = getCurrentBeatRange().getStart().inBeats();
}

tracktion::BeatRange TimeLineComponent::getCurrentBeatRange()
{
    auto start = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart();
    auto length = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getLength();

    return tracktion::BeatRange(start, length);
}

tracktion::TimeRange TimeLineComponent::getCurrentTimeRange()
{
    auto start = m_evs.getVisibleTimeRange(m_timeLineID, getWidth()).getStart();
    auto length = m_evs.getVisibleTimeRange(m_timeLineID, getWidth()).getLength();

    return tracktion::TimeRange(start, length);
}

tracktion_engine::TimecodeSnapType TimeLineComponent::getBestSnapType()
{
    return static_cast<const TimeLineComponent &>(*this).getBestSnapType();
}

EditViewState &TimeLineComponent::getEditViewState() { return m_evs; }

float TimeLineComponent::timeToX(double time)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.timeToX(time, getWidth(), x1beats, x2beats);
}

void TimeLineComponent::drawLoopRange(juce::Graphics &g)
{
    const auto loopRange = (m_loopRangeClicked || m_changeLoopRange)
                             ? m_newLoopRange : m_evs.m_edit.getTransport().getLoopRange();

    const auto loopRect = getTimeRangeRect(loopRange).getIntersection(getLocalBounds().toFloat());
    const auto alpha = m_evs.m_edit.getTransport().looping ? 0.5f : 0.2f;
    g.setColour(m_evs.m_applicationState.getPrimeColour().withAlpha(alpha));
    g.fillRect(loopRect);
}

juce::Rectangle<float> TimeLineComponent::getTimeRangeRect(tracktion::TimeRange tr)
{
    auto x = timeToX(tr.getStart().inSeconds());
    auto w = timeToX(tr.getEnd().inSeconds()) - x;
    auto h = getHeight() / 5;

    return {x, float(getHeight() - h), w, float(h)};
}

tracktion::TimeDuration TimeLineComponent::xToTimeDuration(float x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimeDuration::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}

tracktion::TimePosition TimeLineComponent::beatToTime(tracktion::BeatPosition beats) { return tracktion::TimePosition::fromSeconds(m_evs.beatToTime(beats.inBeats())); }

float TimeLineComponent::beatsToX(double beats)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.beatsToX(beats, getWidth(), x1beats, x2beats);
}

tracktion::TimePosition TimeLineComponent::xToTimePos(float x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimePosition::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}
tracktion::BeatPosition TimeLineComponent::xToBeatPos(float x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    double beats = m_evs.xToBeats(x, getWidth(), x1beats, x2beats);
    return tracktion::BeatPosition::fromBeats(beats);
}

double TimeLineComponent::getBeatsPerPixel()
{
    auto visibleBeatRange = m_evs.getVisibleBeatRange(m_timeLineID, getWidth());
    double beatLength = visibleBeatRange.getLength().inBeats();
    int componentWidth = getWidth();

    if (componentWidth <= 0)
        return 0.0;

    return beatLength / componentWidth;
}
void TimeLineComponent::setTimeLineID(juce::String timeLineID)
{
    m_timeLineID = timeLineID;
    m_tree = m_evs.m_viewDataTree.getOrCreateChildWithName(timeLineID, nullptr);
    m_zoomIntent.reset();
    // Track switches can precede the new editor layout. Do not consume a
    // pending fit using the previous track's bounds; coalesce with resized().
    triggerAsyncUpdate();
}

void TimeLineComponent::setLastNoteLength(double length)
{
    // nullptr: Tool settings (like last note length) are transient and not part of the undo history
    m_tree.setProperty(IDs::lastNoteLenght, length, nullptr);
}

double TimeLineComponent::getLastNoteLength() const
{
    return m_tree.getProperty(IDs::lastNoteLenght, PianoRollNoteLength::defaultLengthBeats);
}

double TimeLineComponent::getNoteInsertLength() const
{
    return PianoRollNoteLength::resolve(
        static_cast<PianoRollNoteLengthMode>(static_cast<int>(m_evs.m_pianoRollNoteLengthMode)),
        static_cast<int>(m_evs.m_pianoRollNoteLengthDenominator),
        getLastNoteLength(),
        getAdaptiveSnapIntervalBeats());
}

double TimeLineComponent::getClipInsertLength() const
{
    if (static_cast<ClipInsertLengthMode>(static_cast<int>(m_evs.m_clipInsertLengthMode))
        == ClipInsertLengthMode::fixed)
        return 4.0 / juce::jmax(1, static_cast<int>(m_evs.m_clipInsertLengthDenominator));

    return getAdaptiveSnapIntervalBeats();
}

double TimeLineComponent::getQuantisedNoteBeat(double beat, const te::MidiClip *c, bool down) const
{
    const auto base = c->getStartBeat().inBeats() - c->getOffsetInBeats().inBeats();
    return getQuantisedBeat(base + beat, down) - base;
}

double TimeLineComponent::getQuantisedBeat(double beat, bool down) const
{
    if (!isSnappingEnabled())
        return beat;

    if (isUsingFixedSnap())
    {
        const auto interval = getSnapIntervalBeats();
        const auto gridPosition = beat / interval;
        return (down ? std::floor(gridPosition) : std::round(gridPosition)) * interval;
    }

    const auto time = tracktion::TimePosition::fromSeconds(m_evs.beatToTime(beat));
    return m_evs.timeToBeat(snapTime(time, down).inSeconds());
}

te::TimecodeSnapType TimeLineComponent::getBestSnapType() const
{
    if (isUsingFixedSnap())
    {
        const auto denominator = m_usePianoRollSnapSettings
                                     ? static_cast<int>(m_evs.m_pianoRollSnapDenominator)
                                     : static_cast<int>(m_evs.m_clipSnapDenominator);
        int level = 9;
        switch (denominator)
        {
        case 4: level = 9; break;
        case 8: level = 8; break;
        case 16: level = 7; break;
        case 32: level = 6; break;
        case 64: level = 5; break;
        case 128: level = 4; break;
        default: break;
        }
        return m_evs.m_edit.getTimecodeFormat().getSnapType(level);
    }

    const auto x1 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    const auto x2 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.getBestSnapType(x1, x2, getWidth());
}

bool TimeLineComponent::isSnappingEnabled() const
{
    const auto mode = m_usePianoRollSnapSettings
                          ? static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_pianoRollSnapMode))
                          : static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_clipSnapMode));
    return mode != PianoRollSnapMode::off;
}

bool TimeLineComponent::isUsingFixedSnap() const
{
    const auto mode = m_usePianoRollSnapSettings
                          ? static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_pianoRollSnapMode))
                          : static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_clipSnapMode));
    return mode == PianoRollSnapMode::fixed;
}

double TimeLineComponent::getSnapIntervalBeats() const
{
    if (isUsingFixedSnap())
    {
        const auto denominator = m_usePianoRollSnapSettings
                                     ? static_cast<int>(m_evs.m_pianoRollSnapDenominator)
                                     : static_cast<int>(m_evs.m_clipSnapDenominator);
        return 4.0 / juce::jmax(1, denominator);
    }

    return getAdaptiveSnapIntervalBeats();
}

double TimeLineComponent::getAdaptiveSnapIntervalBeats() const
{
    const auto x1 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    const auto x2 = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    const auto snapType = m_evs.getBestSnapType(x1, x2, getWidth());
    const auto position = m_evs.m_edit.getTransport().getPosition();
    const auto &tempo = m_evs.m_edit.tempoSequence.getTempoAt(position);
    return m_evs.m_edit.tempoSequence.toBeats(position + snapType.getApproxIntervalTime(tempo)).inBeats()
           - m_evs.m_edit.tempoSequence.toBeats(position).inBeats();
}

double TimeLineComponent::getNudgeDeltaBeats(double beat, int direction) const
{
    if (direction == 0)
        return 0.0;

    if (isUsingFixedSnap() || !isSnappingEnabled())
    {
        const auto interval = isUsingFixedSnap() ? getSnapIntervalBeats()
                                                 : 1.0 / te::Edit::ticksPerQuarterNote;
        constexpr double epsilon = 1.0e-9;
        const auto target = direction < 0
                                ? std::floor((beat - epsilon) / interval) * interval
                                : std::ceil((beat + epsilon) / interval) * interval;
        return target - beat;
    }

    const auto start = tracktion::TimePosition::fromSeconds(m_evs.beatToTime(beat));
    const auto snapped = direction < 0
                             ? getBestSnapType().roundTimeDown(start - tracktion::TimeDuration::fromSeconds(0.01), m_evs.m_edit.tempoSequence)
                             : getBestSnapType().roundTimeUp(start + tracktion::TimeDuration::fromSeconds(0.01), m_evs.m_edit.tempoSequence);
    return m_evs.timeToBeat(snapped.inSeconds()) - beat;
}

tracktion::TimePosition TimeLineComponent::snapTime(tracktion::TimePosition time, bool down) const
{
    if (!isSnappingEnabled())
        return time;

    if (isUsingFixedSnap())
    {
        const auto beat = m_evs.timeToBeat(time.inSeconds());
        return tracktion::TimePosition::fromSeconds(m_evs.beatToTime(getQuantisedBeat(beat, down)));
    }

    return tracktion::TimePosition::fromSeconds(m_evs.getSnappedTime(time.inSeconds(), getBestSnapType(), down));
}

double TimeLineComponent::getSnappedTime(double time)
{
    return snapTime(tracktion::TimePosition::fromSeconds(time), false).inSeconds();
}

TimelineSnapResolver TimeLineComponent::getMouseSnapResolver() const
{
    const auto range = m_evs.getVisibleBeatRange(m_timeLineID, getWidth());
    const auto viewport = m_evs.getTimelineViewport(m_timeLineID);
    return {m_evs.m_edit.tempoSequence,
            {isSnappingEnabled(), isUsingFixedSnap() ? getSnapIntervalBeats() : 0.0,
             getBestSnapType(), getWidth() > 0 ? range.getLength().inBeats() / getWidth() : 0.0,
             viewport.rasterScale, m_evs.getTimelineRevision(m_timeLineID), m_musicalSnapRevision,
             TimelineSoftSnap::profileForEditor(m_usePianoRollSnapSettings)}};
}

double TimeLineComponent::snapBeatForMouse(double beat) const
{
    return getMouseSnapResolver().snapBeatForMouse(beat);
}

tracktion::TimePosition TimeLineComponent::snapTimeForMouse(tracktion::TimePosition time) const
{
    const auto resolver = getMouseSnapResolver();
    return tracktion::TimePosition::fromSeconds(resolver.beatToTime(resolver.snapBeatForMouse(resolver.timeToBeat(time.inSeconds()))));
}

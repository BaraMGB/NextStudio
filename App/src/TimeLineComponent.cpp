
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
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"

TimeLineComponent::TimeLineComponent(EditViewState &evs, juce::String timeLineID, bool usePianoRollSnapSettings)
    : m_evs(evs),
      m_usePianoRollSnapSettings(usePianoRollSnapSettings),
      m_isMouseDown(false)
{
    setTimeLineID(timeLineID);
}

TimeLineComponent::~TimeLineComponent() {}

void TimeLineComponent::paint(juce::Graphics &g)
{
    g.setColour(m_evs.m_applicationState.getTimeLineBackGroundColour());
    g.fillAll();
    g.setFont(12);
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

    GUIHelpers::drawBarsAndBeatLines(g, m_evs, x1beats, x2beats, getLocalBounds().toFloat(), true);

    if (m_isMouseDown)
    {
        auto mouseDown = m_evs.beatsToX(m_cachedBeat, getWidth(), x1beats, x2beats);
        g.setColour(juce::Colours::white);
        auto rect = juce::Rectangle<int>(mouseDown, 1.f, 1.f, float(getHeight()) - 1.f);
        auto bounds = getLocalBounds();
        if (bounds.contains(rect))
        {
            g.fillRect(rect);
        }
    }

    if (m_evs.m_edit.getTransport().looping)
        g.setColour(juce::Colour(0x99FFFFFF));
    else
        g.setColour(juce::Colour(0x44FFFFFF));

    drawLoopRange(g);
}

void TimeLineComponent::mouseMove(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);

    auto loopRange = m_evs.m_edit.getTransport().getLoopRange();
    auto loopRangeRect = getTimeRangeRect(loopRange);

    if (loopRangeRect.contains(e.getPosition()))
    {
        m_changeLoopRange = false;
        if (e.x > loopRangeRect.getHorizontalRange().getStart() && e.x < loopRangeRect.getHorizontalRange().getStart() + 10)
        {
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
            m_leftResized = true;
            m_rightResized = false;
        }
        else if (e.x > loopRangeRect.getHorizontalRange().getEnd() - 10 && e.x < loopRangeRect.getHorizontalRange().getEnd())
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            m_rightResized = true;
            m_leftResized = false;
        }
        else
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            m_leftResized = false;
            m_rightResized = false;
        }
    }
}

void TimeLineComponent::mouseDown(const juce::MouseEvent &e)
{
    // init
    m_cachedFollowPlayhead = m_evs.m_followPlayhead;
    m_evs.followsPlayhead(false);
    m_changeLoopRange = false;
    m_loopRangeClicked = false;
    m_isSnapping = isSnappingEnabled() && !e.mods.isShiftDown();
    m_cachedLoopRange = m_evs.m_edit.getTransport().getLoopRange();
    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;

    auto loopRangeArea = getTimeRangeRect(m_evs.getVisibleTimeRange(m_timeLineID, getWidth()));
    auto loopRange = m_evs.m_edit.getTransport().getLoopRange();

    if (loopRangeArea.contains(e.getPosition()))
        m_changeLoopRange = true;

    if (getTimeRangeRect(loopRange).contains(e.getPosition()))
    {
        m_changeLoopRange = false;
        m_loopRangeClicked = true;
    }
    else if (!m_changeLoopRange)
    {
        double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();

        e.source.enableUnboundedMouseMovement(true, false);

        m_isMouseDown = true;
        m_cachedBeat = m_evs.xToBeats(e.getMouseDownPosition().getX(), getWidth(), x1beats, x2beats);

        if (e.getNumberOfClicks() > 1)
        {
            auto newTime = m_evs.beatToTime(m_cachedBeat);
            m_evs.m_playHeadStartTime = newTime;
            m_evs.m_edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(newTime));
        }
    }
}

void TimeLineComponent::mouseDrag(const juce::MouseEvent &e)
{
    m_draggedTime = tracktion::TimeDuration();
    m_isSnapping = isSnappingEnabled() && !e.mods.isShiftDown();

    if (m_loopRangeClicked)
    {
        auto scroll = beatsToX(0) * -1;

        m_draggedTime = xToTimeDuration(e.getDistanceFromDragStartX() - scroll);
        repaint();
    }
    else if (m_changeLoopRange)
    {
        auto t1 = xToTimePos(e.getMouseDownX());
        auto t2 = xToTimePos(e.x);

        if (m_isSnapping)
        {
            t1 = snapTime(t1, true);
            t2 = snapTime(t2, true);
        }

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

void TimeLineComponent::mouseUp(const juce::MouseEvent &)
{
    m_evs.followsPlayhead(m_cachedFollowPlayhead);
    auto &t = m_evs.m_edit.getTransport();
    if (m_loopRangeClicked)
        t.setLoopRange(getLoopRangeToBeMovedOrResized());
    else if (m_changeLoopRange)
        t.setLoopRange(m_newLoopRange);

    m_oldDragDistanceX = 0;
    m_oldDragDistanceY = 0;
    m_draggedTime = tracktion::TimeDuration();
    m_leftResized = false;
    m_rightResized = false;
    m_loopRangeClicked = false;
    m_isMouseDown = false;
    m_changeLoopRange = false;
    m_newLoopRange = {};
    m_isSnapping = true;

    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}
void TimeLineComponent::updateViewRange(const juce::MouseEvent &e)
{
    const auto sensitivity = 0.03f;

    auto oldViewRange = m_evs.getVisibleBeatRange(m_timeLineID, getWidth());

    // rescale ViewRange at beat on mouse down
    bool isNotToBig = oldViewRange.getLength().inBeats() < 100240.0;
    bool isNotToSmall = oldViewRange.getLength().inBeats() > 0.05;
    auto dragDistanceY = e.getDistanceFromDragStartY();
    auto anchorTime = tracktion::BeatPosition::fromBeats(m_cachedBeat);
    float scaleFactor = 1.0f + ((dragDistanceY > m_oldDragDistanceY) && isNotToBig ? sensitivity : ((dragDistanceY < m_oldDragDistanceY) && isNotToSmall ? -sensitivity : 0.0f));
    m_oldDragDistanceY = dragDistanceY;
    // m_oldDragDistanceY = scaleFactor == 1.0f ? m_oldDragDistanceY : dragDistanceY;

    auto newViewRange = oldViewRange.rescaled(anchorTime, scaleFactor);

    // move horizontal if mouse dragged on X-Axis
    auto newBeatsPerPixel = newViewRange.getLength().inBeats() / getWidth();
    auto dragDistanceX = m_oldDragDistanceX - e.getDistanceFromDragStartX();
    m_oldDragDistanceX = e.getDistanceFromDragStartX();
    auto moveCorrection = tracktion::BeatDuration::fromBeats(dragDistanceX * newBeatsPerPixel);

    newViewRange = newViewRange.movedToStartAt(newViewRange.getStart() + moveCorrection);

    m_evs.setNewBeatRange(m_timeLineID, newViewRange, getWidth());
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

int TimeLineComponent::timeToX(double time)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.timeToX(time, getWidth(), x1beats, x2beats);
}

void TimeLineComponent::drawLoopRange(juce::Graphics &g)
{
    juce::Rectangle<int> loopRect;

    if (m_draggedTime != tracktion::TimeDuration())
    {
        loopRect = getTimeRangeRect(getLoopRangeToBeMovedOrResized());
    }
    else if (m_changeLoopRange)
    {
        loopRect = getTimeRangeRect(m_newLoopRange);
    }
    else
    {
        auto loopRange = m_evs.m_edit.getTransport().getLoopRange();
        loopRect = getTimeRangeRect(loopRange);
    }

    loopRect = loopRect.getIntersection(getLocalBounds());

    g.setColour(m_evs.m_applicationState.getPrimeColour().withAlpha(0.5f));
    g.fillRect(loopRect);
}

juce::Rectangle<int> TimeLineComponent::getTimeRangeRect(tracktion::TimeRange tr)
{
    auto x = timeToX(tr.getStart().inSeconds());
    auto w = timeToX(tr.getEnd().inSeconds()) - x;
    auto h = getHeight() / 5;

    return {x, getHeight() - h, w, h};
}

tracktion::TimeRange TimeLineComponent::getLoopRangeToBeMovedOrResized()
{
    auto draggedLoopRange = m_cachedLoopRange;

    if (m_leftResized)
    {
        auto newStart = m_isSnapping ? snapTime(draggedLoopRange.getStart() + m_draggedTime, true) : draggedLoopRange.getStart() + m_draggedTime;

        if (newStart > draggedLoopRange.getEnd())
            draggedLoopRange = {draggedLoopRange.getEnd(), newStart};
        else
            draggedLoopRange = {newStart, draggedLoopRange.getEnd()};
    }
    else if (m_rightResized)
    {
        auto newEnd = m_isSnapping ? snapTime(draggedLoopRange.getEnd() + m_draggedTime, true) : draggedLoopRange.getEnd() + m_draggedTime;

        if (newEnd < draggedLoopRange.getStart())
            draggedLoopRange = {newEnd, draggedLoopRange.getStart()};
        else
            draggedLoopRange = {draggedLoopRange.getStart(), newEnd};
    }
    else
    {
        auto newStart = m_isSnapping ? snapTime(draggedLoopRange.getStart() + m_draggedTime, true) : draggedLoopRange.getStart() + m_draggedTime;
        newStart = juce::jmax(tracktion::TimePosition::fromSeconds(0.0), newStart);

        draggedLoopRange = draggedLoopRange.movedToStartAt(newStart);
    }

    return draggedLoopRange;
}

tracktion::TimeDuration TimeLineComponent::xToTimeDuration(int x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimeDuration::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}

tracktion::TimePosition TimeLineComponent::beatToTime(tracktion::BeatPosition beats) { return tracktion::TimePosition::fromSeconds(m_evs.beatToTime(beats.inBeats())); }

int TimeLineComponent::beatsToX(double beats)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_evs.beatsToX(beats, getWidth(), x1beats, x2beats);
}

tracktion::TimePosition TimeLineComponent::xToTimePos(int x)
{
    double x1beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double x2beats = m_evs.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return tracktion::TimePosition::fromSeconds(m_evs.xToTime(x, getWidth(), x1beats, x2beats));
}
tracktion::BeatPosition TimeLineComponent::xToBeatPos(int x)
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
}

void TimeLineComponent::setLastNoteLength(double length)
{
    // nullptr: Tool settings (like last note length) are transient and not part of the undo history
    m_tree.setProperty(IDs::lastNoteLenght, length, nullptr);
}

double TimeLineComponent::getLastNoteLength() { return m_tree.getProperty(IDs::lastNoteLenght, 0.25); }

double TimeLineComponent::getQuantisedNoteBeat(double beat, const te::MidiClip *c, bool down) const
{
    auto editBeat = c->getStartBeat().inBeats() + beat;

    return getQuantisedBeat(editBeat, down) - c->getStartBeat().inBeats();
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
        const auto denominator = static_cast<int>(m_evs.m_pianoRollSnapDenominator);
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
    return !m_usePianoRollSnapSettings
           || static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_pianoRollSnapMode)) != PianoRollSnapMode::off;
}

bool TimeLineComponent::isUsingFixedSnap() const
{
    return m_usePianoRollSnapSettings
           && static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_pianoRollSnapMode)) == PianoRollSnapMode::fixed;
}

double TimeLineComponent::getSnapIntervalBeats() const
{
    if (isUsingFixedSnap())
        return 4.0 / juce::jmax(1, static_cast<int>(m_evs.m_pianoRollSnapDenominator));

    const auto snapType = getBestSnapType();
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

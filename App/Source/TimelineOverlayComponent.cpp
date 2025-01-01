
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

#include "TimelineOverlayComponent.h"

#include <utility>
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"

TimelineOverlayComponent::TimelineOverlayComponent(
        EditViewState &evs
      , tracktion_engine::Track::Ptr track
      , TimeLineComponent& tlc)
    : m_evs (evs)
    , m_track(std::move(track))
    , m_timelineComponent(tlc)
{
    //setInterceptsMouseClicks (false, true);
}

void TimelineOverlayComponent::paint(juce::Graphics &g)
{
    auto colour = m_track->getColour ();
    updateClipRects ();
    for (auto cr : m_clipRects)
    {
        g.setColour(colour);
        GUIHelpers::drawRoundedRectWithSide(g, cr.toFloat(), 10, true, true, false, false);
    }
    if (m_drawDraggedClip)
    {
        g.setColour(colour.withAlpha(.3f));
        GUIHelpers::drawRoundedRectWithSide(g, m_draggedClipRect.withBottom(getHeight()).toFloat(), 10, true, true, false, false);
    }
}

bool TimelineOverlayComponent::hitTest(int x, int y)
{
    updateClipRects ();

    for (auto cr : m_clipRects)
    {
        if (cr.contains (x, y)) return true;
    }
    return false;
}

void TimelineOverlayComponent::mouseMove(const juce::MouseEvent &e)
{
    updateClipRects ();
    for (auto cr : m_clipRects)
    {
        if (cr.contains (e.x, e.y))
        {
            m_move = false;
            if (e.x > cr.getHorizontalRange ().getStart ()
            && e.x < cr.getHorizontalRange ().getStart () + 10)
            {
                setMouseCursor (juce::MouseCursor::LeftEdgeResizeCursor);
                m_leftResized = true;
                m_rightResized = false;
            }
            else if (e.x > cr.getHorizontalRange ().getEnd () - 10
                     && e.x < cr.getHorizontalRange ().getEnd ())
            {
                setMouseCursor (juce::MouseCursor::RightEdgeResizeCursor);
                m_rightResized = true;
                m_leftResized = false;
            }
            else
            {
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                m_leftResized = false;
                m_rightResized = false;
                m_move = true;
            }
        }
    }
}

void TimelineOverlayComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void TimelineOverlayComponent::mouseDown(const juce::MouseEvent &e)
{
    if (auto mc = getMidiClipByPos(e.x))
    {
        m_cachedClip = mc;
        m_cachedPos = mc->getPosition ();
    }
}

void TimelineOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (e.mouseWasDraggedSinceMouseDown ())
    {
        auto clickOffset = e.getMouseDownX () - timeToX (m_cachedPos.getStart ().inSeconds());
        auto br = m_timelineComponent.getCurrentBeatRange();
        auto mouseTime = EngineHelpers::getTimePos(m_evs.xToTime(e.x, getWidth(), br.getStart().inBeats(), br.getEnd().inBeats()));
        mouseTime = e.mods.isShiftDown() ? mouseTime : m_timelineComponent.getBestSnapType().roundTimeDown(mouseTime, m_evs.m_edit.tempoSequence);
        m_drawDraggedClip = false; 
        if (m_cachedClip)
        {
            auto cs = m_cachedPos.getStart();

            if (m_leftResized)
            {
                auto co = m_cachedPos.getOffset();
                auto newStart = juce::jmax(mouseTime, cs - co);
                if (!e.mods.isShiftDown())
                    newStart = tracktion::TimePosition::fromSeconds(getSnapedTime(newStart.inSeconds()));
                m_draggedTimeDelta = cs.inSeconds() - newStart.inSeconds();
                m_draggedClipRect = getClipRect(m_cachedClip);
                m_draggedClipRect.setLeft(timeToX(newStart.inSeconds()));
                repaint();
            }
            else if (m_rightResized)
            {
                auto newEnd = juce::jmax(cs, mouseTime);
                if (!e.mods.isShiftDown())
                    newEnd = tracktion::TimePosition::fromSeconds(getSnapedTime(newEnd.inSeconds()));

                m_draggedTimeDelta = m_cachedPos.getEnd().inSeconds() - mouseTime.inSeconds();
                m_draggedClipRect = getClipRect(m_cachedClip);
                m_draggedClipRect.setRight(timeToX(newEnd.inSeconds()));
                repaint();
            }
            else
            {
                auto newStart = EngineHelpers::getTimePos(m_evs.beatToTime (xToBeats (e.x - clickOffset)));
                auto snaped = m_timelineComponent.getBestSnapType ().roundTimeDown (
                            newStart, m_evs.m_edit.tempoSequence);
                newStart = e.mods.isShiftDown () ? newStart
                                                 : snaped;
                m_draggedTimeDelta = cs.inSeconds() - newStart.inSeconds();
                m_draggedClipRect = getClipRect(m_cachedClip);
                m_draggedClipRect.setPosition(timeToX(newStart.inSeconds()), m_draggedClipRect.getY());
                repaint();
            }

            m_evs.m_selectionManager.selectOnly(m_cachedClip);
            m_drawDraggedClip = true; 
        }
    }
}
void TimelineOverlayComponent::mouseUp(const juce::MouseEvent &e)
{
    if (m_leftResized || m_rightResized)
    {
        EngineHelpers::resizeSelectedClips(m_leftResized, -m_draggedTimeDelta, m_evs);
    }
    else if (m_move)
        moveSelectedClips(e.mods.isCtrlDown(), !e.mods.isShiftDown());
    m_drawDraggedClip = false;
    m_draggedTimeDelta = 0;
    repaint();
}

double TimelineOverlayComponent::getSnapedTime(double time)
{
    return m_evs.getSnapedTime(time, m_timelineComponent.getBestSnapType());
}
std::vector<tracktion_engine::MidiClip *> TimelineOverlayComponent::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        for (auto c : at->getClips ())
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
            {
                midiClips.push_back (mc);
            }
        }
    }
    return midiClips;
}

tracktion_engine::MidiClip *TimelineOverlayComponent::getMidiClipByPos(int x)
{
    for (auto & clip : getMidiClipsOfTrack ())
    {
        if (clip->getStartBeat ().inBeats() < xToBeats (x)
                &&  clip->getEndBeat ().inBeats() > xToBeats (x))
        {
            return clip;
        }
    }
    return nullptr;
}
void TimelineOverlayComponent::moveSelectedClips(bool copy, bool snap)
{
    EngineHelpers::moveSelectedClips(copy, -m_draggedTimeDelta, 0, m_evs);
}
int TimelineOverlayComponent::timeToX(double time)
{
    auto br = m_timelineComponent.getCurrentBeatRange();
    return m_evs.timeToX(time, getWidth(), br.getStart().inBeats(), br.getEnd().inBeats());
}

double TimelineOverlayComponent::xToBeats(int x)
{
    auto br = m_timelineComponent.getCurrentBeatRange();
    return m_evs.xToBeats(x, getWidth(), br.getStart().inBeats(), br.getEnd().inBeats());
}

void TimelineOverlayComponent::updateClipRects()
{
    m_clipRects.clear ();
    if (auto audiotrack = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        for (auto c : audiotrack->getClips ())
        {
            m_clipRects.add(getClipRect(c));
        }
    }
}
juce::Rectangle<int> TimelineOverlayComponent::getClipRect(te::Clip::Ptr c)    
{
    auto startX = timeToX (c->getPosition ().getStart ().inSeconds() );
    auto endX = timeToX (c->getPosition ().getEnd ().inSeconds()) + 1;
    auto tlr = m_timelineComponent.getBounds();
    juce::Rectangle<int> clipRect = {startX,tlr.getHeight () - (tlr.getHeight ()/3)
                                     , endX-startX, (tlr.getHeight() / 3)};
    return clipRect;
}

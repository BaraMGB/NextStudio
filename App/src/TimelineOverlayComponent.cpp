
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

#include "TimelineOverlayComponent.h"

#include "Utilities.h"
#include "ClipGestureLimits.h"
#include "tracktion_core/utilities/tracktion_Time.h"
#include <utility>

TimelineOverlayComponent::TimelineOverlayComponent(EditViewState &evs, tracktion_engine::Track::Ptr track, TimeLineComponent &tlc)
    : m_evs(evs),
      m_track(std::move(track)),
      m_timelineComponent(tlc)
{
    setWantsKeyboardFocus(true);
}

TimelineOverlayComponent::~TimelineOverlayComponent() { cancelInteraction(); }

void TimelineOverlayComponent::cancelInteraction()
{
    if (m_drawDraggedClip && m_evs.clipInteractionPreviewChanged)
        m_evs.clipInteractionPreviewChanged({});
    m_drawDraggedClip = false;
    m_cachedClip = nullptr;
    m_draggedTimeDelta = 0;
    m_mouseGesture.reset();
    m_mouseInput.reset();
    m_timelineComponent.clearMouseFeedback(TimelineFeedbackOwner::overlay);
    repaint();
}

bool TimelineOverlayComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey && m_mouseGesture.active())
    {
        cancelInteraction();
        return true;
    }
    return false;
}

void TimelineOverlayComponent::paint(juce::Graphics &g)
{
    auto colour = m_track->getColour();
    updateClipRects();
    const auto strokeColour = m_evs.m_applicationState.getTimeLineStrokeColour();
    const auto selectedStrokeColour = m_evs.m_applicationState.getPrimeColour();
    for (auto i = 0; i < m_clipRects.size(); ++i)
    {
        auto cr = m_clipRects.getUnchecked(i);
        auto *clip = m_clipsForRects.getUnchecked(i);
        g.setColour(colour);
        GUIHelpers::drawRoundedRectWithSide(g, cr, 10, true, true, false, false);
        g.setColour(m_evs.m_selectionManager.isSelected(clip) ? selectedStrokeColour : strokeColour);
        GUIHelpers::strokeRoundedRectWithSide(g, cr, 10, true, true, false, false);
    }
    if (m_drawDraggedClip)
    {
        g.setColour(colour.withAlpha(.3f));
        GUIHelpers::drawRoundedRectWithSide(g, m_draggedClipRect.withBottom(float(getHeight())), 10, true, true, false, false);
    }
    // The ruler signal is owned by PianoRollEditor::paintOverChildren().
    // Keep this body pass below it so the translucent line is not doubled.
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(getLocalBounds().withTrimmedTop(juce::jmin(getHeight(), m_timelineComponent.getHeight())));
    m_timelineComponent.drawMouseFeedback(g, TimelineFeedbackOwner::overlay);
}

bool TimelineOverlayComponent::hitTest(int x, int y)
{
    updateClipRects();

    for (auto cr : m_clipRects)
    {
        if (cr.contains(x, y))
            return true;
    }
    return false;
}

void TimelineOverlayComponent::mouseMove(const juce::MouseEvent &e)
{
    updateClipRects();
    for (auto cr : m_clipRects)
    {
        if (cr.contains(e.x, e.y))
        {
            m_move = false;
            if (e.x > cr.getHorizontalRange().getStart() && e.x < cr.getHorizontalRange().getStart() + 10)
            {
                setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
                m_leftResized = true;
                m_rightResized = false;
            }
            else if (e.x > cr.getHorizontalRange().getEnd() - 10 && e.x < cr.getHorizontalRange().getEnd())
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
                m_move = true;
            }
        }
    }
}

void TimelineOverlayComponent::mouseExit(const juce::MouseEvent & /*e*/) { setMouseCursor(juce::MouseCursor::NormalCursor); }

void TimelineOverlayComponent::mouseDown(const juce::MouseEvent &e)
{
    cancelInteraction();
    if (m_evs.clipInteractionBeginning)
        m_evs.clipInteractionBeginning();
    grabKeyboardFocus();
    m_mouseInput.remember(e);
    mouseMove(e); // resolve the grabbed edge from this event, not stale hover flags
    if (auto mc = getMidiClipAtPoint(e.getPosition()))
    {
        m_cachedClip = mc;
        m_cachedPos = mc->getPosition();
        if (e.mods.isShiftDown())
            m_evs.m_selectionManager.select(mc, true);
        else
            m_evs.m_selectionManager.selectOnly(mc);
        const auto resolver = m_timelineComponent.getMouseSnapResolver();
        m_originalEdgeBeat = resolver.timeToBeat((m_rightResized ? m_cachedPos.getEnd() : m_cachedPos.getStart()).inSeconds());
        m_mouseGesture.begin(m_originalEdgeBeat, e.position.x, resolver);
    }
}

void TimelineOverlayComponent::refreshMouseSnapContext()
{
    if (!isShowing())
    {
        cancelInteraction();
        return;
    }
    if (auto event = m_mouseInput.forContext(*this, juce::ModifierKeys::getCurrentModifiers()))
        mouseDrag(*event);
}

void TimelineOverlayComponent::modifierKeysChanged(const juce::ModifierKeys& mods)
{
    if (auto event = m_mouseInput.withModifiers(mods); event && m_mouseGesture.active())
        mouseDrag(*event);
}

void TimelineOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    m_mouseInput.remember(e);
    if (!e.mouseWasDraggedSinceMouseDown() || m_cachedClip == nullptr)
        return;
    if (!m_cachedClip->state.isAChildOf(m_evs.m_edit.state))
    {
        cancelInteraction();
        return;
    }
    const auto resolver = m_timelineComponent.getMouseSnapResolver();
    const auto candidate = m_mouseGesture.update(e.position.x, resolver, e.mods.isShiftDown());
    const auto kind = m_leftResized ? ClipGestureLimits::Kind::resizeLeft
                    : m_rightResized ? ClipGestureLimits::Kind::resizeRight : ClipGestureLimits::Kind::move;
    const auto sourceEdgeTime = ClipGestureLimits::edgeTime(m_cachedPos, kind);
    const auto delta = ClipGestureLimits::constrain(m_evs.m_selectionManager.getItemsOfType<te::Clip>(), kind,
        resolver.beatToTime(candidate) - sourceEdgeTime);
    m_mouseGesture.setDisplayedBeat(resolver.timeToBeat(sourceEdgeTime + delta));
    m_draggedTimeDelta = delta;
    const auto preview = ClipGestureLimits::previewRange(m_cachedPos, kind, delta);
    const auto start = preview.getStart();
    const auto end = preview.getEnd();
    m_draggedClipRect = getClipRect(m_cachedClip);
    m_draggedClipRect.setLeft(timeToX(start.inSeconds()));
    m_draggedClipRect.setRight(timeToX(end.inSeconds()));
    m_drawDraggedClip = true;
    if (m_evs.clipInteractionPreviewChanged)
        m_evs.clipInteractionPreviewChanged(ClipTimingPreview{preview,
            m_evs.m_selectionManager.getItemsOfType<te::Clip>().size()});
    m_timelineComponent.setMouseFeedback(TimelineInteractionFeedback{m_mouseGesture.feedback(), TimelineFeedbackOwner::overlay,
        {0.0f, float(getHeight())}, float(getHeight()) * 0.5f},
        [safe = juce::Component::SafePointer<TimelineOverlayComponent>(this)]
        { if (safe) safe->refreshMouseSnapContext(); });
    repaint();
}
void TimelineOverlayComponent::mouseUp(const juce::MouseEvent &e)
{
    if (e.mouseWasDraggedSinceMouseDown())
        mouseDrag(e);
    if (m_drawDraggedClip && m_cachedClip != nullptr)
    {
        if (m_leftResized || m_rightResized)
            EngineHelpers::resizeSelectedClips(m_leftResized, m_draggedTimeDelta, m_evs);
        else if (m_move)
            moveSelectedClips(e.mods.isCtrlDown());
    }
    cancelInteraction();
}

std::vector<tracktion_engine::MidiClip *> TimelineOverlayComponent::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip *> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack *>(&(*m_track)))
    {
        for (auto c : at->getClips())
        {
            if (auto mc = dynamic_cast<te::MidiClip *>(c))
            {
                midiClips.push_back(mc);
            }
        }
    }
    return midiClips;
}

tracktion_engine::MidiClip *TimelineOverlayComponent::getMidiClipAtPoint(juce::Point<int> point)
{
    updateClipRects();
    for (auto i = 0; i < m_clipRects.size(); ++i)
        if (m_clipRects.getUnchecked(i).contains(point.toFloat()))
            return m_clipsForRects.getUnchecked(i);
    return {};
}
void TimelineOverlayComponent::moveSelectedClips(bool copy) { EngineHelpers::moveSelectedClips(copy, m_draggedTimeDelta, 0, m_evs); }
float TimelineOverlayComponent::timeToX(double time)
{
    auto br = m_timelineComponent.getCurrentBeatRange();
    return m_evs.timeToX(time, getWidth(), br.getStart().inBeats(), br.getEnd().inBeats());
}

void TimelineOverlayComponent::updateClipRects()
{
    m_clipRects.clear();
    m_clipsForRects.clear();
    for (auto *clip : getMidiClipsOfTrack())
    {
        m_clipRects.add(getClipRect(clip));
        m_clipsForRects.add(clip);
    }
}
juce::Rectangle<float> TimelineOverlayComponent::getClipRect(te::Clip::Ptr c)
{
    auto startX = timeToX(c->getPosition().getStart().inSeconds());
    auto endX = timeToX(c->getPosition().getEnd().inSeconds()) + 1;
    auto tlr = m_timelineComponent.getBounds();
    juce::Rectangle<float> clipRect = {startX, float(tlr.getHeight() - (tlr.getHeight() / 3)), endX - startX, float(tlr.getHeight() / 3)};
    return clipRect;
}

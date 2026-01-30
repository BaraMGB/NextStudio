
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

#include "SongEditorView.h"
#include "Browser_Base.h"
#include "TimeUtils.h"
#include "Utilities.h"

SongEditorView::SongEditorView(EditViewState &evs, MenuBar &toolBar, TimeLineComponent &timeLine)
    : m_editViewState(evs),
      m_toolBar(toolBar),
      m_timeLine(timeLine),
      m_lassoComponent(evs, m_timeLine.getTimeLineID()),
      m_timeRangeOverlay(*this)
{
    setWantsKeyboardFocus(true);
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop(true);
    m_lassoComponent.toFront(true);

    addAndMakeVisible(m_timeRangeOverlay);
    m_timeRangeOverlay.setAlwaysOnTop(true);

    clearSelectedTimeRange();

    m_editViewState.m_edit.getTransport().addChangeListener(this);
    m_editViewState.m_selectionManager.addChangeListener(this);
    m_editViewState.m_trackHeightManager->addChangeListener(this);
}

SongEditorView::~SongEditorView()
{
    m_editViewState.m_trackHeightManager->removeChangeListener(this);
    m_editViewState.m_edit.getTransport().removeChangeListener(this);
    m_editViewState.m_selectionManager.removeChangeListener(this);
}

void SongEditorView::paintOverChildren(juce::Graphics &g)
{
    using namespace juce::Colours;
    auto &sm = m_editViewState.m_selectionManager;
    auto scroll = timeToX(tracktion::TimePosition::fromSeconds(0)) * (-1.0f);
    const auto area = getLocalBounds().toFloat();

    if (m_draggedClip)
    {
        for (auto selectedClip : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(selectedClip->getTrack(), m_draggedVerticalOffset))
            {
                auto clipRect = getClipRect(selectedClip);
                float targetX = clipRect.getX() + timeToX(tracktion::TimePosition() + m_draggedTimeDelta) + scroll;
                float targetY = static_cast<float>(getYForTrack(targetTrack));
                float targetW = clipRect.getWidth();
                float targetH = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(targetTrack, false));

                juce::Rectangle<float> targetRect(targetX, targetY, targetW, targetH);

                if (m_dragState.isLeftEdge)
                {
                    auto offset = selectedClip->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset, m_draggedTimeDelta.inSeconds());
                    auto deltaX = timeToX(tracktion::TimePosition() + tracktion::TimeDuration::fromSeconds(timeDelta)) + scroll;

                    targetRect = juce::Rectangle<float>(clipRect.getX() + deltaX, targetY, clipRect.getWidth() - deltaX, targetH);
                }
                else if (m_dragState.isRightEdge)
                {
                    targetRect = juce::Rectangle<float>(clipRect.getX(), targetY, clipRect.getWidth() + timeToX(tracktion::TimePosition() + m_draggedTimeDelta) + scroll, targetH);
                }

                g.setColour(white);
                g.drawRect(targetRect, 1.0f);

                if (EngineHelpers::trackWantsClip(selectedClip, targetTrack))
                {
                    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
                    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
                    juce::Rectangle<float> trackRect(0.0f, targetY, static_cast<float>(getWidth()), targetH);
                    GUIHelpers::drawClip(g, *this, m_editViewState, targetRect, selectedClip, targetTrack->getColour().withAlpha(0.1f), trackRect, x1, x2);
                }
                else
                {
                    g.setColour(grey);
                    g.fillRect(targetRect.reduced(1.0f, 1.0f));
                    g.setColour(black);
                    g.drawFittedText("not allowed", targetRect.toNearestInt(), juce::Justification::centred, 1);
                }
            }
        }
    }

    if (m_dragItemRect.visible)
    {
        if (m_dragItemRect.valid)
        {
            auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
            auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
            juce::Rectangle<float> dragRectFloat = m_dragItemRect.drawRect.toFloat();

            GUIHelpers::drawClipBody(g, m_editViewState, m_dragItemRect.name, dragRectFloat, false, m_dragItemRect.colour, area, x1, x2);
        }
        else
        {
            g.setColour(grey);
            g.fillRect(m_dragItemRect.drawRect.toFloat().reduced(1.0f, 1.0f));
            g.setColour(black);
            g.drawFittedText("not allowed", m_dragItemRect.drawRect.toNearestIntEdges(), juce::Justification::centred, 1);
        }
    }
    m_lassoComponent.drawLasso(g);
}

void SongEditorView::resized()
{
    auto &trackHeightManager = m_editViewState.m_trackHeightManager;
    const int yScroll = juce::roundToInt(m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    int y = yScroll;

    for (auto lane : m_trackLanes)
    {
        auto trackHeaderHeight = trackHeightManager->getTrackHeight(lane->getTrack(), true);
        auto leftEdge = 0;
        auto w = getWidth();

        lane->setBounds(leftEdge, y, w, trackHeaderHeight);
        y += trackHeaderHeight;
    }

    m_lassoComponent.setBounds(getLocalBounds());
    m_timeRangeOverlay.setBounds(getLocalBounds());
}

// Mouse events are now handled by TrackLaneComponent and AutomationLaneComponent.
// These components call startLasso/updateLasso/stopLasso directly for lasso functionality.

void SongEditorView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_editViewState.m_edit.getTransport())
    {
        for (auto t : te::getAudioTracks(m_editViewState.m_edit))
            buildRecordingClips(t);
    }
    else if (source == &m_editViewState.m_selectionManager)
    {
    }
    else if (source == m_editViewState.m_trackHeightManager.get())
    {
        resized();
    }
}

bool SongEditorView::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent *>(dragSourceDetails.sourceComponent.get()))
        return true;

    GUIHelpers::log(dragSourceDetails.description.toString());

    if (dragSourceDetails.description == "FileBrowser")
    {
        auto f = juce::File();
        if (auto b = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
            f = b->getSelectedFile();

        if (f.existsAsFile())
        {
            if (f.getFileName().endsWith("tracktion_edit"))
                return true;

            auto af = te::AudioFile(m_editViewState.m_edit.engine, f);
            if (af.isValid())
                return true;
        }
    }

    if (dragSourceDetails.description == "SampleBrowser")
        return true;

    return false;
}

void SongEditorView::itemDragEnter(const SourceDetails &dragSourceDetails) {}

void SongEditorView::itemDragMove(const SourceDetails &dragSourceDetails)
{
    auto pos = dragSourceDetails.localPosition;
    bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnappedTime(xtoTime(pos.x));

    auto f = juce::File();
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent *>(dragSourceDetails.sourceComponent.get()))
        f = fileTreeComp->getSelectedFile();
    else if (auto fileBrowser = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
        f = fileBrowser->getSelectedFile();

    m_dragItemRect.valid = false;

    if (!f.exists())
    {
        repaint();
        return;
    }

    auto targetTrack = getTrackAt(pos.y);

    // Calculate drag rect dimensions
    te::AudioFile audioFile(m_editViewState.m_edit.engine, f);
    auto x = timeToX(dropTime);
    auto w = timeDurationToPixel(tracktion::TimeDuration::fromSeconds(audioFile.getLength()));
    float y = 0.0f;
    float h = 0.0f;

    if (targetTrack)
    {
        y = (float)getYForTrack(targetTrack);
        h = (float)m_editViewState.m_trackHeightManager->getTrackHeight(targetTrack, false);

        m_dragItemRect.drawRect = {x, y, w, h};
        m_dragItemRect.name = f.getFileNameWithoutExtension();
        m_dragItemRect.visible = true;

        if (auto at = dynamic_cast<te::AudioTrack *>(targetTrack.get()))
        {
            if (!at->state.getProperty(IDs::isMidiTrack))
            {
                m_dragItemRect.valid = true;
                m_dragItemRect.colour = targetTrack->getColour();
            }
        }
    }
    else
    {
        auto lastTrackID = m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit).getLast();
        auto lastTrack = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, lastTrackID);

        y = lastTrack ? getYForTrack(lastTrack) + m_editViewState.m_trackHeightManager->getTrackHeight(lastTrack, true) : 0;
        h = static_cast<int>(m_editViewState.m_trackDefaultHeight);

        m_dragItemRect.drawRect = {x, y, w, h};
        m_dragItemRect.name = f.getFileNameWithoutExtension();
        m_dragItemRect.valid = true;
        m_dragItemRect.visible = true;
        m_dragItemRect.colour = m_editViewState.m_applicationState.getPrimeColour();
    }

    repaint();
}

void SongEditorView::itemDragExit(const SourceDetails &dragSourceDetails)
{
    m_dragItemRect.visible = false;
    repaint();
}

void SongEditorView::itemDropped(const SourceDetails &dragSourceDetails)
{
    auto pos = dragSourceDetails.localPosition;
    bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnappedTime(xtoTime(pos.x));
    auto f = juce::File();

    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent *>(dragSourceDetails.sourceComponent.get()))
        auto f = fileTreeComp->getSelectedFile();
    else if (auto fileBrowser = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
        f = fileBrowser->getSelectedFile();

    if (f.exists())
    {
        if (auto targetTrack = getTrackAt(pos.y))
        {
            if (auto at = dynamic_cast<te::AudioTrack *>(targetTrack.get()))
            {
                if (!at->state.getProperty(IDs::isMidiTrack))
                {
                    te::AudioFile audioFile(m_editViewState.m_edit.engine, f);
                    addWaveFileToTrack(audioFile, dropTime.inSeconds(), at);
                }
            }
        }
        else
        {
            EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, m_editViewState.m_applicationState.getRandomTrackColour(), dropTime.inSeconds());
        }
    }
    m_dragItemRect.visible = false;
    repaint();
}

void SongEditorView::addWaveFileToTrack(te::AudioFile audioFile, double dropTime, te::AudioTrack::Ptr track) const
{
    if (audioFile.isValid())
    {
        auto length = tracktion::TimeDuration::fromSeconds(audioFile.getLength());
        auto dropPos = tracktion::TimePosition::fromSeconds(dropTime);
        te::ClipPosition clipPos;
        clipPos.time = {dropPos, length};

        EngineHelpers::loadAudioFileToTrack(audioFile.getFile(), track, clipPos);
    }
}

te::Track::Ptr SongEditorView::getTrackAt(int y)
{
    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        auto s = getYForTrack(t);
        auto e = s + m_editViewState.m_trackHeightManager->getTrackHeight(t, true);
        auto vRange = juce::Range<int>(s, e);
        if (vRange.contains(y))
            return t;
    }

    return nullptr;
}

int SongEditorView::getYForTrack(te::Track *track)
{
    int scrollY = -(m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    auto h = m_editViewState.m_trackHeightManager->getYForTrack(track, scrollY);
    return h;
}

void SongEditorView::updateDragGhost(te::Clip::Ptr clip, tracktion::TimeDuration delta, int verticalOffset)
{
    m_draggedClip = clip;
    m_draggedTimeDelta = delta;
    m_draggedVerticalOffset = verticalOffset;
    repaint();
}

juce::Rectangle<float> SongEditorView::getAutomationRect(te::AutomatableParameter::Ptr ap)
{
    int scrollY = -(m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    float x = static_cast<float>(getLocalBounds().getX());
    float y = static_cast<float>(m_editViewState.m_trackHeightManager->getYForAutomatableParameter(ap->getTrack(), ap, scrollY));
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(m_editViewState.m_trackHeightManager->getAutomationHeight(ap));
    return {x, y, w, h};
}

bool SongEditorView::hitTestTimeRange(int x, te::Track *track, bool &outLeftEdge, bool &outRightEdge) const
{
    outLeftEdge = false;
    outRightEdge = false;

    if (track == nullptr)
        return false;

    if (!m_selectedRange.selectedTracks.contains(track))
        return false;

    if (m_selectedRange.timeRange.getLength().inSeconds() <= 0)
        return false;

    // Check if x position is within the time range
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    auto timeAtX = tracktion::TimePosition::fromSeconds(m_editViewState.xToTime(x, getWidth(), x1, x2));

    if (!m_selectedRange.timeRange.contains(timeAtX))
        return false;

    // Check for edge proximity
    auto leftX = static_cast<int>(m_editViewState.timeToX(m_selectedRange.timeRange.getStart().inSeconds(), getWidth(), x1, x2));
    auto rightX = static_cast<int>(m_editViewState.timeToX(m_selectedRange.timeRange.getEnd().inSeconds(), getWidth(), x1, x2));

    const int edgeThreshold = 5;
    if (x < leftX + edgeThreshold)
        outLeftEdge = true;
    else if (x > rightX - edgeThreshold)
        outRightEdge = true;

    return true;
}

bool SongEditorView::hitTestTimeRange(int x, te::AutomatableParameter *param, bool &outLeftEdge, bool &outRightEdge) const
{
    outLeftEdge = false;
    outRightEdge = false;

    if (param == nullptr)
        return false;

    if (!m_selectedRange.selectedAutomations.contains(param))
        return false;

    if (m_selectedRange.timeRange.getLength().inSeconds() <= 0)
        return false;

    // Check if x position is within the time range
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    auto timeAtX = tracktion::TimePosition::fromSeconds(m_editViewState.xToTime(x, getWidth(), x1, x2));

    if (!m_selectedRange.timeRange.contains(timeAtX))
        return false;

    // Check for edge proximity
    auto leftX = static_cast<int>(m_editViewState.timeToX(m_selectedRange.timeRange.getStart().inSeconds(), getWidth(), x1, x2));
    auto rightX = static_cast<int>(m_editViewState.timeToX(m_selectedRange.timeRange.getEnd().inSeconds(), getWidth(), x1, x2));

    const int edgeThreshold = 5;
    if (x < leftX + edgeThreshold)
        outLeftEdge = true;
    else if (x > rightX - edgeThreshold)
        outRightEdge = true;

    return true;
}

void SongEditorView::startTimeRangeDrag()
{
    m_isDraggingSelectedTimeRange = false;
    m_draggedTimeDelta = tracktion::TimeDuration();
}

void SongEditorView::updateTimeRangeDragMove(tracktion::TimeDuration delta)
{
    // Moving the entire range
    m_isDraggingSelectedTimeRange = true;
    m_draggedTimeDelta = delta;
    repaint();
}

void SongEditorView::updateTimeRangeDragResizeLeft(tracktion::TimePosition newEdgeTime)
{
    newEdgeTime = juce::jmax(tracktion::TimePosition::fromSeconds(0.0), newEdgeTime);
    // Ensure left edge doesn't pass right edge
    if (newEdgeTime >= m_selectedRange.getEnd())
        newEdgeTime = m_selectedRange.getEnd() - tracktion::TimeDuration::fromSeconds(0.01);
    setSelectedTimeRange({newEdgeTime, m_selectedRange.getEnd()}, true, false);
    repaint();
}

void SongEditorView::updateTimeRangeDragResizeRight(tracktion::TimePosition newEdgeTime)
{
    // Ensure right edge doesn't pass left edge
    newEdgeTime = juce::jmax(m_selectedRange.getStart() + tracktion::TimeDuration::fromSeconds(0.01), newEdgeTime);
    setSelectedTimeRange({m_selectedRange.getStart(), newEdgeTime}, false, false);
    repaint();
}

void SongEditorView::finishTimeRangeDrag(bool copy)
{
    if (m_isDraggingSelectedTimeRange)
    {
        moveSelectedTimeRanges(m_draggedTimeDelta, copy);
        auto newStart = m_selectedRange.getStart() + m_draggedTimeDelta;
        m_selectedRange.timeRange = m_selectedRange.timeRange.movedToStartAt(newStart);
    }

    m_isDraggingSelectedTimeRange = false;
    m_draggedTimeDelta = tracktion::TimeDuration();
    repaint();
}

void SongEditorView::cancelTimeRangeDrag()
{
    m_isDraggingSelectedTimeRange = false;
    m_draggedTimeDelta = tracktion::TimeDuration();
    repaint();
}

void SongEditorView::startLasso(const juce::MouseEvent &e, bool fromAutomation, bool selectRange)
{
    m_lassoComponent.startLasso({e.x, e.y}, m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()), selectRange);
    m_isSelectingTimeRange = selectRange;
    m_isLassoStartedInAutomation = fromAutomation;
    if (selectRange)
    {
        clearSelectedTimeRange();
        m_cachedSelectedClips.clear();
    }
    else
    {
        if (fromAutomation)
        {
            m_cachedSelectedClips.clear();
        }
        else
        {
            updateClipCache();
        }
    }
}
void SongEditorView::updateLasso(const juce::MouseEvent &e)
{
    if (m_lassoComponent.isVisible() || m_isSelectingTimeRange)
    {
        m_lassoComponent.updateLasso({e.x, e.y}, m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
        if (m_isSelectingTimeRange)
            updateRangeSelection();
        else if (m_isLassoStartedInAutomation)
            updateAutomationSelection(e.mods.isShiftDown());
        else
            updateClipSelection(e.mods.isShiftDown());
        repaint();
    }
}

void SongEditorView::stopLasso()
{
    // Finalize lasso selection with snapping
    if (m_lassoComponent.isVisible() || m_isSelectingTimeRange)
    {
        auto start = m_lassoComponent.getLassoRect().m_timeRange.getStart();
        auto end = m_lassoComponent.getLassoRect().m_timeRange.getEnd();
        m_selectedRange.timeRange = {getSnappedTime(start, true), getSnappedTime(end, false)};
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
    m_lassoComponent.stopLasso();
    m_isLassoStartedInAutomation = false;

    // Switch back to pointer mode after TimeRange selection
    if (m_isSelectingTimeRange || m_toolMode == Tool::range || m_toolMode == Tool::lasso)
    {
        for (auto b : m_toolBar.getButtons())
            if (b->getName() == "select")
                b->setToggleState(true, juce::sendNotification);
    }

    m_isSelectingTimeRange = false;
}

void SongEditorView::duplicateSelectedClipsOrTimeRange()
{
    // This function handles duplication of either selected clips or a time range.
    // The editor allows selection of either individual clips OR a time range, but not both simultaneously.
    auto isTimeRangeSelected = m_selectedRange.selectedTracks.size() != 0;

    if (isTimeRangeSelected)
    {
        moveSelectedTimeRanges(m_selectedRange.getLength(), true);
        setSelectedTimeRange({m_selectedRange.getStart() + m_selectedRange.getLength(), m_selectedRange.getLength()}, false, false);
        repaint(); // Repaint all tracks and lanes after duplication
    }
    else
    {
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto range = te::getTimeRangeForSelectedItems(selectedClips);
        auto delta = range.getLength().inSeconds();

        GUIHelpers::log("moving Clips.");
        moveSelectedClips(true, delta, 0);
        GUIHelpers::log("Clips moved.");
    }
}

void SongEditorView::moveSelectedClips(bool copy, double delta, int verticalOffset)
{
    EngineHelpers::moveSelectedClips(copy, delta, verticalOffset, m_editViewState);
    repaint();
}

int SongEditorView::getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int> &dropPos)
{
    auto targetTrack = getTrackAt(dropPos.getY());

    if (targetTrack)
    {
        auto showedTracks = m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit);
        if (showedTracks.contains(sourceTrack->itemID) && showedTracks.contains(targetTrack->itemID))
            return showedTracks.indexOf(targetTrack->itemID) - showedTracks.indexOf(sourceTrack->itemID);
    }

    return 0;
}

void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge) { EngineHelpers::resizeSelectedClips(fromLeftEdge, m_draggedTimeDelta.inSeconds(), m_editViewState); }

tracktion_engine::MidiClip::Ptr SongEditorView::createNewMidiClip(double beatPos, te::Track::Ptr track)
{

    if (auto at = dynamic_cast<te::AudioTrack *>(track.get()))
    {

        auto start = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beatPos)));
        auto end = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beatPos)) + m_editViewState.beatToTime(4));
        tracktion::core::TimeRange newPos(start, end);
        at->deleteRegion(newPos, &m_editViewState.m_selectionManager);

        auto mc = at->insertMIDIClip(newPos, &m_editViewState.m_selectionManager);
        mc->setName(at->getName());
        auto trackTimeLineID = "ID" + track->itemID.toString().removeCharacters("{}-");
        setPianoRoll(track.get());
        GUIHelpers::centerMidiEditorToClip(m_editViewState, mc, trackTimeLineID, getWidth());

        return mc;
    }
    return nullptr;
}

void SongEditorView::setPianoRoll(te::Track *track)
{
    m_editViewState.setLowerRangeView(LowerRangeView::midiEditor);

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        t->state.setProperty(IDs::showLowerRange, false, nullptr);

        if (t == track)
            t->state.setProperty(IDs::showLowerRange, true, nullptr);
    }
}

void SongEditorView::updateClipCache()
{
    clearSelectedTimeRange();
    m_cachedSelectedClips.clear();

    for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        m_cachedSelectedClips.add(c);
}

void SongEditorView::updateRangeSelection()
{
    auto &sm = m_editViewState.m_selectionManager;
    sm.deselectAll();
    clearSelectedTimeRange();

    auto range = m_lassoComponent.getLassoRect().m_timeRange;
    juce::Range<int> lassoRangeY = m_lassoComponent.getLassoRect().m_verticalRange;

    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        auto trackVRange = getVerticalRangeOfTrack(t, false);
        if (trackVRange.intersects(lassoRangeY))
            m_selectedRange.selectedTracks.add(t);
    }

    for (auto &ap : m_editViewState.m_edit.getAllAutomatableParams(true))
    {
        if (m_editViewState.m_trackHeightManager->isAutomationVisible(*ap))
        {
            auto rect = getAutomationRect(ap);
            juce::Range<int> vRange = juce::Range<int>(rect.getY(), rect.getBottom());
            if (vRange.intersects(lassoRangeY))
                m_selectedRange.selectedAutomations.addIfNotAlreadyThere(ap);
        }
    }

    setSelectedTimeRange(range, true, false);
}

void SongEditorView::clearSelectedTimeRange()
{
    m_selectedRange.timeRange = tracktion::TimeRange();
    m_selectedRange.selectedTracks.clear();
    m_selectedRange.selectedAutomations.clear();
}

void SongEditorView::deleteSelectedTimeRange()
{
    for (auto t : m_selectedRange.selectedTracks)
        if (auto ct = dynamic_cast<te::ClipTrack *>(t))
            ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

    for (auto a : m_selectedRange.selectedAutomations)
        a->getCurve().removePointsInRegion(m_selectedRange.timeRange);
}
void SongEditorView::setSelectedTimeRange(tracktion::TimeRange tr, bool snapDownAtStart, bool snapDownAtEnd)
{
    auto start = tr.getStart();
    auto end = tr.getEnd();
    m_selectedRange.timeRange = {getSnappedTime(start, snapDownAtStart), getSnappedTime(end, snapDownAtEnd)};
}

juce::Array<te::Track *> SongEditorView::getTracksWithSelectedTimeRange() { return m_selectedRange.selectedTracks; }

tracktion::TimeRange SongEditorView::getSelectedTimeRange() { return m_selectedRange.timeRange; }

// splitClipAt removed as it is now handled in TrackLaneComponent

void SongEditorView::reverseSelectedClips()
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip *>(c))
        {
            auto reversed = wac->getIsReversed();
            wac->setIsReversed(!reversed);
            m_editViewState.removeThumbnail(wac->itemID);
        }
    }
}
void SongEditorView::transposeSelectedClips(float pitchChange)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip *>(c))
        {
            auto pitch = wac->getPitchChange();
            wac->setPitchChange(pitch + pitchChange);
            m_editViewState.removeThumbnail(wac->itemID);
        }
    }
}

void SongEditorView::setNewTempoOfClipByNewLength(te::WaveAudioClip::Ptr wac, double newLength)
{
    auto &engine = m_editViewState.m_edit.engine;

    wac->setTimeStretchMode(te::TimeStretcher::soundtouchBetter);

    const auto audioFileInfo = wac->getAudioFile().getInfo();

    const auto loopInfo = audioFileInfo.loopInfo;
    const auto tempo = loopInfo.getBpm(audioFileInfo);

    GUIHelpers::log("file's BPM: ", tempo);

    if (newLength != 0)
    {
        const double newSpeedRatio = audioFileInfo.getLengthInSeconds() / newLength;

        wac->setSpeedRatio(newSpeedRatio);
        wac->setLength(tracktion::TimeDuration::fromSeconds(audioFileInfo.getLengthInSeconds()) / wac->getSpeedRatio(), true);
    }
}

void SongEditorView::updateAutomationSelection(bool add)
{
    if (!add)
        m_editViewState.m_selectionManager.deselectAll();

    auto lassoRect = m_lassoComponent.getLassoRect().m_rect;

    for (auto *tl : m_trackLanes)
    {
        if (auto t = tl->getTrack())
        {
            for (auto *ap : t->getAllAutomatableParams())
            {
                if (auto *al = tl->getAutomationLane(ap))
                {
                    al->selectPointsInLasso(lassoRect, add);
                }
            }
        }
    }
}

void SongEditorView::updateClipSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll();

    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        juce::Range<int> lassoRangeY = {(int)m_lassoComponent.getLassoRect().m_verticalRange.getStart(), (int)m_lassoComponent.getLassoRect().m_verticalRange.getEnd()};
        if (getVerticalRangeOfTrack(t, false).intersects(lassoRangeY) && !(t->isFolderTrack()))
            selectClipsInLasso(t);
    }

    if (add)
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
}

juce::Range<int> SongEditorView::getVerticalRangeOfTrack(tracktion_engine::Track::Ptr track, bool withAutomation)
{
    auto trackY = getYForTrack(track);
    auto trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(track, withAutomation);

    return {trackY, trackY + trackHeight};
}

void SongEditorView::selectClipsInLasso(const tracktion_engine::Track *track)
{
    for (auto ti = 0; ti < track->getNumTrackItems(); ti++)
    {
        auto item = track->getTrackItem(ti);
        if (m_lassoComponent.getLassoRect().m_startTime < item->getPosition().getEnd().inSeconds() && m_lassoComponent.getLassoRect().m_endTime > item->getPosition().getStart().inSeconds())
        {
            m_editViewState.m_selectionManager.addToSelection(item);
        }
    }
}

void SongEditorView::moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy)
{
    for (auto t : m_selectedRange.selectedTracks)
        if (t != nullptr)
            moveSelectedRangeOfTrack(t, td, copy);

    for (auto ap : m_selectedRange.selectedAutomations)
    {
        auto as = EngineHelpers::getTrackAutomationSection(ap, m_selectedRange.timeRange);
        EngineHelpers::moveAutomationOrCopy(as, td, copy);
    }
}

void SongEditorView::moveSelectedRangeOfTrack(te::Track::Ptr track, tracktion::TimeDuration duration, bool copy)
{
    if (auto ct = dynamic_cast<te::ClipTrack *>(track.get()))
    {
        const auto editStart = tracktion::TimePosition::fromSeconds(0.0);
        const auto viewStartTime = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength();
        const auto targetStart = m_selectedRange.getStart() + duration;
        const auto targetEnd = m_selectedRange.getEnd() + duration;

        te::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();

        for (auto &c : ct->getClips())
            if (EngineHelpers::isTrackItemInRange(c, m_selectedRange.timeRange))
                clipContent->addClip(0, c->state);

        ct->deleteRegion({targetStart, targetEnd}, &m_editViewState.m_selectionManager);

        if (!copy)
            ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

        te::EditInsertPoint insertPoint(m_editViewState.m_edit);
        insertPoint.setNextInsertPoint(tracktion::TimePosition(), track);
        te::Clipboard::ContentType::EditPastingOptions options(m_editViewState.m_edit, insertPoint);
        options.selectionManager = &m_editViewState.m_selectionManager;
        options.startTime = editStart + duration;

        clipContent->pasteIntoEdit(options);

        for (auto &clip : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        {
            constrainClipInRange(clip, {targetStart, targetEnd});
            m_editViewState.m_selectionManager.deselect(clip);
        }
    }
}

void SongEditorView::constrainClipInRange(te::Clip *c, tracktion::TimeRange r)
{
    auto pos = c->getPosition();

    if (!r.intersects(c->getPosition().time))
    {
        c->removeFromParent();
    }
    else
    {
        if (pos.getStart() < r.getStart())
            c->setStart(r.getStart(), true, false);

        if (pos.getEnd() > r.getEnd())
            c->setEnd(r.getEnd(), true);
    }
}

tracktion::TimeDuration SongEditorView::distanceToTime(int distance)
{
    auto timePerPixel = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength().inSeconds() / getWidth();
    auto time = distance * timePerPixel;
    return tracktion::TimeDuration::fromSeconds(time);
}

tracktion::BeatPosition SongEditorView::xToBeatPosition(int x)
{
    auto visibleRange = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth());
    auto beatsPerPixel = visibleRange.getLength().inBeats() / getWidth();

    auto startBeat = visibleRange.getStart().inBeats();
    auto beatPosition = (x * beatsPerPixel) + startBeat;

    return tracktion::BeatPosition::fromBeats(beatPosition);
}
tracktion::TimePosition SongEditorView::xtoTime(int x) { return TimeUtils::xToTime(x, m_editViewState, m_timeLine.getTimeLineID(), getWidth()); }

tracktion::BeatPosition SongEditorView::getSnapedBeat(tracktion::BeatPosition beatPos, bool downwards)
{

    auto &ts = m_editViewState.m_edit.tempoSequence;
    auto timePos = ts.toTime(beatPos);
    auto snapTime = getSnappedTime(timePos, downwards);

    return ts.toBeats(snapTime);
}

tracktion::TimePosition SongEditorView::getSnappedTime(tracktion::TimePosition time, bool downwards) { return TimeUtils::getSnappedTime(time, m_editViewState, m_timeLine.getTimeLineID(), getWidth(), downwards); }

float SongEditorView::timeDurationToPixel(tracktion::TimeDuration duration)
{
    float timePerPixel = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength().inSeconds() / getWidth();
    return duration.inSeconds() / timePerPixel;
}
float SongEditorView::timeToX(tracktion::TimePosition time) { return TimeUtils::timeToX(time, m_editViewState, m_timeLine.getTimeLineID(), getWidth()); }

float SongEditorView::beatToX(tracktion::BeatPosition beat)
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    return m_editViewState.beatsToX(beat.inBeats(), getWidth(), x1, x2);
}

double SongEditorView::xToSnapedBeat(int x)
{
    auto time = xtoTime(x);
    time = getSnappedTime(time);
    return m_editViewState.timeToBeat(time.inSeconds());
}

juce::Rectangle<float> SongEditorView::getClipRect(te::Clip::Ptr clip)
{
    float x = timeToX(clip->getPosition().getStart());
    float y = static_cast<float>(getYForTrack(clip->getClipTrack()));
    float w = timeToX(clip->getPosition().getEnd()) - x;
    float h = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(clip->getClipTrack(), false));

    juce::Rectangle<float> clipRect = {x, y, w, h};
    return clipRect;
}

void SongEditorView::renderSelectedTimeRangeToNewTrack()
{
    if (getTracksWithSelectedTimeRange().size() <= 0)
        return;

    auto selectedTracks = m_selectedRange.selectedTracks;

    auto range = getSelectedTimeRange();

    EngineHelpers::renderToNewTrack(m_editViewState, selectedTracks, range);
}

void SongEditorView::buildRecordingClips(te::Track::Ptr track)
{
    bool needed = false;

    if (track->edit.getTransport().isRecording())
    {
        for (auto in : track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive() && track->itemID == in->getTargets().getFirst())
            {
                needed = true;
                break;
            }
        }
    }
    if (needed)
    {
        for (auto rc : m_recordingClips)
            if (rc->getTrack() == track)
                break;

        auto recordingClip = std::make_unique<RecordingClipComponent>(track, m_editViewState, m_timeLine);
        addAndMakeVisible(*recordingClip);
        m_recordingClips.add(std::move(recordingClip));
    }
    else
    {
        for (auto rc : m_recordingClips)
            if (rc->getTrack() == track)
                m_recordingClips.removeObject(rc, true);
    }
}

void SongEditorView::logMousePositionInfo()
{
    GUIHelpers::log("------------------------------------------------------------");
    GUIHelpers::log("ToolMode    : ", (int)m_toolMode);
    GUIHelpers::log("Track       : ", m_hoveredTrack != nullptr);
    if (m_hoveredTrack)
    {
        GUIHelpers::log("Track : ", m_hoveredTrack->getName());
    }

    repaint();
}

void SongEditorView::startDrag(DragType type, tracktion::TimePosition time, juce::Point<int> pos)
{
    m_dragState.startDrag(type, time, pos);
    m_isDragging = true;

    if (type == DragType::TimeRangeLeft || type == DragType::TimeRangeRight || type == DragType::TimeRangeMove)
    {
        m_isDraggingSelectedTimeRange = true;
    }
}

void SongEditorView::startDrag(DragType type, tracktion::TimePosition time, juce::Point<int> pos, tracktion::EditItemID itemId)
{
    m_dragState.startDrag(type, time, pos, itemId);
    m_isDragging = true;

    if (type == DragType::TimeRangeLeft || type == DragType::TimeRangeRight || type == DragType::TimeRangeMove)
    {
        m_isDraggingSelectedTimeRange = true;
    }
}

void SongEditorView::updateDrag(tracktion::TimePosition time, juce::Point<int> pos)
{
    m_dragState.currentTime = time;

    if (m_dragState.isTimeRangeDrag())
    {
        if (m_dragState.type == DragType::TimeRangeLeft)
        {
            updateTimeRangeDragResizeLeft(time);
        }
        else if (m_dragState.type == DragType::TimeRangeRight)
        {
            updateTimeRangeDragResizeRight(time);
        }
        else if (m_dragState.type == DragType::TimeRangeMove)
        {
            auto delta = time - m_dragState.startTime;
            updateTimeRangeDragMove(delta);
        }
    }
}

void SongEditorView::endDrag()
{
    if (m_dragState.isTimeRangeDrag())
    {
        finishTimeRangeDrag(false);
    }

    m_dragState.reset();
    m_isDragging = false;
    m_isDraggingSelectedTimeRange = false;
}

//==============================================================================
// TimeRangeOverlayComponent Implementation
//==============================================================================

SongEditorView::TimeRangeOverlayComponent::TimeRangeOverlayComponent(SongEditorView &owner)
    : m_owner(owner)
{
    setInterceptsMouseClicks(true, false); // Handle mouse, but don't block child mouse Move unless hitTest passes
}

bool SongEditorView::TimeRangeOverlayComponent::hitTest(int x, int y)
{
    // If no range is selected, we are invisible to mouse
    if (m_owner.m_selectedRange.selectedTracks.isEmpty() && m_owner.m_selectedRange.selectedAutomations.isEmpty())
        return false;

    if (m_owner.m_selectedRange.timeRange.getLength().inSeconds() <= 0)
        return false;

    bool left = false, right = false;
    // We check ALL tracks/automations because the overlay covers everything
    // Use the first track to determine if we are within the horizontal bounds of the selection
    if (m_owner.hitTestTimeRange(x, m_owner.m_selectedRange.selectedTracks.getFirst(), left, right))
    {
        // Check if Y is actually within one of the selected tracks or automations
        // Otherwise a click on a non-selected track in the same time range would be intercepted
        auto track = m_owner.getTrackAt(y);
        if (track != nullptr && m_owner.m_selectedRange.selectedTracks.contains(track))
            return true;

        // Check automations
        for (auto ap : m_owner.m_selectedRange.selectedAutomations)
        {
            if (m_owner.getAutomationRect(ap).contains(static_cast<float>(x), static_cast<float>(y)))
                return true;
        }
    }

    return false;
}

void SongEditorView::TimeRangeOverlayComponent::paint(juce::Graphics &g)
{
    using namespace juce::Colours;
    const auto area = getLocalBounds().toFloat();

    for (auto track : te::getAllTracks(m_owner.m_editViewState.m_edit))
    {
        bool isShowable = m_owner.m_editViewState.m_trackHeightManager->isTrackShowable(track);
        bool isVisible = !(m_owner.m_editViewState.m_trackHeightManager->isTrackInMinimizedFolderRecursive(track));
        bool isSelected = m_owner.m_selectedRange.selectedTracks.contains(track);
        bool isTimeRangeValid = m_owner.m_selectedRange.getLength().inSeconds() > 0;

        if (isShowable && isVisible && isSelected && isTimeRangeValid)
        {
            float y = static_cast<float>(m_owner.getYForTrack(track));
            float h = static_cast<float>(m_owner.m_editViewState.m_trackHeightManager->getTrackHeight(track, false));
            float rangeX = m_owner.timeToX(m_owner.m_selectedRange.getStart());
            float rangeW = m_owner.timeToX(m_owner.m_selectedRange.getEnd()) - rangeX;
            g.setColour(juce::Colour(0x50ffffff));
            juce::Rectangle<float> timeRangeRect(rangeX, y, rangeW, h);
            timeRangeRect = timeRangeRect.getIntersection(area);

            g.fillRect(timeRangeRect);
        }

        for (auto &ap : track->getAllAutomatableParams())
        {
            if (m_owner.m_editViewState.m_trackHeightManager->isAutomationVisible(*ap) == false)
                continue;

            auto rect = m_owner.getAutomationRect(ap);

            if (rect.getHeight() <= 0)
                continue;

            if (m_owner.m_selectedRange.selectedAutomations.contains(ap) == false)
                continue;

            if (m_owner.m_selectedRange.getLength().inSeconds() <= 0)
                continue;

            float rangeX = m_owner.timeToX(m_owner.m_selectedRange.getStart());
            float rangeY = rect.getY();
            float rangeW = m_owner.timeToX(m_owner.m_selectedRange.getEnd()) - rangeX;
            float rangeH = rect.getHeight();

            g.setColour(juce::Colour(0x50ffffff));
            juce::Rectangle<float> automationRangeRect(rangeX, rangeY, rangeW, rangeH);
            automationRangeRect = automationRangeRect.getIntersection(area);
            g.fillRect(automationRangeRect);
        }
    }

    if (m_owner.m_isDraggingSelectedTimeRange)
    {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillAll();
        juce::Rectangle<float> selectedRangeRect;

        for (auto track : m_owner.m_selectedRange.selectedTracks)
        {
            float x = m_owner.timeToX(m_owner.m_selectedRange.getStart());
            float y = static_cast<float>(m_owner.getYForTrack(track));
            float w = m_owner.timeToX(m_owner.m_selectedRange.getEnd()) - x;
            float h = static_cast<float>(m_owner.m_editViewState.m_trackHeightManager->getTrackHeight(track, false));

            x = x + m_owner.timeDurationToPixel(m_owner.m_draggedTimeDelta);

            juce::Rectangle<float> rect(x, y, w, h);

            selectedRangeRect = selectedRangeRect.getUnion(rect);
            if (auto ct = dynamic_cast<te::ClipTrack *>(track))
            {
                GUIHelpers::drawTrack(g, m_owner, m_owner.m_editViewState, rect, ct, m_owner.m_selectedRange.timeRange, true);
            }
            else if (track->isFolderTrack())
            {
                auto beatX1 = m_owner.m_editViewState.timeToBeat(m_owner.m_selectedRange.getStart().inSeconds());
                auto beatX2 = m_owner.m_editViewState.timeToBeat(m_owner.m_selectedRange.getEnd().inSeconds());

                GUIHelpers::drawBarsAndBeatLines(g, m_owner.m_editViewState, beatX1, beatX2, rect);
            }
        }

        for (auto automation : m_owner.m_selectedRange.selectedAutomations)
        {
            auto rect = m_owner.getAutomationRect(automation);

            if (rect.getHeight() <= 0)
                continue;

            if (!m_owner.m_selectedRange.selectedAutomations.contains(automation))
                continue;

            if (m_owner.m_selectedRange.getLength().inSeconds() <= 0)
                continue;

            float rangeX = m_owner.timeToX(m_owner.m_selectedRange.getStart());
            float rangeY = rect.getY();
            float rangeW = m_owner.timeToX(m_owner.m_selectedRange.getEnd()) - rangeX;
            float rangeH = rect.getHeight();

            juce::Rectangle<float> automationRangeRect(rangeX, rangeY, rangeW, rangeH);
            automationRangeRect = automationRangeRect.getIntersection(area);
            automationRangeRect.setX(rangeX + m_owner.timeDurationToPixel(m_owner.m_draggedTimeDelta));
            if (auto al = m_owner.getAutomationLane(automation))
                al->drawAutomationLane(g, m_owner.m_selectedRange.timeRange, automationRangeRect);

            selectedRangeRect = selectedRangeRect.getUnion(automationRangeRect);
        }

        g.setColour(juce::Colours::yellowgreen);
        selectedRangeRect = selectedRangeRect.getIntersection(area);
        g.drawRect(selectedRangeRect, 1.0f);
    }

    if (m_hoveredHandleLeft)
    {
        auto lastSelectedTrack = m_owner.m_selectedRange.selectedTracks.getLast();
        auto height = static_cast<float>(m_owner.m_editViewState.m_trackHeightManager->getTrackHeight(lastSelectedTrack, true));
        float x = m_owner.timeToX(m_owner.m_selectedRange.getStart());
        float y = static_cast<float>(m_owner.getYForTrack(m_owner.m_selectedRange.selectedTracks.getLast())) + height;
        g.setColour(yellowgreen);
        g.drawLine(x, 0.0f, x, y, 1.0f);
    }

    if (m_hoveredHandleRight)
    {
        auto lastSelectedTrack = m_owner.m_selectedRange.selectedTracks.getLast();
        auto height = static_cast<float>(m_owner.m_editViewState.m_trackHeightManager->getTrackHeight(lastSelectedTrack, true));
        float x = m_owner.timeToX(m_owner.m_selectedRange.getEnd());
        float y = static_cast<float>(m_owner.getYForTrack(m_owner.m_selectedRange.selectedTracks.getLast())) + height;
        g.setColour(yellowgreen);
        g.drawLine(x, 0.0f, x, y, 1.0f);
    }
}

void SongEditorView::TimeRangeOverlayComponent::mouseMove(const juce::MouseEvent &e)
{
    bool left = false, right = false;
    if (m_owner.m_selectedRange.selectedTracks.size() > 0)
    {
        m_owner.hitTestTimeRange(e.x, m_owner.m_selectedRange.selectedTracks.getFirst(), left, right);
    }
    else if (m_owner.m_selectedRange.selectedAutomations.size() > 0)
    {
        m_owner.hitTestTimeRange(e.x, m_owner.m_selectedRange.selectedAutomations.getFirst(), left, right);
    }

    m_hoveredHandleLeft = left;
    m_hoveredHandleRight = right;

    if (left)
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
    else if (right)
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    else
        setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, m_owner.m_editViewState.m_applicationState.m_mouseCursorScale));
}

void SongEditorView::TimeRangeOverlayComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isLeftButtonDown())
    {
        bool left = false, right = false;
        te::Track *track = nullptr;
        if (m_owner.m_selectedRange.selectedTracks.size() > 0)
            track = m_owner.m_selectedRange.selectedTracks.getFirst();

        if (track && m_owner.hitTestTimeRange(e.x, track, left, right))
        {
            DragType dragType = left ? DragType::TimeRangeLeft : (right ? DragType::TimeRangeRight : DragType::TimeRangeMove);
            m_owner.startDrag(dragType, m_owner.xtoTime(e.x), e.getPosition());
            m_owner.m_dragState.isLeftEdge = left;
            m_owner.m_dragState.isRightEdge = right;
            m_owner.startTimeRangeDrag();
        }
    }
}

void SongEditorView::TimeRangeOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto &dragState = m_owner.getDragState();
    if (dragState.isTimeRangeDrag())
    {
        auto currentTime = m_owner.xtoTime(e.x);
        if (!e.mods.isShiftDown())
            currentTime = m_owner.getSnappedTime(currentTime);

        if (dragState.isLeftEdge)
            m_owner.updateTimeRangeDragResizeLeft(currentTime);
        else if (dragState.isRightEdge)
            m_owner.updateTimeRangeDragResizeRight(currentTime);
        else
        {
            auto snappedStart = dragState.startTime;
            if (!e.mods.isShiftDown())
                snappedStart = m_owner.getSnappedTime(dragState.startTime, true);

            auto draggedDuration = currentTime - snappedStart;
            m_owner.updateTimeRangeDragMove(draggedDuration);
        }
    }
}

void SongEditorView::TimeRangeOverlayComponent::mouseUp(const juce::MouseEvent &e)
{
    if (m_owner.getDragState().isTimeRangeDrag())
    {
        if (e.mouseWasDraggedSinceMouseDown())
            m_owner.finishTimeRangeDrag(e.mods.isCtrlDown());
        else
        {
            m_owner.cancelTimeRangeDrag();
            m_owner.clearSelectedTimeRange();
        }
        m_owner.endDrag();
    }
}

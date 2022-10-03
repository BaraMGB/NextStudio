#include "SongEditorView.h"
#include "Utilities.h"


SongEditorView::SongEditorView(EditViewState& evs)
        : m_editViewState(evs)
        , m_lassoComponent(evs, evs.m_viewX1, evs.m_viewX2)
{
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
}
int SongEditorView::getTrackHeight(const TrackComponent* tc) const
{
    return GUIHelpers::getTrackHeight(tc->getTrack(),m_editViewState, true);
}

void SongEditorView::paint(juce::Graphics& g)
{
	auto area = getLocalBounds();
	g.setColour(juce::Colour(0xff303030));
	g.fillRect(area);
}
void SongEditorView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    for (auto trackView : m_trackViews)
    {
        auto trackHeight = getTrackHeight(trackView);

        if (auto ft = trackView->getTrack()->getParentFolderTrack())
            if (ft->state.getProperty(IDs::isTrackMinimized))
                trackHeight = 0;

        trackView->setBounds (0, y, getWidth(), trackHeight);
        y += trackHeight;
    }
    m_lassoComponent.setBounds(getLocalBounds());
}

void SongEditorView::mouseMove (const juce::MouseEvent &e)
{
}
void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_editViewState.m_selectionManager.deselectAll();
    startLasso(e, false, e.mods.isAltDown());
}

void SongEditorView::mouseDrag(const juce::MouseEvent&e)
{
    updateLasso (e);
}
void SongEditorView::mouseUp(const juce::MouseEvent& e)
{
    if (m_lassoComponent.isVisible () || m_selectTimerange)
    {
        stopLasso();
    }
	if(!e.mouseWasDraggedSinceMouseDown())
	{
        m_editViewState.m_selectionManager.deselectAll ();
		m_editViewState.m_edit.getTransport().setCurrentPosition(getSnapedTime(xtoTime(e.x), true));	
	}
}

bool SongEditorView::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& sourceDetails)
{
    if (sourceDetails.description == "Clip")
	      return true;
    return false;
}
void SongEditorView::itemDragMove(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    turnoffAllTrackOverlays();

    if (auto draggedClip = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get ()))
    {
        const auto dropPos = dragSourceDetails.localPosition;
        int verticalOffset = getVerticalOffset(dragSourceDetails, dropPos);
        m_draggedTimeDelta = xtoTime(dropPos.getX() - draggedClip->getClickPosOffsetX()) - draggedClip->getClip()->getPosition().getStart().inSeconds();

        if (draggedClip->isResizeLeft() || draggedClip->isResizeRight())
        {
            drawResizingOverlays (draggedClip);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);

            drawDraggingOverlays (draggedClip, dropPos, verticalOffset);
        }
    }
}
void SongEditorView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    turnoffAllTrackOverlays();
    auto dropPos = dragSourceDetails.localPosition;
    auto dropTime = m_editViewState.xToTime (
        dropPos.getX()
        , getWidth()
        , m_editViewState.m_viewX1
        , m_editViewState.m_viewX2);
    dropTime = juce::jlimit(0.0,(double) m_editViewState.m_viewX2, dropTime);
    
	//copy/moving selected clips by drag and drop // resizing
    if (auto draggedClip = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get ()))
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        auto verticalOffset = getVerticalOffset(dragSourceDetails, dropPos);
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto startTime = draggedClip->getClip()->getPosition().getStart().inSeconds();
        auto copy = draggedClip->isCtrlDown();
        auto snap = !draggedClip->isShiftDown();
        auto resizeL = draggedClip->isResizeLeft();
        auto resizeR = draggedClip->isResizeRight();

        if (resizeR || resizeL)
            resizeSelectedClips(snap, resizeL);
        else
            moveSelectedClips(startTime, copy, snap, m_draggedTimeDelta, verticalOffset);

        draggedClip->setResizeLeft(false);
        draggedClip->setResizeRight(false);
    }

    m_draggedTimeDelta = 0;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}
void SongEditorView::itemDragExit(const juce::DragAndDropTarget::SourceDetails&)
{
    turnoffAllTrackOverlays();
}

juce::OwnedArray<TrackComponent>& SongEditorView::getTrackViews()
{
    return m_trackViews;
}
void SongEditorView::addTrackView(TrackComponent& tc)
{
    m_trackViews.add(&tc);
}
void SongEditorView::updateViews()
{
    for (auto v : m_trackViews)
        addAndMakeVisible(v);

    resized();
}
TrackComponent* SongEditorView::getTrackCompForTrack(const tracktion_engine::Track::Ptr& track)
{
    for (auto &tc : m_trackViews)
        if (tc->getTrack () == track)
            return tc;

    return nullptr;
}

void SongEditorView::clearTracks()
{
    m_trackViews.clear(true);
    resized();
}
int SongEditorView::countTracks()
{
    return m_trackViews.size();
}

void SongEditorView::drawDraggingOverlays (const ClipComponent *draggedClip, const juce::Point<int> &dropPos,
                                           int verticalOffset)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
    for (auto sc: selectedClips)
    {
        if (auto newTrackForSelectedClip = getTrackForClip (verticalOffset, sc))
        {
            bool isValid = EngineHelpers::trackWantsClip (sc, newTrackForSelectedClip->getTrack());

            if (auto selectedClipView = getClipComponentForClip (sc))
            {
                auto startOffsetX = selectedClipView->getPosition().x - draggedClip->getPosition().x;
                auto insertPos = dropPos.getX() - draggedClip->getClickPosOffsetX();
                insertPos = draggedClip->isShiftDown() ? insertPos : getSnapedX(insertPos);

                newTrackForSelectedClip->addDraggedClip(isValid, insertPos + startOffsetX, selectedClipView->getWidth(), false); 
                newTrackForSelectedClip->repaint();
            }
        }
    }
}

void SongEditorView::drawResizingOverlays (const ClipComponent *draggedClip)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
    for (auto sc: selectedClips)
    {
        auto newStart = sc->getPosition().getStart().inSeconds();
        auto newEnd = sc->getPosition().getEnd().inSeconds();

        if (draggedClip->isResizeRight())
        {
            newEnd = newEnd + m_draggedTimeDelta;
            if (!draggedClip->isShiftDown())
                newEnd = getSnapedTime (newEnd);
        }
        else
        {
            double clipStart = sc->getPosition ().getStart ().inSeconds() - sc->getPosition ().getOffset ().inSeconds();
            newStart = juce::jmax (clipStart, newStart + m_draggedTimeDelta);

            if (!draggedClip->isShiftDown())
                newStart = juce::jmax (clipStart, getSnapedTime (newStart));
        }

        if (auto track = getTrackForClip (0, sc))
        {
            track->addDraggedClip(true, timeToX(newStart), timeToX(newEnd) - timeToX(newStart), true);
            track->repaint();
        }
    }
}

void SongEditorView::startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange)
{
    m_lassoComponent.startLasso({e.x, e.y}, m_editViewState.m_viewY, selectRange);
    m_selectTimerange = selectRange;
    m_automationClicked = fromAutomation;
    if (selectRange)
    {
        clearSelectedTimeRange();
        m_cachedSelectedAutomation.clear();
        m_cachedSelectedClips.clear();

    }
    else 
    {
        if (fromAutomation)
        {
            updateAutomationCache();
        }
        else
        {
            updateClipCache();    
        }
    }
}
void SongEditorView::updateLasso(const juce::MouseEvent& e )
{
    if (m_lassoComponent.isVisible () || m_selectTimerange)
    {
        m_lassoComponent.updateLasso({e.x, e.y}, m_editViewState.m_viewY);
        if (m_selectTimerange)
        {
            updateRangeSelection();
        }
        else
        {
            if (m_automationClicked)
            {
                updateAutomationSelection(e.mods.isShiftDown());
            }
            else
            {
                updateClipSelection(e.mods.isShiftDown());
            }
        }
    }
}

void SongEditorView::stopLasso()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    m_lassoComponent.stopLasso();
    m_automationClicked = false;
    m_selectTimerange = false;  
}
void SongEditorView::duplicateSelectedClips()
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

    auto range = te::getTimeRangeForSelectedItems(selectedClips);
    auto delta = range.getLength().inSeconds();
    auto sourceTime = range.getStart().inSeconds();
    if (auto cc = getClipComponentForClip(selectedClips.getFirst()))
    {
        moveSelectedClips(sourceTime, true, false, delta, 0);
    }
}

void SongEditorView::moveSelectedClips(double sourceTime, bool copy, bool snap, double delta, int verticalOffset)
{
    auto snapType = m_editViewState.getBestSnapType(
                        m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth());
    EngineHelpers::moveSelectedClips(sourceTime, copy, snap, delta, verticalOffset,m_editViewState, snapType); 
}
TrackComponent *SongEditorView::getTrackForClip(int verticalOffset, const te::Clip *clip)
{
    auto idx = m_trackViews.indexOf(getTrackCompForTrack(clip->getTrack()));
    return m_trackViews[idx + verticalOffset];
}
int SongEditorView::getVerticalOffset(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails,
    const juce::Point<int>& dropPos)
{
    auto targetTrackComp = getTrackComponent(dropPos.getY ());
    auto sourceTrackComp = dynamic_cast<TrackComponent*>
        (dragSourceDetails.sourceComponent.get()->getParentComponent ());
	if (targetTrackComp)
	{
		auto verticalOffset = m_trackViews.indexOf(targetTrackComp)
		                    - m_trackViews.indexOf(sourceTrackComp);
		return verticalOffset;
	}
	return 0;
}
int SongEditorView::getSnapedX(int x, bool down) const
{
    auto snapType = m_editViewState.getBestSnapType(
        m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth());
    int snapedX = m_editViewState.snapedX(x, getWidth(),
                                          snapType, m_editViewState.m_viewX1, m_editViewState.m_viewX2, down);
    return snapedX;
}
void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge)
{
    auto snapType = m_editViewState.getBestSnapType (
        m_editViewState.m_viewX1
        , m_editViewState.m_viewX2
        , getWidth());

    EngineHelpers::resizeSelectedClips(snap, fromLeftEdge, m_draggedTimeDelta, m_editViewState, snapType);
}
void SongEditorView::addWaveFileToNewTrack(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails, double dropTime) const
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
        auto f = fileTreeComp->getSelectedFile();
        EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, juce::Colour(0xffff33cc), dropTime);
    }
}
double SongEditorView::getPasteTime(double dropTime,
                                    ClipComponent* draggedClip) const
{
    const auto firstClipTime = draggedClip->getClip ()->getPosition ().getStart ();
    const auto offset = draggedClip->getClickPosTime ();
    const auto snap = !draggedClip->isShiftDown ();
    const auto xTime = m_editViewState.beatToTime(m_editViewState.m_viewX1);
    const auto rawTime = juce::jmax(0.0, dropTime - offset + xTime);
    auto snapType = m_editViewState.getBestSnapType (
        m_editViewState.m_viewX1
        , m_editViewState.m_viewX2
        , getWidth());
    const auto snapedTime = m_editViewState.getSnapedTime (rawTime, snapType);
    const auto pasteTime = !snap
                               ? rawTime - firstClipTime.inSeconds()
                               : snapedTime - firstClipTime.inSeconds();
    return pasteTime;
}
TrackComponent* SongEditorView::getTrackComponent(int y)
{	
	if (m_trackViews.isEmpty())
	    return nullptr;
	if (y < 0)
		return m_trackViews.getFirst();

    auto tcHeight = 0;
    for (auto & tc : m_trackViews)
    {
        if (y - m_editViewState.m_viewY > tcHeight
            && y - m_editViewState.m_viewY <= tcHeight + tc->getHeight ())
        {
            return tc;
        }
        tcHeight = tcHeight + tc->getHeight ();
    }
	return m_trackViews.getLast();
}

juce::OwnedArray<TrackComponent>& SongEditorView::getTrackComps ()
{
    return m_trackViews;
}


ClipComponent* SongEditorView::getClipComponentForClip(const tracktion_engine::Clip::Ptr& clip)
{
    for (auto& track : m_trackViews)
    {
        for (auto &c : track->getClipComponents ())
        {
            if (c->getClip () == clip)
            {
                return c;
            }
        }
    }
    return nullptr;
}
void SongEditorView::updateClipCache()
{
    clearSelectedTimeRange();
    m_cachedSelectedAutomation.clear();
    m_cachedSelectedClips.clear();

    for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        m_cachedSelectedClips.add(c);
}

void SongEditorView::updateRangeSelection()
{
    auto& sm = m_editViewState.m_selectionManager;
    sm.deselectAll();
   
    double scrollY = m_editViewState.m_viewY;
    auto range = m_lassoComponent.getLassoRect().m_timeRange;
    clearSelectedTimeRange();
    for (auto tv: m_trackViews)
    {
        auto trackVRange = getVerticalRangeOfTrack(scrollY, tv->getTrack(), false);
        auto lassoRangeY = m_lassoComponent.getLassoRect ().m_verticalRange;
        if (trackVRange.intersects (lassoRangeY) && !(tv->getTrack()->isFolderTrack()))
        {
            tv->setSelectedTimeRange(range);
        }
        scrollY += trackVRange.getLength();

        for (auto al : tv->getAutomationLanes())
        {
            juce::Range<double> r (scrollY,al->getHeight());
            if (r.intersects(lassoRangeY))
            {
                al->setSelectedTimeRange(range);
            }
            scrollY += al->getHeight();
            al->repaint();
        }
        tv->repaint();
    }

}

void SongEditorView::clearSelectedTimeRange()
{
    for (auto tv : m_trackViews)
    {
        tv->clearSelectedTimeRange();
        for (auto al : tv->getAutomationLanes())
            al->clearSelectedTimeRange();
    }

}

void SongEditorView::updateAutomationSelection(bool add)
{
    auto& sm = m_editViewState.m_selectionManager;
    sm.deselectAll();

    clearSelectedTimeRange();
    double scrollY = m_editViewState.m_viewY;
    for (auto tv: m_trackViews)
    {
        scrollY += GUIHelpers::getTrackHeight(tv->getTrack(), m_editViewState, false);
        if (!tv->getTrack()->state.getProperty(IDs::isTrackMinimized))
        {
            for (auto al : tv->getAutomationLanes())
            {
                auto range = m_lassoComponent.getLassoRect().m_timeRange;
                auto firstPoint = al->getCurve().nextIndexAfter(range.getStart());
                auto lastPoint = al->getCurve().indexBefore(range.getEnd());
                for (auto i = firstPoint ; i <= lastPoint; i++)
                {
                    auto p = al->getCurve().getPoint(i);

                    auto pointPos = al->getPointXY(p.time, p.value);
                    pointPos.addXY(0, scrollY);
                    if (m_lassoComponent.getLassoRect().m_verticalRange.contains(pointPos.getY()))
                       al->selectPoint(i, true);
                }
                scrollY += al->getHeight();
            }
        }
    }


//    if (add)
//        for (auto p : m_cachedSelectedAutomation)
//            m_editViewState.m_selectionManager.addToSelection(p);
}

void SongEditorView::updateAutomationCache()
{
    GUIHelpers::log("Automation cache updated...");
    m_cachedSelectedAutomation.clear();
    m_cachedSelectedClips.clear();

    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<AutomationPoint>())
        m_cachedSelectedAutomation.add(p);
}
void SongEditorView::updateClipSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    double scrollY = m_editViewState.m_viewY;
    for (auto tv: m_trackViews)
    {

        if (auto track = tv->getTrack())
        {
            auto lassoRangeY = m_lassoComponent.getLassoRect ().m_verticalRange;
            if (getVerticalRangeOfTrack(scrollY, track, false).intersects (lassoRangeY) && !(track->isFolderTrack()))
            {
                selectCatchedClips(track);
            }
            scrollY += tv->getHeight();
        }
    }

    if (add)
    {
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
    }
}
juce::Range<double> SongEditorView::getVerticalRangeOfTrack(
    double scrollY, tracktion_engine::Track* track, bool withAutomation) const
{
    auto trackTop = (double) scrollY;
    auto trackBottom = trackTop + (double) GUIHelpers::getTrackHeight(track, m_editViewState, withAutomation);

    return {trackTop , trackBottom};
}
void SongEditorView::selectCatchedClips(const tracktion_engine::Track *track)
{
    for (auto ti = 0; ti < track->getNumTrackItems(); ti++)
    {
        auto item = track->getTrackItem(ti);
        if (m_lassoComponent.getLassoRect ().m_startTime < item->getPosition ().getEnd ().inSeconds()
                && m_lassoComponent.getLassoRect ().m_endTime > item->getPosition ().getStart ().inSeconds())
        {
            m_editViewState.m_selectionManager.addToSelection(item);
        }
    }
}

LassoSelectionTool& SongEditorView::getLasso()
{
    return m_lassoComponent;
}
void SongEditorView::turnoffAllTrackOverlays()
{
    for (auto &tc : getTrackComps ())
    {
        tc->clearDraggedClips();
    }
}

double SongEditorView::xtoTime(int x)
{
    return m_editViewState.xToTime(x, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

double SongEditorView::getSnapedTime(double time, bool downwards)
{
    return m_editViewState.getSnapedTime(
            time,
			m_editViewState.getBestSnapType(
                    m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth()),
			downwards);
}

int SongEditorView::timeToX (double time)
{
    return m_editViewState.timeToX (time, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

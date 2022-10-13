#include "SongEditorView.h"
#include "AutomationLaneComponent.h"
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"
#include <memory>


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
    m_hoveredTrack = getTrackAt(e.y);
    m_hoveredClip = nullptr;
    m_hoveredAutamatableParam = getAutomatableParamAt(e.y);
    m_hoveredAutomationPoint = -1;
    m_hoveredCurve = -1;
    m_leftBorderHovered = false;
    m_rightBorderHovered = false;

    auto mousePosTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));

    if (m_hoveredAutamatableParam)
    {
        auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);
        auto alEvent = e.getEventRelativeTo(al);
        auto curve = m_hoveredAutamatableParam->getCurve();
        GUIHelpers::log("Parameter: ",m_hoveredAutamatableParam->getFullName());

        const auto hoveredRect =  al->getHoveredRect(alEvent);   
        for (auto i = 0; curve.getNumPoints() > i; i++)
        {
            auto pointXy = al->getPointXY(curve.getPointTime(i),
                                          curve.getPointValue(i));

            if (hoveredRect.contains(pointXy))
            {
                m_hoveredAutomationPoint = i;
                GUIHelpers::log("Point : " ,i);
            }
        }

        auto valueAtMouseTime = m_hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
        auto curvePointAtMouseTime = juce::Point<int>(alEvent.x,al->getYPos(valueAtMouseTime));

        if (hoveredRect.contains(curvePointAtMouseTime) && m_hoveredAutomationPoint == -1)
        {
            m_hoveredCurve = curve.nextIndexAfter(mousePosTime);
            m_curveAtMousedown = al->getCurve().getPoint(m_hoveredCurve - 1).curve;
            GUIHelpers::log("curve:hovered: " , m_hoveredCurve);
        }
        
        auto cp = al->getPointXY(mousePosTime, valueAtMouseTime);
        auto rect = al->getRectFromPoint(cp);

        al->setHover(m_hoveredAutomationPoint, m_hoveredCurve, rect.toNearestInt());
        al->repaint();
    }

    if (auto at = dynamic_cast<te::AudioTrack*>(m_hoveredTrack.get()))
    {
        GUIHelpers::log("Track: " , m_hoveredTrack->getName());
        for (auto clip : at->getClips())
        {
            if (clip->getEditTimeRange().contains(mousePosTime))
            {
                m_hoveredClip = clip;
                if (auto cc = getClipComponentForClip(clip))
                {
                    GUIHelpers::log("Clip: " , cc->getClip()->getName());
                    if (e.getPosition().getX() < cc->getX() + 10 && cc->getWidth () > 30)
                    { 
                        GUIHelpers::log("Left Border: " , cc->getClip()->getName());
                        m_leftBorderHovered = true;
                    }
                    else if (e.getPosition().getX() > cc->getRight() - 10
                         &&  cc->getWidth () > 30)

                    {
                        GUIHelpers::log("Right Border: " , cc->getClip()->getName());
                        m_rightBorderHovered = true;

                    }
                }
            }
        }
    }

    if (m_hoveredTrack && m_hoveredTrack->isFolderTrack())
    {
        GUIHelpers::log("FolderTrack : ", m_hoveredTrack->getName());
    }

    
}
void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_draggedClipComponent = nullptr;
    m_draggedTimeDelta = 0.0;
    m_timeOfHoveredAutomationPoint = tracktion::TimePosition::fromSeconds(0.0);
    auto &sm = m_editViewState.m_selectionManager;

    if (e.mods.isRightButtonDown())
    {
       if (m_hoveredAutamatableParam && m_hoveredAutomationPoint != -1)
            m_hoveredAutamatableParam->getCurve().removePoint(m_hoveredAutomationPoint);
    }
    else if (e.mods.isLeftButtonDown())
    {
        if (e.mods.isCtrlDown()) 
        {
            if (m_hoveredClip)
            {

            }
        }
        else if (e.mods.isShiftDown())
        {

        }
        else if (e.mods.isAltDown())
        {
            startLasso(e, true, true);
        }
        else 
        {
            if (m_hoveredClip)
            {
                if (!sm.isSelected(m_hoveredClip))
                {
                    sm.selectOnly(m_hoveredClip);
                }
                m_clipPosAtMouseDown = m_hoveredClip->getPosition().getStart().inSeconds();
            }
            else if (m_hoveredAutomationPoint != -1)
            {

                if (auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam))
                {
                    if (!al->isPointSelected(m_hoveredAutomationPoint))
                        al->selectPoint(m_hoveredAutomationPoint, false);
                }
            }
            else if (m_hoveredCurve != -1)
            {
                if (auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam))
                {
                    auto mousePosTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
                    auto valueAtMouseTime = m_hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
                    
                    auto p = al->getCurve().addPoint(mousePosTime, valueAtMouseTime, 0.f);
                    m_hoveredCurve = -1;
                    m_hoveredAutomationPoint = p;
                    al->selectPoint(p, false);
                    al->setIsDragging(true);
                }
            }
            else if (m_hoveredAutamatableParam)
            {
                startLasso(e, true, false);
            }
            else if (m_hoveredTrack)
            {
                sm.deselectAll();
                startLasso(e, false, false);
            }
        }


        m_selPointsAtMousedown.clear();
        for (auto p : sm.getItemsOfType<AutomationPoint>())
        {
            auto cp = new AutomationLaneComponent::CurvePoint(
                                     p->m_curve.getPointTime(p->index),
                                     p->m_curve.getOwnerParameter()->valueRange.convertTo0to1(p->m_curve.getPointValue(p->index)),
                                     p->index,
                                    *p->m_curve.getOwnerParameter());

            m_selPointsAtMousedown.add(cp);
        }

    }

    if (m_hoveredAutamatableParam && m_hoveredAutomationPoint != -1)
        m_timeOfHoveredAutomationPoint = m_hoveredAutamatableParam->getCurve().getPointTime(m_hoveredAutomationPoint);
}

void SongEditorView::mouseDrag(const juce::MouseEvent&e)
{
    auto &sm = m_editViewState.m_selectionManager;
    auto screenStartTime = xtoTime(0);
    m_draggedTimeDelta = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime;

    if (!sm.isSelected(m_hoveredClip))
    {
        sm.addToSelection(m_hoveredClip);
    }

    if (m_lassoComponent.isVisible () || m_selectTimerange)
    {
        updateLasso (e);
    }

    if (m_hoveredAutomationPoint != -1 && e.mouseWasDraggedSinceMouseDown ())
    { 
        auto oldPos = m_timeOfHoveredAutomationPoint;
        auto newPos = tracktion::TimePosition::fromSeconds(xtoTime(e.x)); 

        //lock time
        if (e.mods.isShiftDown())
        {
            newPos = oldPos;
        }
        //snap
        else if (!e.mods.isCtrlDown())
        {
            newPos = tracktion::TimePosition::fromSeconds(getSnapedTime(xtoTime(e.x)));
        }
        
        auto draggedTime = newPos - oldPos;

        //move all selected Points 
        for (auto p : m_selPointsAtMousedown)
        {
            auto al = getAutomationLaneForAutomatableParameter(p->param);
            auto newTime =  p->time + draggedTime;

            auto newValue = p->value - al->getValue(al->getHeight() - e.getDistanceFromDragStartY());

            newValue = p->param.valueRange.convertFrom0to1(newValue);
            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }

        repaint();
    }
       //change curve steepness
    if (m_hoveredAutomationPoint == -1 && m_hoveredCurve != -1 && e.mods.isCtrlDown ())
    {
        if (m_hoveredAutamatableParam)
        {
            auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);

            al->setIsDragging(true);

            auto valueP1 = al->getCurve().getPointValue(m_hoveredCurve - 1);
            auto valueP2 = al->getCurve().getPointValue(m_hoveredCurve);
            auto delta =  valueP1 < valueP2 ? e.getDistanceFromDragStartY() *  0.01 
                                    : e.getDistanceFromDragStartY() * -0.01;
            auto newSteep = juce::jlimit(-0.5f, 0.5f, (float) (m_curveAtMousedown + delta));

            al->getCurve().setCurveValue(m_hoveredCurve - 1, newSteep);
  
            repaint();
           
        }
    }
 
}
void SongEditorView::mouseUp(const juce::MouseEvent& e)
{
    auto &sm = m_editViewState.m_selectionManager;

    if (m_lassoComponent.isVisible () || m_selectTimerange)
    {
        stopLasso();
    }

    if (e.mods.isLeftButtonDown())
    {
        if (auto cc = getClipComponentForClip(m_hoveredClip) && e.mouseWasDraggedSinceMouseDown())
        {
            auto snap = !e.mods.isShiftDown();
            auto snapType = m_editViewState.getBestSnapType(m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth());
            auto verticalOffset = getVerticalOffset(getTrackCompForTrack(m_hoveredClip->getTrack()), e.position.toInt());
    
            if (m_leftBorderHovered || m_rightBorderHovered)
            {
                EngineHelpers::resizeSelectedClips(snap, m_leftBorderHovered, m_draggedTimeDelta, m_editViewState, snapType);
            }
            else
            {
                EngineHelpers::moveSelectedClips(m_clipPosAtMouseDown, e.mods.isCtrlDown(), snap, m_draggedTimeDelta, verticalOffset, m_editViewState, snapType);
            }
        }
        else if (m_hoveredClip && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isCtrlDown())
        {
            sm.selectOnly(m_hoveredClip);
        }
        else if (m_hoveredClip && !e.mouseWasDraggedSinceMouseDown())
        {
            if (sm.isSelected(m_hoveredClip))
            {
                sm.deselect(m_hoveredClip);
            }
            else
            {
                sm.addToSelection(m_hoveredClip);
            }
        }
        else if (m_hoveredAutomationPoint != -1 && !e.mouseWasDraggedSinceMouseDown())
        {
            if (m_hoveredAutamatableParam)
            {
                if (e.mods.isCtrlDown())
                {
                    auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);
    
                    if (al->isPointSelected(m_hoveredAutomationPoint))
                        al->deselectPoint(m_hoveredAutomationPoint);
                    else
                        al->selectPoint(m_hoveredAutomationPoint, true);
                }
                else
                {
                    auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);
                    al->selectPoint(m_hoveredAutomationPoint, false);
                }
            }
        }
        else if (m_hoveredAutomationPoint == -1 && m_hoveredCurve == -1 && m_hoveredClip == nullptr && !e.mouseWasDraggedSinceMouseDown())
        {
            sm.deselectAll();
        }
    }

    if (m_hoveredAutamatableParam)
    {
        auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);
        al->setHover(-1, -1, juce::Rectangle<int>());
        al->setIsDragging(false);
    }

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
TrackComponent* SongEditorView::getTrackCompForTrack(tracktion_engine::Track::Ptr track)
{
    for (auto &tc : m_trackViews)
        if (tc->getTrack () == track)
            return tc;

    return nullptr;
}


AutomationLaneComponent *SongEditorView::getAutomationLaneForAutomatableParameter(te::AutomatableParameter::Ptr ap)
{
    auto tc = getTrackCompForTrack(ap->getTrack());
    for (auto al : tc->getAutomationLanes())
    {
        if (al->getCurve().getOwnerParameter() == ap.get())
        {
            return al;
        }
    }
    return nullptr;
}

te::Track::Ptr SongEditorView::getTrackAt(int y)
{
//per component
    for (auto tv : m_trackViews)
    {
        auto verticalRange = getVerticalRangeOfTrack(tv->getTrack(), false);
        if (verticalRange.contains(y))
        {
            return tv->getTrack();
        }
    }
//per te::track
    // for (auto t : m_editViewState.m_edit.getTrackList())
    // {
    //     auto trackHeight = GUIHelpers::getTrackHeight(t, m_editViewState, false);
    //     auto verticalRange = juce::Range<int>(scrollY, scrollY + trackHeight);
    //
    //     if (verticalRange.contains(y))
    //     {
    //         return t;
    //     }
    //     scrollY += GUIHelpers::getTrackHeight(t,m_editViewState, true);
    // }
    //
    
    return nullptr;
}

te::AutomatableParameter::Ptr SongEditorView::getAutomatableParamAt(int y)
{
    int scrollY = m_editViewState.m_viewY;
    //** per components
    for (auto tv : m_trackViews)
    {
        auto verticalRange = getVerticalRangeOfTrack(tv->getTrack(), false);
        scrollY += verticalRange.getLength();
        for (auto al : tv->getAutomationLanes())
        {
            if (juce::Range<int> (scrollY, scrollY + al->getHeight()).contains(y))
            {
                return al->getCurve().getOwnerParameter();
            }
            scrollY += al->getHeight();
        }
    }

    //** per te::tracks and 
  //   for (auto t : te::getAllTracks(m_editViewState.m_edit))
  //   {
  //       if (t->isAudioTrack() || t->isFolderTrack())
  //       {
  //           int trackHeight = GUIHelpers::getTrackHeight(t, m_editViewState, false);
  //           auto verticalRange = juce::Range<int>(scrollY, scrollY + trackHeight);
  //           for (auto p : t->getAllAutomatableParams())
  //           {
  //               int h = p->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));
  // 
  //               if (juce::Range<int>(scrollY + trackHeight, scrollY + trackHeight + h).contains(y))
  //               {
  //                   return p;
  //               }
  //           }
  // 
  //           scrollY += GUIHelpers::getTrackHeight(t,m_editViewState, true);
  //       }
  //   }
//
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

int SongEditorView::getVerticalOffset(TrackComponent* sourceTrackComp, const juce::Point<int>& dropPos)
{
    auto targetTrackComp = getTrackComponent(dropPos.getY ());

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
// void SongEditorView::addWaveFileToNewTrack(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails, double dropTime) const
// {
//     if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
//         (dragSourceDetails.sourceComponent.get()))
//     {
//         auto f = fileTreeComp->getSelectedFile();
//         EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, juce::Colour(0xffff33cc), dropTime);
//     }
// }
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
    clearSelectedTimeRange();
   
    auto scrollY = static_cast<double>(m_editViewState.m_viewY);
    auto range = m_lassoComponent.getLassoRect().m_timeRange;

    for (auto tv: m_trackViews)
    {
        auto trackVRange = getVerticalRangeOfTrack(tv->getTrack(), false);
        juce::Range<int> lassoRangeY = {(int) m_lassoComponent.getLassoRect ().m_verticalRange.getStart(),
                                        (int) m_lassoComponent.getLassoRect ().m_verticalRange.getEnd()};

        if (trackVRange.intersects (lassoRangeY) && !(tv->getTrack()->isFolderTrack()))
        {
            tv->setSelectedTimeRange(range);
        }

        scrollY += trackVRange.getLength();

        for (auto al : tv->getAutomationLanes())
        {
            juce::Range<int> r (scrollY,scrollY + al->getHeight());

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

    for (auto tv: m_trackViews)
    {

        if (auto track = tv->getTrack())
        {
            juce::Range<int> lassoRangeY = 
                {(int) m_lassoComponent.getLassoRect ().m_verticalRange.getStart(),
                 (int) m_lassoComponent.getLassoRect ().m_verticalRange.getEnd()};
            if (getVerticalRangeOfTrack(track, false).intersects (lassoRangeY) && !(track->isFolderTrack()))
            {
                selectCatchedClips(track);
            }
        }
    }

    if (add)
    {
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
    }
}
juce::Range<int> SongEditorView::getVerticalRangeOfTrack(
     tracktion_engine::Track::Ptr track, bool withAutomation) 
{   
    auto tv = getTrackCompForTrack(track);
    auto trackHeight = GUIHelpers::getTrackHeight(track, m_editViewState, withAutomation);

    return {tv->getY() , tv->getY() + trackHeight};
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

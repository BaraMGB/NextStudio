#include "SongEditorView.h"
#include "AutomationLaneComponent.h"
#include "Utilities.h"


SongEditorView::SongEditorView(EditViewState& evs, LowerRangeComponent& lr)
        : m_editViewState(evs)
        , m_lowerRange(lr)
        , m_lassoComponent(evs, evs.m_viewX1, evs.m_viewX2)
{
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
    clearSelectedTimeRange();
}

void SongEditorView::paint(juce::Graphics& g)
{
	auto area = getLocalBounds();
	g.setColour(juce::Colour(0xff303030));
	g.fillRect(area);
}


void SongEditorView::paintOverChildren (juce::Graphics& g)
{
    using namespace juce::Colours;
    auto &sm = m_editViewState.m_selectionManager;
    auto scroll = timeToX(0) * (-1);


    if (m_isDraggingSelectedTimeRange)
    {
        auto x = timeToX(m_selectedRange.getStart().inSeconds() + m_draggedTimeDelta);
        auto y = getYForTrack(m_selectedRange.selectedTracks.getFirst());
        g.drawImageAt(m_timeRangeImage, x, y, false);
    }


    if (m_draggedClipComponent)
    {
        for (auto sc : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(sc->getTrack(), m_draggedVerticalOffset))
            {
                auto cc = getClipViewForClip(sc);
                juce::Rectangle<int> targetRect = {cc->getX() + timeToX(m_draggedTimeDelta) + scroll,
                                                       getYForTrack(targetTrack),
                                                       cc->getWidth(),
                                                        GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};
               
                if (m_leftBorderHovered)
                {
                    auto offset = sc->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset , m_draggedTimeDelta);
                    auto deltaX =  timeToX(timeDelta) + scroll;

                    targetRect = {cc->getX() + deltaX, getYForTrack(targetTrack),
                                  cc->getWidth() - deltaX, GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};

                }
                else if (m_rightBorderHovered)
                {
                    targetRect = {cc->getX(), getYForTrack(targetTrack),
                                  cc->getWidth() + timeToX(m_draggedTimeDelta) + scroll, GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};

                }

                g.setColour(white);
                g.drawRect(targetRect);

                if (EngineHelpers::trackWantsClip(sc, targetTrack))
                {
                    GUIHelpers::drawClip(g, targetRect, sc, targetTrack->getColour().withAlpha(0.1f), m_editViewState);
                }
                else
                {
                    g.setColour(grey);
                    g.fillRect(targetRect.reduced(1,1));
                    g.setColour(black);
                    g.drawFittedText("not allowed",targetRect,juce::Justification::centred, 1);
                }
            }
        }
    }
}

void SongEditorView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());

    for (auto trackView : m_trackViews)
    {
        auto trackHeight = 0;

        if (auto track = trackView->getTrack())
        {
            trackHeight = GUIHelpers::getTrackHeight(track, m_editViewState, true);
            if (auto ft = trackView->getTrack()->getParentFolderTrack())
                if (ft->state.getProperty(IDs::isTrackMinimized))
                    trackHeight = 0;
        }

        trackView->setBounds (0, y, getWidth(), trackHeight);

        y += trackHeight;
    }

    m_lassoComponent.setBounds(getLocalBounds());
}

void SongEditorView::mouseMove (const juce::MouseEvent &e)
{
    //init
    m_hoveredTrack = getTrackAt(e.y);
    m_hoveredClip = nullptr;
    m_hoveredAutamatableParam = getAutomatableParamAt(e.y);
    m_hoveredAutomationPoint = -1;
    m_hoveredCurve = -1;
    m_leftBorderHovered = false;
    m_rightBorderHovered = false;
    m_hoveredTimeRange = false;


    auto mousePosTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));

    if (m_hoveredAutamatableParam)
    {
        auto al = getAutomationLaneForAutomatableParameter(m_hoveredAutamatableParam);
        auto alEvent = e.getEventRelativeTo(al);
        auto curve = m_hoveredAutamatableParam->getCurve();

        
        if (m_selectedRange.selectedTracks.contains(al->getCurve().getOwnerParameter()->getTrack())
            && m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x)))
            && m_editViewState.m_syncAutomation)
        {
            m_hoveredTimeRange = true;
        }
        else
        {
            const auto hoveredRect =  al->getHoveredRect(alEvent);   
            for (auto i = 0; curve.getNumPoints() > i; i++)
            {
                auto pointXy = al->getPointXY(curve.getPointTime(i),
                                              curve.getPointValue(i));

                if (hoveredRect.contains(pointXy))
                {
                    m_hoveredAutomationPoint = i;
                }
            }

            auto valueAtMouseTime = m_hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
            auto curvePointAtMouseTime = juce::Point<int>(alEvent.x,al->getYPos(valueAtMouseTime));

            if (hoveredRect.contains(curvePointAtMouseTime) && m_hoveredAutomationPoint == -1)
            {
                m_hoveredCurve = curve.nextIndexAfter(mousePosTime);
                m_curveAtMousedown = al->getCurve().getPoint(m_hoveredCurve - 1).curve;
            }
            
            auto cp = al->getPointXY(mousePosTime, valueAtMouseTime);
            auto rect = al->getRectFromPoint(cp);

            al->setHover(m_hoveredAutomationPoint, m_hoveredCurve, rect.toNearestInt());
            al->repaint();
        }
    }

    if (m_hoveredTrack)
    {
        if (m_selectedRange.selectedTracks.contains(m_hoveredTrack) && m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x))))
            m_hoveredTimeRange = true;
    }

    if (auto at = dynamic_cast<te::AudioTrack*>(m_hoveredTrack.get()))
    {
        if (!m_hoveredTimeRange)
        {
            for (auto clip : at->getClips())
            {
                if (clip->getEditTimeRange().contains(mousePosTime))
                {
                    m_hoveredClip = clip;
                    if (auto cc = getClipViewForClip(clip))
                    {
                        if (e.getPosition().getX() < cc->getX() + 10 && cc->getWidth () > 30)
                        { 
                            m_leftBorderHovered = true;
                        }
                        else if (e.getPosition().getX() > cc->getRight() - 10
                             &&  cc->getWidth () > 30)

                        {
                            m_rightBorderHovered = true;

                        }
                    }
                }
            }
        }
    }

    if (m_hoveredTrack && m_hoveredTrack->isFolderTrack() && !m_hoveredTimeRange)
    {
    }

    if (m_hoveredClip != nullptr && !m_hoveredTimeRange)   
    {
        if (m_leftBorderHovered)
        {
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        }
        else if (m_rightBorderHovered)
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
    }
    else if (m_hoveredAutamatableParam && !m_hoveredTimeRange)
    {
        if (m_hoveredAutomationPoint != -1)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        else if (m_hoveredCurve != -1 && e.mods.isCtrlDown())
        {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        else if (m_hoveredCurve != -1)
        {
            setMouseCursor(juce::MouseCursor::NoCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
        }
    }

    else if (!m_hoveredTimeRange)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    else if (m_hoveredTimeRange)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
}
void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_draggedVerticalOffset = 0;
    m_isDraggingSelectedTimeRange = false;
    m_isSelectingTimeRange = false;
    m_draggedClipComponent = nullptr;
    m_draggedTimeDelta = 0.0;
    m_timeOfHoveredAutomationPoint = tracktion::TimePosition::fromSeconds(0.0);
    if (m_hoveredTimeRange)
    {
        auto scroll = timeToX(0) * (-1);
        auto selectedTracks = m_selectedRange.selectedTracks;
        auto x = timeToX(m_selectedRange.getStart().inSeconds());
        auto w = timeToX(m_selectedRange.getLength().inSeconds()) + scroll ;
        auto y = getYForTrack(selectedTracks.getFirst());
        auto h = getYForTrack(selectedTracks.getLast()) + GUIHelpers::getTrackHeight(selectedTracks.getLast(), m_editViewState, true) - y;
        m_timeRangeImage = createComponentSnapshot({x, y, w, h}, false, getDesktopScaleFactor());
    }
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
        else if (m_hoveredTimeRange)
        {
        }
        else 
        {
            if (m_hoveredClip)
            {
                if (e.getNumberOfClicks() > 1 || m_editViewState.m_isPianoRollVisible)
                {
                    m_lowerRange.showPianoRoll(m_hoveredClip->getTrack());
                }

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
                if (e.getNumberOfClicks() > 1)
                {
                    auto beat = xToSnapedBeat(e.x);

                    auto tc = getTrackViewForTrack(m_hoveredTrack);
                    if (tc->isMidiTrack())
                        tc->createNewMidiClip(beat);
                }
                else
                {
                    sm.deselectAll();
                    startLasso(e, false, false);
                }
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
    // m_draggedTimeDelta = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime;

    if (!sm.isSelected(m_hoveredClip))
    {
        sm.addToSelection(m_hoveredClip);
    }
    //move/copy selected timeRange
    if(m_hoveredTimeRange && !m_isSelectingTimeRange)
    {
        m_isDraggingSelectedTimeRange = true;

        auto startTime = getSnapedTime(xtoTime(e.getMouseDownX()), true);
        auto targetTime = getSnapedTime(xtoTime(e.x), false);
        m_draggedTimeDelta = targetTime - startTime; 
    }
    else if (m_hoveredClip)
    {
        m_draggedClipComponent = getClipViewForClip(m_hoveredClip);
        m_draggedVerticalOffset = getVerticalOffset(getTrackViewForTrack(m_hoveredTrack), {e.x, e.y});

        auto screenStartTime = xtoTime(0);
        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime; 

        auto startTime = m_draggedClipComponent->getClip()->getPosition().getStart().inSeconds();
        if (m_rightBorderHovered)
            startTime = m_draggedClipComponent->getClip()->getPosition().getEnd().inSeconds();

        auto targetTime = startTime + draggedTime;
        if (!e.mods.isShiftDown())
            targetTime = getSnapedTime(targetTime);

        m_draggedTimeDelta = targetTime - startTime;
    }

    //lasso or select TimeRange
    if (m_lassoComponent.isVisible () || m_isSelectingTimeRange)
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

            auto newValue = p->value -  al->getValue(al->getHeight() - e.getDistanceFromDragStartY());
            newValue = juce::jmax((float)newValue, p->param.getValueRange().getStart());
            newValue = juce::jmin((float)newValue, p->param.getValueRange().getEnd());
            newValue = p->param.valueRange.convertFrom0to1(newValue);
            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }
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
        }
    }

    repaint(); 
}
void SongEditorView::mouseUp(const juce::MouseEvent& e)
{
    auto &sm = m_editViewState.m_selectionManager;

    if (m_lassoComponent.isVisible () || m_isSelectingTimeRange)
    {
        auto start = m_lassoComponent.getLassoRect().m_timeRange.getStart();
        auto end = m_lassoComponent.getLassoRect().m_timeRange.getEnd();
        m_selectedRange.timeRange = {tracktion::TimePosition::fromSeconds(getSnapedTime(start.inSeconds(), true)),
                               tracktion::TimePosition::fromSeconds(getSnapedTime(end.inSeconds(), false))};
        stopLasso();
    }

    if (e.mods.isLeftButtonDown())
    {
        if (m_hoveredTimeRange && e.mouseWasDraggedSinceMouseDown() == false)
            clearSelectedTimeRange();

        if (m_isDraggingSelectedTimeRange)
        {
            moveSelectedTimeRanges(tracktion::TimeDuration::fromSeconds(m_draggedTimeDelta), e.mods.isCtrlDown());
            auto newStart = getSnapedTime(m_selectedRange.getStart().inSeconds() + m_draggedTimeDelta, true);
            setSelectedTimeRange(m_selectedRange.timeRange.movedToStartAt(tracktion::TimePosition::fromSeconds(newStart)));

        }
        else if (auto cc = getClipViewForClip(m_hoveredClip) && e.mouseWasDraggedSinceMouseDown())
        {
            auto verticalOffset = getVerticalOffset(getTrackViewForTrack(m_hoveredClip->getTrack()), e.position.toInt());
    
            if (m_leftBorderHovered || m_rightBorderHovered)
            {
                EngineHelpers::resizeSelectedClips(m_leftBorderHovered, m_draggedTimeDelta, m_editViewState);
            }
            else
            {
                EngineHelpers::moveSelectedClips(m_clipPosAtMouseDown, e.mods.isCtrlDown(), m_draggedTimeDelta, verticalOffset, m_editViewState);
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

    m_draggedClipComponent = nullptr;
    m_isDraggingSelectedTimeRange = false;
    repaint();
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
TrackComponent* SongEditorView::getTrackViewForTrack(tracktion_engine::Track::Ptr track)
{
    for (auto &tc : m_trackViews)
        if (tc->getTrack () == track)
            return tc;

    return nullptr;
}


AutomationLaneComponent *SongEditorView::getAutomationLaneForAutomatableParameter(te::AutomatableParameter::Ptr ap)
{
    auto tc = getTrackViewForTrack(ap->getTrack());
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

int SongEditorView::getYForTrack (te::Track* track)
{
    int scrollY = m_editViewState.m_viewY;
    for (auto t : tracktion::getAllTracks(m_editViewState.m_edit))
    {
        if (t == track)
            return scrollY;
        if (!t->isAutomationTrack() && !t->isArrangerTrack() && !t->isChordTrack() && !t->isMarkerTrack() && !t->isTempoTrack() && !t->isMasterTrack())
        {
            scrollY += GUIHelpers::getTrackHeight(t, m_editViewState);
        }
    }
    return getHeight();
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

void SongEditorView::clearTrackViews()
{
    m_trackViews.clear(true);
    resized();
}

void SongEditorView::startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange)
{
    m_lassoComponent.startLasso({e.x, e.y}, m_editViewState.m_viewY, selectRange);
    m_isSelectingTimeRange = selectRange;
    m_lassoStartsInAutomation = fromAutomation;
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
    if (m_lassoComponent.isVisible () || m_isSelectingTimeRange)
    {
        m_lassoComponent.updateLasso({e.x, e.y}, m_editViewState.m_viewY);
        if (m_isSelectingTimeRange)
        {
            updateRangeSelection();
        }
        else
        {
            if (m_lassoStartsInAutomation)
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
    m_lassoStartsInAutomation = false;
    m_isSelectingTimeRange = false;  
}

void SongEditorView::duplicateSelectedClips()
{
    if (m_selectedRange.selectedTracks.size() == 0)
    {
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto range = te::getTimeRangeForSelectedItems(selectedClips);
        auto delta = range.getLength().inSeconds();
        auto sourceTime = range.getStart().inSeconds();

        if (auto cc = getClipViewForClip(selectedClips.getFirst()))
            moveSelectedClips(sourceTime, true, delta, 0);
    }
    else 
    {
        moveSelectedTimeRanges(m_selectedRange.getLength(), true);
        setSelectedTimeRange({m_selectedRange.getStart() + m_selectedRange.getLength(), m_selectedRange.getLength()});
    }
}

void SongEditorView::moveSelectedClips(double sourceTime, bool copy,  double delta, int verticalOffset)
{
    EngineHelpers::moveSelectedClips(sourceTime, copy, delta, verticalOffset,m_editViewState); 
}

int SongEditorView::getVerticalOffset(TrackComponent* sourceTrackComp, const juce::Point<int>& dropPos)
{
    auto targetTrackComp = getTrackViewForTrack(getTrackAt(dropPos.getY()));

	if (targetTrackComp)
	{
		auto verticalOffset = m_trackViews.indexOf(targetTrackComp)
		                    - m_trackViews.indexOf(sourceTrackComp);
		return verticalOffset;
	}
	return 0;
}

void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge)
{
    EngineHelpers::resizeSelectedClips(fromLeftEdge, m_draggedTimeDelta, m_editViewState);
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

ClipComponent* SongEditorView::getClipViewForClip(const tracktion_engine::Clip::Ptr& clip)
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
   
    auto range = m_lassoComponent.getLassoRect().m_timeRange;
    auto verticalScroll = static_cast<int>(m_editViewState.m_viewY);

    for (auto tv: m_trackViews)
    {
        auto trackVRange = getVerticalRangeOfTrack(tv->getTrack(), false);
        juce::Range<int> lassoRangeY = m_lassoComponent.getLassoRect().m_verticalRange; 
        if (trackVRange.intersects (lassoRangeY))
            m_selectedRange.selectedTracks.add(tv->getTrack());

        verticalScroll += trackVRange.getLength();

        for (auto al : tv->getAutomationLanes())
            verticalScroll += al->getHeight();
    }

    setSelectedTimeRange(range);
}

void SongEditorView::clearSelectedTimeRange()
{
   m_selectedRange.timeRange = tracktion::TimeRange();
   m_selectedRange.selectedTracks.clear();
}

void SongEditorView::deleteSelectedTimeRange()
{
    for (auto t : m_selectedRange.selectedTracks)
    {
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
            ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

        if (m_editViewState.m_syncAutomation)
            if (auto tv = getTrackViewForTrack(t))
                for (auto al : tv->getAutomationLanes())
                    al->getCurve().removePointsInRegion(m_selectedRange.timeRange);
    }
}
void SongEditorView::setSelectedTimeRange(tracktion::TimeRange tr)
{
    auto start = tr.getStart();
    auto end = tr.getEnd();
    m_selectedRange.timeRange = {tracktion::TimePosition::fromSeconds(getSnapedTime(start.inSeconds(), true)),
                                tracktion::TimePosition::fromSeconds(getSnapedTime(end.inSeconds(), false))};
}

juce::Array<te::Track*> SongEditorView::getTracksWithSelectedTimeRange()
{
    return m_selectedRange.selectedTracks;
}

tracktion::TimeRange SongEditorView::getSelectedTimeRange()
{
    return m_selectedRange.timeRange;
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
    auto tv = getTrackViewForTrack(track);
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

void SongEditorView::moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy)
{
    for (auto t : m_selectedRange.selectedTracks)
        if (t!= nullptr)
            moveSelectedRangeOfTrack(t, td, copy);
}

void SongEditorView::moveSelectedRangeOfTrack(te::Track::Ptr track, tracktion::TimeDuration duration, bool copy)
{
    if (auto ct = dynamic_cast<te::ClipTrack*>(track.get()))
    {
        const auto editStart = tracktion::TimePosition::fromSeconds(0.0);
        const auto targetStart =  m_selectedRange.getStart() + duration;
        const auto targetEnd = m_selectedRange.getEnd() + duration;

        te::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();

        for (auto& c : ct->getClips())
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

        for (auto& clip : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        {
            constrainClipInRange (clip, {targetStart, targetEnd});
            m_editViewState.m_selectionManager.deselect(clip);
        }
    }

    //AUTOMATION
    if (m_editViewState.m_syncAutomation && m_selectedRange.selectedTracks.contains(track))
    {
        te::TrackAutomationSection as;
        as.src = track;
        as.dst = track;
        as.position = m_selectedRange.timeRange;

        if (auto tv = getTrackViewForTrack(track))
        {
            for (auto al : tv->getAutomationLanes())
            {
                te::TrackAutomationSection::ActiveParameters par;
                par.param = al->getCurve().getOwnerParameter();
                par.curve = al->getCurve();
                as.activeParameters.add(par);
            }
            
            te::moveAutomation(as, duration, copy);
        }
    }
}

void SongEditorView::constrainClipInRange(te::Clip* c, tracktion::TimeRange r)
{
    auto pos = c->getPosition();

    if (!r.intersects(c->getPosition().time))
    {
        c->removeFromParentTrack();
    }
    else 
    {
        if (pos.getStart() < r.getStart())
            c->setStart(r.getStart(), true, false);

        if (pos.getEnd() > r.getEnd())
            c->setEnd(r.getEnd(), true);
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

double SongEditorView::xToSnapedBeat (int x)
{
    auto time = xtoTime(x);
    time = getSnapedTime(time);
    return m_editViewState.timeToBeat(time);
}

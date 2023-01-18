#include "SongEditorView.h"
#include "EditViewState.h"
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

    m_editViewState.m_edit.getTransport().addChangeListener(this);
}

void SongEditorView::paint(juce::Graphics& g)
{
	auto area = getLocalBounds();
	g.setColour(juce::Colour(0xff303030));
	g.fillRect(area);

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
        {
            if (!ct->isMasterTrack() && !ct->isTempoTrack() && !ct->isAutomationTrack() && !ct->isArrangerTrack() && !ct->isMarkerTrack() && !ct->isChordTrack())
            {
                auto x = 0;
                auto y = getYForTrack(t);
                auto w = getWidth();
                auto h = GUIHelpers::getTrackHeight(t, m_editViewState, false);

                drawTrack(g, {x, y, w, h}, ct, m_editViewState.getSongEditorViewedTimeRange());
            }
        }

        for (auto& ap : t->getAllAutomatableParams())
        {
            if (ap->getCurve().getNumPoints() > 0)
            {
                auto rect = getAutomationRect(ap);
                    
                if (rect.getHeight() > 0)
                    drawAutomationLane(g, m_editViewState.getSongEditorViewedTimeRange(), rect, ap);
            }
        }
    }
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

    if (m_hoveredTimeRangeLeft)
    {
        auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + GUIHelpers::getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }

    if (m_hoveredTimeRangeRight)
    {
        auto x = timeToX(m_selectedRange.getEnd().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + GUIHelpers::getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }
    if (m_draggedClip)
    {
        for (auto sc : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(sc->getTrack(), m_draggedVerticalOffset))
            {
                auto clipRect = getClipRect(sc);
                juce::Rectangle<int> targetRect = {clipRect.getX() + timeToX(m_draggedTimeDelta) + scroll,
                                                       getYForTrack(targetTrack),
                                                       clipRect.getWidth(),
                                                        GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};
               
                if (m_leftBorderHovered)
                {
                    auto offset = sc->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset , m_draggedTimeDelta);
                    auto deltaX =  timeToX(timeDelta) + scroll;

                    targetRect = {clipRect.getX() + deltaX, getYForTrack(targetTrack),
                                  clipRect.getWidth() - deltaX, GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};

                }
                else if (m_rightBorderHovered)
                {
                    targetRect = {clipRect.getX(), getYForTrack(targetTrack),
                                  clipRect.getWidth() + timeToX(m_draggedTimeDelta) + scroll, GUIHelpers::getTrackHeight(targetTrack, m_editViewState, false)};

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
    m_lassoComponent.setBounds(getLocalBounds());
}


void SongEditorView::mouseMove (const juce::MouseEvent &e)
{
    //init
    m_hoveredTrack = getTrackAt(e.y);
    m_hoveredAutamatableParam = getAutomatableParamAt(e.y);
    m_isDragging = false;

    m_hoveredClip = nullptr;
    m_hoveredAutomationPoint = -1;
    m_hoveredCurve = -1;
    m_leftBorderHovered = false;
    m_rightBorderHovered = false;
    m_hoveredTimeRange = false;
    m_hoveredTimeRangeLeft = false;
    m_hoveredTimeRangeRight = false;
    //to do: implement tool buttons for choosing tool mode
    m_toolMode = Tool::pointer;

    auto mousePosTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));

    //Automation Lane hit tests 
    if (m_hoveredAutamatableParam)
    {
        auto cursorHitsSelectedRange = m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x)));

        if (m_selectedRange.selectedAutomations.contains(m_hoveredAutamatableParam) && cursorHitsSelectedRange)
        {
            m_hoveredTimeRange = true;

            int leftX = timeToX(m_selectedRange.getStart().inSeconds());
            int rightX = timeToX(m_selectedRange.getEnd().inSeconds());

            if (e.x < leftX + 5)
                m_hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                m_hoveredTimeRangeRight = true;

        }
        else
        {
            juce::Point<int> hoveredPointInLane = {e.x, e.y - getYForAutomatableParam(m_hoveredAutamatableParam)};
            const auto hoveredRectOnLane = GUIHelpers::getSensibleArea(hoveredPointInLane, 3) ;   
            auto curve = m_hoveredAutamatableParam->getCurve();

            for (auto i = 0; curve.getNumPoints() > i; i++)
            {
                auto time = curve.getPoint(i).time;
                auto value = curve.getPoint(i).value;

                auto pointXy = getPointOnAutomationRect(time, value, m_hoveredAutamatableParam);

                if (hoveredRectOnLane.contains(pointXy))
                    m_hoveredAutomationPoint = i;
            }

            auto valueAtMouseTime = m_hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
            auto curvePointAtMouseTime = juce::Point<int>(e.x, getYPos(valueAtMouseTime, m_hoveredAutamatableParam));

            if (hoveredRectOnLane.contains(curvePointAtMouseTime) && m_hoveredAutomationPoint == -1)
                m_hoveredCurve = curve.nextIndexAfter(mousePosTime);
            
            auto cp = getPointOnAutomationRect(mousePosTime, valueAtMouseTime, m_hoveredAutamatableParam);
            cp = cp.translated (0, getYForAutomatableParam(m_hoveredAutamatableParam));
            m_hoveredRectOnAutomation = GUIHelpers::getSensibleArea(cp, 8);
            repaint();
        }
    }
    //track hit test 
    if (m_hoveredTrack)
    {
        if (m_selectedRange.selectedTracks.contains(m_hoveredTrack) && m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x))))
        {
            int leftX = timeToX(m_selectedRange.getStart().inSeconds());
            int rightX = timeToX(m_selectedRange.getEnd().inSeconds());

            if (e.x < leftX + 5)
                m_hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                m_hoveredTimeRangeRight = true;

            m_hoveredTimeRange = true;
        }
    }
    //clip hit test 
    if (auto at = dynamic_cast<te::AudioTrack*>(m_hoveredTrack.get()))
    {
        if (!m_hoveredTimeRange && m_hoveredAutamatableParam == nullptr)
        {
            for (auto clip : at->getClips())
            {
                if (clip->getEditTimeRange().contains(mousePosTime))
                {
                    m_hoveredClip = clip;
                    auto clipRect = getClipRect(clip);
                    {
                        if (e.getPosition().getX() < clipRect.getX() + 10 && clipRect.getWidth () > 30)
                        { 
                            m_leftBorderHovered = true;
                        }
                        else if (e.getPosition().getX() > clipRect.getRight() - 10
                             &&  clipRect.getWidth () > 30)
                        {
                            m_rightBorderHovered = true;
                        }
                    }
                }
            }
        }
    }


    if (e.mods.isCtrlDown() && e.mods.isShiftDown())
        m_toolMode = Tool::knife;
    else if (e.mods.isAltDown())
        m_toolMode = Tool::range;

    updateCursor(e.mods);

    //log 
    // GUIHelpers::log("------------------------------------------------------------");
    // GUIHelpers::log("ToolMode    : ", (int) m_toolMode);
    // GUIHelpers::log("TimeRange   : ", m_hoveredTimeRange);
    // GUIHelpers::log("TimeRange L : ", m_hoveredTimeRangeLeft);
    // GUIHelpers::log("TimeRange R : ", m_hoveredTimeRangeRight);
    //  GUIHelpers::log("Track       : ", m_hoveredTrack != nullptr);
    // if (m_hoveredTrack)
    // {
    //     GUIHelpers::log("Track : ", m_hoveredTrack->getName());
    // }
    // GUIHelpers::log("Clip        : ", m_hoveredClip != nullptr);
    // GUIHelpers::log("Clip      L : ", m_leftBorderHovered);
    // GUIHelpers::log("Clip      R : ", m_rightBorderHovered);
    // if (m_hoveredAutamatableParam)
    // {
    //     GUIHelpers::log("Automation  : ", m_hoveredAutamatableParam != nullptr);
    //     GUIHelpers::log("Track: ", m_hoveredAutamatableParam->getTrack()->getName());
    //     std::cout << "Automation Y: " << getYForAutomatableParam(m_hoveredAutamatableParam) << " Height: " << getHeightOfAutomation(m_hoveredAutamatableParam) << std::endl;
    //     GUIHelpers::log("Point       : ", m_hoveredAutomationPoint) ;
    //     GUIHelpers::log("Curve       : ", m_hoveredCurve);
    // }
}

void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    auto &sm = m_editViewState.m_selectionManager;

    //init
    m_draggedVerticalOffset = 0;
    m_isDraggingSelectedTimeRange = false;
    m_isSelectingTimeRange = false;
    m_draggedClip = nullptr;
    m_draggedTimeDelta = 0.0;
    m_timeOfHoveredAutomationPoint = tracktion::TimePosition::fromSeconds(0.0);
    m_selPointsAtMousedown = getSelectedPoints();

    bool leftButton = e.mods.isLeftButtonDown();
    bool rightButton = e.mods.isRightButtonDown();

    bool clickedOnTrack = m_hoveredTrack != nullptr;
    bool clickedOnClip = m_hoveredClip != nullptr;
    bool clickedOnAutomationLane = m_hoveredAutamatableParam != nullptr;
    bool clickedOnPoint = m_hoveredAutomationPoint != -1;
    bool clickedOnCurve = m_hoveredCurve != -1;
    bool clickedOnTimeRange = m_hoveredTimeRange && m_toolMode == Tool::pointer;


    if (clickedOnTimeRange && leftButton)
    {
        // m_timeRangeImage = createSnapshotOfTimeRange();
        return;
    }

    if (clickedOnClip && leftButton && m_toolMode == Tool::pointer)
    {
        clearSelectedTimeRange();

        if (e.getNumberOfClicks() > 1 || m_editViewState.m_isPianoRollVisible)
            m_lowerRange.showPianoRoll(m_hoveredClip->getTrack());

        if (!sm.isSelected(m_hoveredClip))
            sm.selectOnly(m_hoveredClip);

        m_clipPosAtMouseDown = m_hoveredClip->getPosition().getStart().inSeconds();

        return;
    }

    if (clickedOnClip && leftButton && m_toolMode == Tool::knife)
    {
        splitClipAt(e.x, e.y);
        return;
    }

    if (clickedOnPoint && leftButton)
    {
        if (!isAutomationPointSelected(m_hoveredAutamatableParam, m_hoveredAutomationPoint))
            selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, false);

        m_timeOfHoveredAutomationPoint = m_hoveredAutamatableParam->getCurve().getPointTime(m_hoveredAutomationPoint);
        m_selPointsAtMousedown = getSelectedPoints();
        return;
    }

    if (clickedOnPoint && rightButton)
    {
        m_hoveredAutamatableParam->getCurve().removePoint(m_hoveredAutomationPoint);  
        sm.deselectAll();
        m_selPointsAtMousedown = getSelectedPoints();
        return;
    }

    if (clickedOnCurve && leftButton)
    {
        if (e.mods.isCtrlDown())
        {
            m_curveSteepAtMousedown = m_hoveredAutamatableParam->getCurve().getPointCurve(m_hoveredCurve -1);
        }
        else 
        {
            auto mouseTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
            addAutomationPointAt(m_hoveredAutamatableParam, mouseTime);
        }
            
        m_selPointsAtMousedown = getSelectedPoints();
        m_timeOfHoveredAutomationPoint = m_hoveredAutamatableParam->getCurve().getPointTime(m_hoveredAutomationPoint);

        return;
    }

    if (clickedOnCurve && rightButton)
    {
        m_hoveredAutamatableParam->getCurve().setCurveValue(m_hoveredCurve, .0f);
        return;
    }

    if (clickedOnTrack && leftButton && e.getNumberOfClicks() > 1)
    {
        auto beat = xToSnapedBeat(e.x);
        if ((bool) m_hoveredTrack->state.getProperty(IDs::isMidiTrack))
            createNewMidiClip(beat, m_hoveredTrack);

        return;
    }

    if ((clickedOnTrack || clickedOnAutomationLane) && leftButton)
    {
        startLasso(e, clickedOnAutomationLane, m_toolMode == Tool::range);
    }
}

void SongEditorView::mouseDrag(const juce::MouseEvent&e)
{
    auto &sm = m_editViewState.m_selectionManager;

    auto isDraggingTimeRange = m_hoveredTimeRange && !m_isSelectingTimeRange && !m_hoveredTimeRangeLeft && !m_hoveredTimeRangeRight;
    auto isDraggingClip = m_hoveredClip != nullptr;
    auto isDraggingMultiSelectionToolSpan = m_lassoComponent.isVisible () || m_isSelectingTimeRange;
    auto isDraggingAutomationPoint = m_hoveredAutomationPoint != -1 && e.mouseWasDraggedSinceMouseDown ();
    auto isChangingCurveSteepness = m_hoveredAutomationPoint == -1 && m_hoveredCurve != -1 && e.mods.isCtrlDown ();

    if (isDraggingTimeRange)
    {
        m_isDraggingSelectedTimeRange = true;

        auto startTime = getSnapedTime(xtoTime(e.getMouseDownX()), true);
        auto targetTime = getSnapedTime(xtoTime(e.x), false);
        m_draggedTimeDelta = targetTime - startTime; 
    }
        
    else if (m_hoveredTimeRangeLeft)
    {
        auto newStartTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
        setSelectedTimeRange({newStartTime, m_selectedRange.getEnd()}, true, false);
    }

    else if (m_hoveredTimeRangeRight)
    {
        auto newEndTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
        setSelectedTimeRange({m_selectedRange.getStart(), newEndTime}, false, false);
    }

    else if (isDraggingClip)
    {
        sm.addToSelection(m_hoveredClip);
        m_draggedClip = m_hoveredClip;
        m_draggedVerticalOffset = getVerticalOffset(m_hoveredTrack, {e.x, e.y});

        auto screenStartTime = xtoTime(0);
        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime; 

        auto startTime = m_draggedClip->getPosition().getStart().inSeconds();
        if (m_rightBorderHovered)
            startTime = m_draggedClip->getPosition().getEnd().inSeconds();

        auto targetTime = startTime + draggedTime;
        if (!e.mods.isShiftDown())
            targetTime = getSnapedTime(targetTime);

        m_draggedTimeDelta = targetTime - startTime;
    }

    if (isDraggingMultiSelectionToolSpan)
        updateLasso (e);

    if (isDraggingAutomationPoint)
    { 
        auto lockTime = e.mods.isShiftDown();
        auto snap = e.mods.isShiftDown();

        auto oldPos = m_timeOfHoveredAutomationPoint;
        auto newPos = tracktion::TimePosition::fromSeconds(xtoTime(e.x)); 

        if (lockTime)
            newPos = oldPos;
        else if (snap)
            newPos = tracktion::TimePosition::fromSeconds(getSnapedTime(xtoTime(e.x)));
        
        auto draggedTime = newPos - oldPos;

        for (auto p : m_selPointsAtMousedown)
        {
            auto newTime =  p->time + draggedTime;
            auto newValue = getValue(getYPos(p->value, p->param) + e.getDistanceFromDragStartY(), p->param);

            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }
    }

    if (isChangingCurveSteepness)
    {
        m_isDragging = true;

        auto valueP1 = m_hoveredAutamatableParam->getCurve().getPointValue(m_hoveredCurve - 1);
        auto valueP2 = m_hoveredAutamatableParam->getCurve().getPointValue(m_hoveredCurve);
        auto delta =  valueP1 < valueP2 ? e.getDistanceFromDragStartY() *  0.01 
                                : e.getDistanceFromDragStartY() * -0.01;
        auto newSteep = juce::jlimit(-0.5f, 0.5f, (float) (m_curveSteepAtMousedown + delta));

        m_hoveredAutamatableParam->getCurve().setCurveValue(m_hoveredCurve - 1, newSteep);
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
            setSelectedTimeRange(m_selectedRange.timeRange.movedToStartAt(tracktion::TimePosition::fromSeconds(newStart)),true, false);

        }
        else if (m_hoveredClip && e.mouseWasDraggedSinceMouseDown())
        {
            auto verticalOffset = getVerticalOffset(m_hoveredClip->getTrack(), e.position.toInt());
    
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
            clearSelectedTimeRange();
            sm.selectOnly(m_hoveredClip);
        }
        else if (m_hoveredClip && !e.mouseWasDraggedSinceMouseDown())
        {
            clearSelectedTimeRange();
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
                clearSelectedTimeRange();
                if (e.mods.isCtrlDown())
                {
                    if (isAutomationPointSelected(m_hoveredAutamatableParam, m_hoveredAutomationPoint))
                        deselectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint);
                    else
                        selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, true);
                }
                else
                {
                    selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, false);
                }
            }
        }
        else if (m_hoveredAutomationPoint == -1 && m_hoveredCurve == -1 && m_hoveredClip == nullptr && !e.mouseWasDraggedSinceMouseDown())
        {
            clearSelectedTimeRange();
            sm.deselectAll();
        }
    }

    m_draggedClip = nullptr;
    m_isDraggingSelectedTimeRange = false;
    m_hoveredTimeRangeRight = false;
    m_hoveredTimeRangeLeft = false;
    m_isDragging = false;
            
    mouseMove(e);
}

void SongEditorView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    for (auto t : te::getAudioTracks(m_editViewState.m_edit))
    {
        buildRecordingClips(t);
    }
}

te::Track::Ptr SongEditorView::getTrackAt(int y)
{
    for (auto t : getShowedTracks())
    {
        auto s = getYForTrack(t);
        auto e = s + GUIHelpers::getTrackHeight(t, m_editViewState);
        auto vRange = juce::Range<int> (s, e);
        if (vRange.contains(y))
            return t;
    }
    
    return nullptr;
}

int SongEditorView::getYForTrack (te::Track* track)
{
    int scrollY = m_editViewState.m_viewY;
    for (auto t : getShowedTracks())
    {
        if (t.get() == track)
            return scrollY;
        
        scrollY += GUIHelpers::getTrackHeight(t, m_editViewState);
    }

    return getHeight();
}

te::AutomatableParameter::Ptr SongEditorView::getAutomatableParamAt(int y)
{
    int scrollY = m_editViewState.m_viewY;

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (t->isAudioTrack() || t->isFolderTrack())
        {
            int trackHeight = GUIHelpers::getTrackHeight(t, m_editViewState, false);

            if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
            {
                for (auto p : t->getAllAutomatableParams())
                {
                    if (p->getCurve().getNumPoints() > 0)
                    {
                        int h = p->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));

                        if (juce::Range<int>(scrollY + trackHeight, scrollY + trackHeight + h).contains(y))
                        {
                            return p;
                        }
                    }
                }
            }

            scrollY += GUIHelpers::getTrackHeight(t,m_editViewState, true);
        }
    }

    return nullptr;
}

int SongEditorView::getYForAutomatableParam(te::AutomatableParameter::Ptr ap)
{
    double scrollY = m_editViewState.m_viewY;
    for (auto t : getShowedTracks())
    {
        auto trackHeight = GUIHelpers::getTrackHeight(t, m_editViewState, false); 
        scrollY += trackHeight;

        bool isMinimized = (bool) t->state.getProperty(IDs::isTrackMinimized);
        if (!isMinimized && trackHeight > 0)
        {
            for (auto p : t->getAllAutomatableParams())
            {
                if (p->getCurve().getNumPoints() > 0)
                {
                    if (p == ap.get())
                        return scrollY;

                    scrollY += getHeightOfAutomation(p);
                }   
            }
        }
    }

    jassert(false); 
    return -1;
}

juce::Rectangle<int> SongEditorView::getAutomationRect (te::AutomatableParameter::Ptr ap)
{
    auto x = getLocalBounds().getX();
    auto y = getYForAutomatableParam(ap);
    auto w = getWidth();
    auto h = getHeightOfAutomation(ap);
    return {x, y, w, h};
}

int SongEditorView::getHeightOfAutomation (te::AutomatableParameter::Ptr ap)
{
    if (GUIHelpers::getTrackHeight(ap->getTrack(), m_editViewState) == 0 || ap->getTrack()->state.getProperty(IDs::isTrackMinimized))
        return 0;

    return ap->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));
}

void SongEditorView::addAutomationPointAt(te::AutomatableParameter::Ptr par, tracktion::TimePosition pos)
{
    auto valueAtTime = m_hoveredAutamatableParam->getCurve().getValueAt(pos);
        
    auto p = par->getCurve().addPoint(pos, valueAtTime, 0.f);
    m_hoveredCurve = -1;
    m_hoveredAutomationPoint = p;
    selectAutomationPoint(par, p, false);
    m_isDragging = true;
}

void SongEditorView::selectAutomationPoint(te::AutomatableParameter::Ptr ap,int index, bool add)
{
    if (index >= 0 && index < ap->getCurve().getNumPoints())
        m_editViewState.m_selectionManager.select(createSelectablePoint (ap, index), add);
}

SelectableAutomationPoint* SongEditorView::createSelectablePoint(te::AutomatableParameter::Ptr ap, int index)
{
    auto apoint = new SelectableAutomationPoint(index, ap->getCurve());
    return apoint;
}

bool SongEditorView::isAutomationPointSelected(te::AutomatableParameter::Ptr ap, int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == ap->getCurve().getOwnerParameter() && p->index == index)
            return true;

    return false;
}

void SongEditorView::deselectAutomationPoint(te::AutomatableParameter::Ptr ap, int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == ap->getCurve().getOwnerParameter() && p->index == index)
            p->deselect();
}

juce::Array<SongEditorView::CurvePoint*> SongEditorView::getSelectedPoints()
{
    auto &sm = m_editViewState.m_selectionManager;
    juce::Array<CurvePoint*> points;
    for (auto p : sm.getItemsOfType<SelectableAutomationPoint>())
    {
        auto cp = new SongEditorView::CurvePoint(
                                 p->m_curve.getPointTime(p->index),
                                 p->m_curve.getPointValue(p->index),
                                 p->index,
                                *p->m_curve.getOwnerParameter());

        points.add(cp);
    }

    return points;
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
            updateRangeSelection();
        else
            if (m_lassoStartsInAutomation)
                updateAutomationSelection(e.mods.isShiftDown());
            else
                updateClipSelection(e.mods.isShiftDown());
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

        moveSelectedClips(sourceTime, true, delta, 0);
    }
    else 
    {
        moveSelectedTimeRanges(m_selectedRange.getLength(), true);
        setSelectedTimeRange({m_selectedRange.getStart() + m_selectedRange.getLength(), m_selectedRange.getLength()},false, false);
    }
}

void SongEditorView::updateCursor(juce::ModifierKeys modifierKeys)
{
    if (m_hoveredTrack && m_hoveredTrack->isFolderTrack() && !m_hoveredTimeRange)
    {
    }
    else if (m_hoveredClip != nullptr && !m_hoveredTimeRange && m_toolMode == Tool::pointer)   
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
    else if (m_hoveredClip != nullptr && m_toolMode == Tool::knife)
    {
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else if (m_toolMode == Tool::range)
    {
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else if (m_hoveredAutamatableParam && !m_hoveredTimeRange)
    {
        if (m_hoveredAutomationPoint != -1)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        else if (m_hoveredCurve != -1 && modifierKeys.isCtrlDown())
        {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            m_isDragging = true;
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
        if (m_hoveredTimeRangeLeft)
           setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        else if (m_hoveredTimeRangeRight)
           setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
}

void SongEditorView::moveSelectedClips(double sourceTime, bool copy,  double delta, int verticalOffset)
{
    EngineHelpers::moveSelectedClips(sourceTime, copy, delta, verticalOffset,m_editViewState); 
}

juce::Array<te::Track::Ptr> SongEditorView::getShowedTracks ()
{
    juce::Array<te::Track::Ptr> showedTracks;

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (EngineHelpers::isTrackShowable(t))
        {
            if (GUIHelpers::getTrackHeight(t, m_editViewState) > 0)
                showedTracks.add(t);
        }
    }

    return showedTracks;
}

int SongEditorView::getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int>& dropPos)
{

    auto targetTrack = getTrackAt(dropPos.getY());

	if (targetTrack)
	{
        auto showedTracks = getShowedTracks(); 
		return showedTracks.indexOf(targetTrack) - showedTracks.indexOf(sourceTrack);
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

tracktion_engine::MidiClip::Ptr SongEditorView::createNewMidiClip(double beatPos, te::Track::Ptr track)
{

    if (auto at = dynamic_cast<te::AudioTrack*>(track.get()))
    {

        auto start = tracktion::core::TimePosition::fromSeconds(
            juce::jmax(0.0, m_editViewState.beatToTime(beatPos)));
        auto end = tracktion::core::TimePosition::fromSeconds(
            juce::jmax(0.0, m_editViewState.beatToTime(beatPos))
            + m_editViewState.beatToTime(4));
        tracktion::core::TimeRange newPos(start, end);
        at->deleteRegion(newPos, &m_editViewState.m_selectionManager);

        auto mc = at->insertMIDIClip(newPos, &m_editViewState.m_selectionManager);
        mc->setName(at->getName());
        GUIHelpers::centerMidiEditorToClip(m_editViewState, mc);

        return mc;
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

    for (auto t: getShowedTracks())
    {
        auto trackVRange = getVerticalRangeOfTrack(t, false);
        juce::Range<int> lassoRangeY = m_lassoComponent.getLassoRect().m_verticalRange; 
        if (trackVRange.intersects (lassoRangeY))
            m_selectedRange.selectedTracks.add(t);

        for (auto ap : t->getAllAutomatableParams())
        {
            auto rect = getAutomationRect(ap);
            juce::Range<int> vRange = {rect.getY(), rect.getBottom()};
            if (vRange.intersects(lassoRangeY))
                m_selectedRange.selectedAutomations.add(ap);
        }
    }

    setSelectedTimeRange(range,true, false);
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
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
            ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

    for (auto a : m_selectedRange.selectedAutomations)
        a->getCurve().removePointsInRegion(m_selectedRange.timeRange);
}
void SongEditorView::setSelectedTimeRange(tracktion::TimeRange tr, bool snapDownAtStart, bool snapDownAtEnd)
{
    auto start = tr.getStart();
    auto end = tr.getEnd();
    m_selectedRange.timeRange = {tracktion::TimePosition::fromSeconds(getSnapedTime(start.inSeconds(), snapDownAtStart)),
                                tracktion::TimePosition::fromSeconds(getSnapedTime(end.inSeconds(), snapDownAtEnd))};
}

juce::Array<te::Track*> SongEditorView::getTracksWithSelectedTimeRange()
{
    return m_selectedRange.selectedTracks;
}

tracktion::TimeRange SongEditorView::getSelectedTimeRange()
{
    return m_selectedRange.timeRange;
}

void SongEditorView::splitClipAt(int x, int y)
{
    if (m_hoveredClip)
    {
        te::splitClips({m_hoveredClip}, tracktion::TimePosition::fromSeconds(xtoTime(x)));
    }
}

void SongEditorView::updateAutomationSelection(bool add)
{
    auto& sm = m_editViewState.m_selectionManager;
    sm.deselectAll();

    clearSelectedTimeRange();

    for (auto t : getShowedTracks())
    {
        auto trackHeight = GUIHelpers::getTrackHeight(t, m_editViewState, false);

        if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
        {
            for (auto ap : t->getAllAutomatableParams())
            {
                auto range = m_lassoComponent.getLassoRect().m_timeRange;
                auto firstPoint = ap->getCurve().nextIndexAfter(range.getStart());
                auto lastPoint = ap->getCurve().indexBefore(range.getEnd());
                for (auto i = firstPoint ; i <= lastPoint; i++)
                {
                    auto p = ap->getCurve().getPoint(i);
                    auto pointPos = getPointOnAutomationRect(p.time, p.value, ap);
                    pointPos.addXY(0, getYForAutomatableParam(ap));

                    if (m_lassoComponent.getLassoRect().m_verticalRange.contains(pointPos.getY()))
                       selectAutomationPoint(ap, i, true);
                }
            }
        }
    }

   if (add)
       for (auto p : m_cachedSelectedAutomation)
           m_editViewState.m_selectionManager.addToSelection(p);
}

void SongEditorView::updateAutomationCache()
{
    m_cachedSelectedAutomation.clear();
    m_cachedSelectedClips.clear();

    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        m_cachedSelectedAutomation.add(p);
}

void SongEditorView::updateClipSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    for (auto t : getShowedTracks())
    {
        juce::Range<int> lassoRangeY = 
            {(int) m_lassoComponent.getLassoRect ().m_verticalRange.getStart(),
             (int) m_lassoComponent.getLassoRect ().m_verticalRange.getEnd()};
        if (getVerticalRangeOfTrack(t, false).intersects (lassoRangeY) && !(t->isFolderTrack()))
            selectCatchedClips(t);
    }

    if (add)
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
}

juce::Range<int> SongEditorView::getVerticalRangeOfTrack(
     tracktion_engine::Track::Ptr track, bool withAutomation) 
{   
    auto trackY = getYForTrack(track);
    auto trackHeight = GUIHelpers::getTrackHeight(track, m_editViewState, withAutomation);

    return {trackY , trackY + trackHeight};
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

    for (auto a : m_selectedRange.selectedAutomations)
    {
        auto as = getTrackAutomationSection(a, m_selectedRange.timeRange);
        te::moveAutomation(as, td, copy);
    }
}

te::TrackAutomationSection SongEditorView::getTrackAutomationSection(te::AutomatableParameter* a, tracktion::TimeRange tr)
{
    te::TrackAutomationSection as;
    as.src = a->getTrack();
    as.dst = a->getTrack();
    as.position = tr;
    te::TrackAutomationSection::ActiveParameters par;
    par.param = a;
    par.curve = a->getCurve();
    as.activeParameters.add(par);

    return as;
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

std::unique_ptr<te::SmartThumbnail>& SongEditorView::getOrCreateThumbnail (te::WaveAudioClip::Ptr wac)
{

    for (auto tn : m_thumbnails)
        if (tn->waveAudioClip == wac)
            return tn->smartThumbnail;

    te::AudioFile af (wac->getAudioFile());
    std::unique_ptr<te::SmartThumbnail> thumbnail;

    if (af.getFile().existsAsFile() || (! wac->usesSourceFile()))
    {
        if (af.isValid())
        {
            const te::AudioFile proxy(
                        (wac->hasAnyTakes() && wac->isShowingTakes())
                        ? wac->getAudioFile()
                        : wac->getPlaybackFile());

            thumbnail = std::make_unique<te::SmartThumbnail>(
                        wac->edit.engine
                      , proxy
                      , *this
                      , &wac->edit);
        }
    }
    auto clipThumbnail = std::make_unique<ClipThumbNail> (wac, std::move(thumbnail));
    m_thumbnails.add(std::move(clipThumbnail));

    return m_thumbnails.getLast()->smartThumbnail;
}

void SongEditorView::drawTrack(juce::Graphics& g, juce::Rectangle<int> displayedRect, te::ClipTrack::Ptr clipTrack, tracktion::TimeRange etr)
{
    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(displayedRect.getX(),displayedRect.getBottom(), displayedRect.getRight(), displayedRect.getBottom());

    double x2beats = m_editViewState.timeToBeat(xtoTime(displayedRect.getRight()));
    double x1beats = m_editViewState.timeToBeat(xtoTime(displayedRect.getX()));

    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, x1beats, x2beats, displayedRect);

    for (auto clipIdx = 0; clipIdx < clipTrack->getNumTrackItems(); clipIdx++)
    {
        auto clip = clipTrack->getTrackItem(clipIdx);

        if (clip->getPosition().time.intersects(etr))
        {
            int x = timeToX(clip->getPosition().getStart().inSeconds());
            int y = displayedRect.getY();
            int w = timeToX(clip->getPosition().getEnd().inSeconds()) - x;
            int h = displayedRect.getHeight();

            juce::Rectangle<int> clipRect = {x,y,w,h};

            auto color = clip->getTrack()->getColour();
            if (auto c = dynamic_cast<te::Clip*>(clip))
                drawClip(g,clipRect, c, color, displayedRect); 
        }
    }


}

juce::Rectangle<int> SongEditorView::getClipRect (te::Clip::Ptr clip)
{
    int x = timeToX(clip->getPosition().getStart().inSeconds());
    int y = getYForTrack(clip->getClipTrack());
    int w = timeToX(clip->getPosition().getEnd().inSeconds()) - x;
    int h = GUIHelpers::getTrackHeight(clip->getClipTrack(), m_editViewState, false);

    juce::Rectangle<int> clipRect = {x,y,w,h};

    return clipRect;
}

void SongEditorView::drawClip(juce::Graphics& g, juce::Rectangle<int> clipRect, te::Clip * clip, juce::Colour color, juce::Rectangle<int> displayedRect)
{
    auto area = clipRect;
    if (clipRect.getX() < displayedRect.getX())
        area.removeFromLeft(displayedRect.getX() - clipRect.getX());
    if (clipRect.getRight() > displayedRect.getRight())
        area.removeFromRight(clipRect.getRight() - displayedRect.getRight());

    auto& evs = m_editViewState;
    auto header = area.withHeight(evs.m_clipHeaderHeight);
    auto isSelected = evs.m_selectionManager.isSelected (clip);

    auto clipColor = color;
    auto innerGlow = clipColor.brighter(0.5f);
    auto borderColour = clipColor.darker(0.95f);
    auto backgroundColor = borderColour.withAlpha(0.6f);

    // area.removeFromBottom(1);
    g.setColour(backgroundColor);
    g.fillRect(area.reduced(1, 1));

    g.setColour(innerGlow);
    g.drawRect(header);
    g.drawRect(area);
    g.setColour(clipColor);
    if (isSelected)
        g.setColour(clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f));

    g.fillRect(header.reduced(2,2));

    clipRect.removeFromTop(header.getHeight());
    clipRect.reduce(1,1);
    if (auto wac = dynamic_cast<te::WaveAudioClip*>(clip))
    {
        if (auto& thumb = getOrCreateThumbnail(wac))
            drawWaveform(g, *wac, *thumb, color, clipRect , displayedRect);
    }
    else if (auto mc = dynamic_cast<te::MidiClip*>(clip))
    {
        drawMidiClip(g, mc, clipRect, displayedRect, color);
    }
}

void SongEditorView::drawWaveform(juce::Graphics& g,
                                      te::AudioClipBase& c,
                                      te::SmartThumbnail& thumb,
                                      juce::Colour colour,
                                      juce::Rectangle<int> clipRect,
                                      juce::Rectangle<int> displayedRect)
{
    auto getTimeRangeForDrawing = [this] (const te::AudioClipBase& clip, const juce::Rectangle<int> clRect, const juce::Rectangle<int> displRect)
        -> tracktion::core::TimeRange
    {
        auto t1 = EngineHelpers::getTimePos(0.0);
        auto t2 = t1 + clip.getPosition().getLength();

        if (clRect.getX() < displRect.getX())
            t1 = t1 + tracktion::TimeDuration::fromSeconds(xtoTime(displRect.getX()) - xtoTime(clRect.getX()));

        if (clRect.getRight() > displRect.getRight())
            t2 = t2 - tracktion::TimeDuration::fromSeconds(xtoTime(clRect.getRight()) - xtoTime(displRect.getRight()));

        return { t1, t2 };
    };

    auto area = clipRect;
    if (clipRect.getX() < displayedRect.getX())
        area.removeFromLeft(displayedRect.getX() - clipRect.getX());
    if (clipRect.getRight() > displayedRect.getRight())
        area.removeFromRight(clipRect.getRight() - displayedRect.getRight());

    const auto gain = c.getGain();
    const auto pan = thumb.getNumChannels() == 1 ? 0.0f : c.getPan();

    const float pv = pan * gain;
    const float gainL = (gain - pv);
    const float gainR = (gain + pv);

    const bool usesTimeStretchedProxy = c.usesTimeStretchedProxy();

    const auto clipPos = c.getPosition();
    auto offset = clipPos.getOffset();
    auto speedRatio = c.getSpeedRatio();

    g.setColour (colour);

    bool showBothChannels = displayedRect.getHeight() > 100;

    if (usesTimeStretchedProxy)
    {

        if (!thumb.isOutOfDate())
        {
            drawChannels(g
                       , thumb
                       ,  area
                       , false
                       , getTimeRangeForDrawing(c, clipRect, displayedRect)
                       , c.isLeftChannelActive() && showBothChannels
                       , c.isRightChannelActive()
                       , gainL
                       , gainR);
        }
    }
    else if (c.getLoopLength().inSeconds() == 0)
    {
        auto region = getTimeRangeForDrawing (c, clipRect, displayedRect);

        auto t1 = EngineHelpers::getTimePos((region.getStart().inSeconds() + offset.inSeconds()) * speedRatio);
        auto t2 = EngineHelpers::getTimePos((region.getEnd().inSeconds()   + offset.inSeconds()) * speedRatio);
        bool useHighres = true;
        drawChannels(g
                   , thumb
                   , area
                   , useHighres
                   , {t1, t2}
                   , c.isLeftChannelActive()
                   , c.isRightChannelActive() && showBothChannels
                   , gainL
                   , gainR);
    }
}

void SongEditorView::drawChannels(juce::Graphics& g
                                    , te::SmartThumbnail& thumb
                                    , juce::Rectangle<int> area
                                    , bool useHighRes
                                    , tracktion::core::TimeRange time
                                    , bool useLeft
                                    , bool useRight
                                    , float leftGain
                                    , float rightGain)
{
    if (useLeft && useRight && thumb.getNumChannels() > 1)
    {
        thumb.drawChannel(g
                        , area.removeFromTop(area.getHeight() / 2)
                        , useHighRes
                        , time
                        , 0
                        , leftGain);
        thumb.drawChannel(g, area, useHighRes, time, 1, rightGain);
    }
    else if (useLeft)
        thumb.drawChannel (g, area, useHighRes, time, 0, leftGain);
    else if (useRight)
        thumb.drawChannel (g, area, useHighRes, time, 1, rightGain);
}


void  SongEditorView::drawMidiClip (juce::Graphics& g,te::MidiClip::Ptr clip, juce::Rectangle<int> clipRect, juce::Rectangle<int> displayedRect, juce::Colour color)
{
    auto area = clipRect;

    if (clipRect.getX() < displayedRect.getX())
        area.removeFromLeft(displayedRect.getX() - clipRect.getX());

    if (clipRect.getRight() > displayedRect.getRight())
        area.removeFromRight(clipRect.getRight() - displayedRect.getRight());

    auto& seq = clip->getSequence();
    auto range = seq.getNoteNumberRange();
    auto lines = range.getLength();
    auto noteHeight = juce::jmax(1,((clipRect.getHeight() ) / 20));
    auto noteColor = color.withLightness(0.6f);

    for (auto n: seq.getNotes())
    {
        double sBeat = n->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats();
        double eBeat = n->getEndBeat().inBeats() - clip->getOffsetInBeats().inBeats();
        auto y = clipRect.getCentreY();
        if (!range.isEmpty())
            y = juce::jmap(n->getNoteNumber(), range.getStart() + lines, range.getStart(), clipRect.getY() + (noteHeight/2), clipRect.getY() + clipRect.getHeight() - noteHeight - (noteHeight/2));

        int x1 = clipRect.getX() + timeToX(m_editViewState.beatToTime(sBeat )) +(timeToX(0.0) * -1);
        int x2 = clipRect.getX() + timeToX(m_editViewState.beatToTime(eBeat )) +(timeToX(0.0) * -1);

        int gap = 2;
        x1 = juce::jmin(juce::jmax(gap, x1), clipRect.getRight() - gap);
        x2 = juce::jmin(juce::jmax(gap, x2), clipRect.getRight () - gap);

        x1 = juce::jmax(area.getX(), x1);
        x2 = juce::jmin(area.getRight(), x2);
        auto w = juce::jmax(0, x2 - x1);
        g.setColour(noteColor);
        g.fillRect(x1, y, w, noteHeight);
    }
}
juce::Point<int> SongEditorView::getPointOnAutomationRect (tracktion::TimePosition t, double v, te::AutomatableParameter::Ptr ap) 
{
   return {timeToX(t.inSeconds()), getYPos(v, ap)}; 
}

int SongEditorView::getYPos (double value, te::AutomatableParameter::Ptr ap)
{
    double height = getAutomationRect(ap).getHeight();
    double pwh = getAutomationPointWidth(ap);

    double v = ap->valueRange.convertTo0to1(value);
    return height - juce::jmap(v, 0.0, 1.0, pwh, height - pwh);
}

double SongEditorView::getValue (int y, te::AutomatableParameter::Ptr ap)
{
    double pwh = (double) getAutomationPointWidth(ap) / 2;
    double lh = getAutomationRect(ap).getHeight();

    return 1.0 - juce::jmap((double)y, pwh, lh - pwh, 0.0, 1.0);
}

int SongEditorView::getAutomationPointWidth (te::AutomatableParameter::Ptr ap)
{
    if (getAutomationRect(ap).getHeight() <= 50)
        return 4;

    return 8;
}

void SongEditorView::drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<int> drawRect, te::AutomatableParameter::Ptr ap)
{
    using namespace juce::Colours;

    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, m_editViewState.m_viewX1, m_editViewState.m_viewX2, drawRect);

    juce::Path pointsPath;
    juce::Path selectedPointsPath;
    juce::Path hoveredPointPath;
    juce::Path curvePath;
    juce::Path hoveredCurvePath;
    juce::Path hoveredDotOnCurvePath;
    juce::Path fillPath;

    float startX = timeToX(drawRange.getStart().inSeconds());
    float startY = getYPos(ap->getCurve().getValueAt(drawRange.getStart()),ap) + drawRect.getY();
    curvePath.startNewSubPath({startX, startY});
    juce::Point<int> oldPoint = {(int)startX, (int)startY };
    for (auto i = 0; i < ap->getCurve().getNumPoints(); i++)
    {
        if (ap->getCurve().getPoint(i).time >= drawRange.getStart()
         && ap->getCurve().getPoint(i).time <= drawRange.getEnd())
        {
            auto time = ap->getCurve().getPoint(i).time;
            auto value = ap->getCurve().getPoint(i).value;
            juce::Point<int> pointPos = getPointOnAutomationRect(time, value, ap).translated(0, drawRect.getY());

            auto curve = juce::jlimit(-.5f, .5f, ap->getCurve().getPoint(i - 1).curve);
            auto x = oldPoint.x + ( (pointPos.x - oldPoint.x) * (.5f + curve) );
            auto y = oldPoint.y + ( (pointPos.y - oldPoint.y) * (.5f - curve) );
            auto curveControlPoint = juce::Point<float>(x, y);

            curvePath.quadraticTo(curveControlPoint, {pointPos.toFloat().x, pointPos.toFloat().y});
            auto ellipseRect = GUIHelpers::getSensibleArea(pointPos, getAutomationPointWidth(ap)).toFloat();
 
            if (m_hoveredCurve == i)
            {
                hoveredCurvePath.startNewSubPath (oldPoint.toFloat());
                hoveredCurvePath.quadraticTo (curveControlPoint , pointPos.toFloat());
                if (!m_isDragging) 
                    hoveredDotOnCurvePath.addEllipse (m_hoveredRectOnAutomation.toFloat());
            }

            oldPoint = pointPos;

            if (m_hoveredAutomationPoint == i)
            {
                hoveredPointPath.addEllipse(ellipseRect);
            }

            if (isAutomationPointSelected(ap, i))
            {
                selectedPointsPath.addEllipse(ellipseRect);
            }

            pointsPath.addEllipse(ellipseRect);
        }
    }

    float endX = timeToX(drawRange.getEnd().inSeconds());
    float endY = getYPos(ap->getCurve().getValueAt(drawRange.getEnd()),ap) + drawRect.getY();

    curvePath.lineTo(endX, endY);
    fillPath = curvePath;

    fillPath.lineTo(drawRect.getBottomRight().toFloat());
    fillPath.lineTo(drawRect.getBottomLeft().toFloat());
    fillPath.closeSubPath();

    if (getHeight () <= 50)
        g.setColour(juce::Colour(0x10ffffff));
    else
        g.setColour (ap->getTrack ()->getColour ().withAlpha (0.2f));
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xff888888));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white);
    if (m_isDragging)
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());
    g.strokePath(hoveredCurvePath, juce::PathStrokeType(2.0f));

    g.fillPath (hoveredDotOnCurvePath);

    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillPath(pointsPath);

    float lineThickness = 2.0;   

    g.setColour(juce::Colour(0xff888888));
    g.strokePath(pointsPath, juce::PathStrokeType(lineThickness));

    g.setColour(juce::Colours::white);
    g.strokePath(hoveredPointPath, juce::PathStrokeType(lineThickness));

    g.setColour(juce::Colours::red);
    g.strokePath(selectedPointsPath, juce::PathStrokeType(lineThickness));
}

void SongEditorView::buildRecordingClips(te::Track::Ptr track)
{
    bool needed = false;

    if (track->edit.getTransport().isRecording())
    {
        for (auto in: track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive()
                && track == *(in->getTargetTracks().getFirst()))
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

        auto recordingClip = std::make_unique<RecordingClipComponent>(track, m_editViewState);
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

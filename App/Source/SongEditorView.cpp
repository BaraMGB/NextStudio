#include "SongEditorView.h"
#include "EditViewState.h"
#include "Utilities.h"
#include <ostream>


SongEditorView::SongEditorView(EditViewState& evs, LowerRangeComponent& lr)
        : m_editViewState(evs)
        , m_lowerRange(lr)
        , m_lassoComponent(evs, evs.m_viewX1, evs.m_viewX2)
{		
    setWantsKeyboardFocus (true);
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
    clearSelectedTimeRange();

    m_editViewState.m_edit.getTransport().addChangeListener(this);
}

SongEditorView::~SongEditorView()
{
    m_editViewState.m_edit.getTransport().removeChangeListener(this);
}

void SongEditorView::paint(juce::Graphics& g)
{
    using namespace juce::Colours;
    auto &sm = m_editViewState.m_selectionManager;
    auto scroll = timeToX(0) * (-1);

	auto area = getLocalBounds();
	g.setColour(juce::Colour(0xff303030));
	g.fillRect(area);

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
        {
            if (EngineHelpers::isTrackShowable(t))
            {
                auto x = 0;
                auto y = getYForTrack(t);
                auto w = getWidth();
                auto h = getTrackHeight(t, m_editViewState, false);

                drawTrack(g, {x, y, w, h}, ct, m_editViewState.getSongEditorVisibleTimeRange());

                if (m_selectedRange.selectedTracks.contains(t) && m_selectedRange.getLength().inSeconds() > 0)
                {
                    auto x = timeToX(m_selectedRange.getStart().inSeconds());
                    auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;

                    g.setColour(juce::Colour(0x50ffffff));
                    g.fillRect(juce::Rectangle<int>{x, y, w, h});
                }
            }
        }

        for (auto& ap : t->getAllAutomatableParams())
        {
            if (GUIHelpers::isAutomationVisible(*ap))
            {
                auto rect = getAutomationRect(ap);
                    
                if (rect.getHeight() > 0)
                {
                    drawAutomationLane(g, m_editViewState.getSongEditorVisibleTimeRange(), rect, ap);

                    if (m_selectedRange.selectedAutomations.contains(ap) && m_selectedRange.getLength().inSeconds() > 0)
                    {
                        auto x = timeToX(m_selectedRange.getStart().inSeconds());
                        auto y = rect.getY();
                        auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
                        auto h = rect.getHeight();

                        g.setColour(juce::Colour(0x50ffffff));
                        g.fillRect(juce::Rectangle<int>{x, y, w, h});
                    }
                }
            }
        }
    }

    if (m_isDraggingSelectedTimeRange)
    {
        auto selectedRangeRect = juce::Rectangle<int>();

        for (auto track : m_selectedRange.selectedTracks)
        {
            auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
            auto y = getYForTrack(track);
            auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
            auto h = getTrackHeight(track, m_editViewState, false);
                
            x = x + timeToX(m_draggedTimeDelta) + scroll;

            juce::Rectangle<int> rect = {x, y, w, h};

            selectedRangeRect = selectedRangeRect.getUnion(rect);
            if (auto ct = dynamic_cast<te::ClipTrack*>(track))
                drawTrack(g, rect, ct, m_selectedRange.timeRange, true);
        }

        for (auto automation : m_selectedRange.selectedAutomations)
        {
            auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
            auto y = getAutomationRect(automation).getY();
            auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
            auto h = getHeightOfAutomation(automation);

            x = x + timeToX(m_draggedTimeDelta) + scroll;

            juce::Rectangle<int> rect = {x, y, w, h};

            drawAutomationLane(g, m_selectedRange.timeRange, rect, automation, true);

            selectedRangeRect = selectedRangeRect.getUnion(rect);
        }
        g.setColour(juce::Colours::yellowgreen);
        g.drawRect(selectedRangeRect);
    }

    if (m_hoveredTimeRangeLeft)
    {
        auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }

    if (m_hoveredTimeRangeRight)
    {
        auto x = timeToX(m_selectedRange.getEnd().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }
    if (m_draggedClip)
    {
        for (auto selectedClip : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(selectedClip->getTrack(), m_draggedVerticalOffset))
            {
                auto clipRect = getClipRect(selectedClip);
                juce::Rectangle<int> targetRect = {clipRect.getX() + timeToX(m_draggedTimeDelta) + scroll,
                                                       getYForTrack(targetTrack),
                                                       clipRect.getWidth(),
                                                        getTrackHeight(targetTrack, m_editViewState, false)};
               
                if (m_leftBorderHovered)
                {
                    auto offset = selectedClip->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset , m_draggedTimeDelta);
                    auto deltaX =  timeToX(timeDelta) + scroll;

                    targetRect = {clipRect.getX() + deltaX, getYForTrack(targetTrack),
                                  clipRect.getWidth() - deltaX, getTrackHeight(targetTrack, m_editViewState, false)};

                }
                else if (m_rightBorderHovered)
                {
                    targetRect = {clipRect.getX(), getYForTrack(targetTrack),
                                  clipRect.getWidth() + timeToX(m_draggedTimeDelta) + scroll, getTrackHeight(targetTrack, m_editViewState, false)};

                }

                g.setColour(white);
                g.drawRect(targetRect);

                if (EngineHelpers::trackWantsClip(selectedClip, targetTrack))
                {
                    juce::Rectangle<int> trackRect = {0, getYForTrack(targetTrack), getWidth(), getTrackHeight(targetTrack,m_editViewState, false)};
                    drawClip(g, targetRect, selectedClip, targetTrack->getColour().withAlpha(0.1f), trackRect, m_editViewState.m_viewX1, m_editViewState.m_viewX2);
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
    if (m_dragItemRect.visible)
    {
        if (m_dragItemRect.valid)
        {
            drawClipBody(g, m_dragItemRect.name, m_dragItemRect.drawRect, false, m_dragItemRect.colour, m_dragItemRect.drawRect, m_editViewState.m_viewX1, m_editViewState.m_viewX2);
        }
        else
        {
            g.setColour(grey);
            g.fillRect(m_dragItemRect.drawRect.reduced(1,1));
            g.setColour(black);
            g.drawFittedText("not allowed",m_dragItemRect.drawRect,juce::Justification::centred, 1);
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

            repaint();

        }
        else
        {
            juce::Point<int> hoveredPointInLane = {e.x, e.y - getYForAutomatableParam(m_hoveredAutamatableParam)};
            const auto hoveredRectOnLane = GUIHelpers::getSensibleArea(hoveredPointInLane, getAutomationPointWidth(m_hoveredAutamatableParam) * 2) ;   
            auto curve = m_hoveredAutamatableParam->getCurve();

            for (auto i = 0; curve.getNumPoints() > i; i++)
            {
                auto time = curve.getPoint(i).time;
                auto value = curve.getPoint(i).value;

                auto pointXy = getPointOnAutomationRect(time, value, m_hoveredAutamatableParam, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);

                if (hoveredRectOnLane.contains(pointXy.toInt()))
                    m_hoveredAutomationPoint = i;
            }

            auto valueAtMouseTime = m_hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
            auto curvePointAtMouseTime = juce::Point<int>(e.x, getYPos(valueAtMouseTime, m_hoveredAutamatableParam));

            if (hoveredRectOnLane.contains(curvePointAtMouseTime) && m_hoveredAutomationPoint == -1)
                m_hoveredCurve = curve.nextIndexAfter(mousePosTime);
            
            juce::Point<int> cp = getPointOnAutomationRect(mousePosTime, valueAtMouseTime, m_hoveredAutamatableParam, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2).toInt();
            cp = cp.translated (0, getYForAutomatableParam(m_hoveredAutamatableParam));
            m_hoveredRectOnAutomation = GUIHelpers::getSensibleArea(cp, 8);
            repaint();
        }
    }
    if (m_hoveredTrack && m_hoveredAutamatableParam == nullptr)
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

                        auto borderWidth = clipRect.getWidth() > 30 ? 10 : clipRect.getWidth() / 3;

                        if (e.getPosition().getX() < clipRect.getX() + borderWidth)
                        { 
                            m_leftBorderHovered = true;
                        }
                        else if (e.getPosition().getX() > clipRect.getRight() - borderWidth)
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

    //logMousePositionInfo();
}

void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_editViewState.m_isSavingLocked = true;

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

        repaint();

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
    auto snap = !e.mods.isShiftDown();

    if (isDraggingTimeRange)
    {
        m_isDraggingSelectedTimeRange = true;

        auto startTime = xtoTime(e.getMouseDownX());
        auto targetTime = xtoTime(e.x);

        if (snap)
        {
            startTime = getSnapedTime(startTime, true);
            targetTime = getSnapedTime(targetTime, false);
        }

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
        auto lockTime = e.mods.isCtrlDown();

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
    
            if (e.mods.isCommandDown() && m_rightBorderHovered)
            {
                for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
                {
                    if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
                    {
                        setNewTempoOfClipByNewLength(wac, c->getPosition().getLength().inSeconds() + m_draggedTimeDelta);
                        removeThumbnail(wac);
                    }
                }
            }
            else if (m_leftBorderHovered || m_rightBorderHovered)
            {
                EngineHelpers::resizeSelectedClips(m_leftBorderHovered, m_draggedTimeDelta, m_editViewState);
            }
            else
            {
                EngineHelpers::moveSelectedClips(e.mods.isCtrlDown(), m_draggedTimeDelta, verticalOffset, m_editViewState);
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
    m_editViewState.m_isSavingLocked = false;
            
    mouseMove(e);
}

void SongEditorView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    for (auto t : te::getAudioTracks(m_editViewState.m_edit))
    {
        buildRecordingClips(t);
    }
}

void SongEditorView::getAllCommands (juce::Array<juce::CommandID>& commands) 
{
    juce::Array<juce::CommandID> ids {

            KeyPressCommandIDs::deleteSelectedClips,
            KeyPressCommandIDs::duplicateSelectedClips,
            KeyPressCommandIDs::selectAllClips,
            KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack,
            KeyPressCommandIDs::transposeClipUp,
            KeyPressCommandIDs::transposeClipDown,
            KeyPressCommandIDs::reverseClip
        };

    commands.addArray(ids);
}
void SongEditorView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) 
{
    switch (commandID)
    { 
        case KeyPressCommandIDs::duplicateSelectedClips :
            result.setInfo("Duplicate selected clips", "Duplicate selected clips", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::deleteSelectedClips :
            result.setInfo("Delete selected clips", "Delete selected clips", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey , 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::selectAllClips :
            result.setInfo("select all Clips","select all Clips", "Song Editor", 0);

            result.addDefaultKeypress(juce::KeyPress::createFromDescription("a").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;

        case KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack :
            result.setInfo("render time range to new track","render time range on new track", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("r").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::transposeClipUp :
            result.setInfo("transpose up", "transpose selected clips up 1 key", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::transposeClipDown :
            result.setInfo("transpose Down", "transpose selected clips Down 1 key", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
            break;  
        case KeyPressCommandIDs::reverseClip :
            result.setInfo("Reverse wave of clip","Reverse wave of clip", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("b").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        default:
            break;
        }

}
bool SongEditorView::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{
    GUIHelpers::log("SongEditorView perform commandID: ", info.commandID);

    switch (info.commandID)
    { 
        //send NoteOn
        case KeyPressCommandIDs::deleteSelectedClips:
        {
            GUIHelpers::log("deleteSelectedClips Outer");
            if (getTracksWithSelectedTimeRange().size() > 0)
            {
                GUIHelpers::log("deleteSelectedTimeRange");
                deleteSelectedTimeRange();
            }
            else 
            {
                GUIHelpers::log("deleteSelectedClips");
                EngineHelpers::deleteSelectedClips (m_editViewState);
            }
            break;
        }
        case KeyPressCommandIDs::duplicateSelectedClips:
            duplicateSelectedClipsOrTimeRange();
            break;
        case KeyPressCommandIDs::selectAllClips:
            EngineHelpers::selectAllClips(m_editViewState.m_selectionManager, m_editViewState.m_edit);
            break;
        case KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack:
            renderSelectedTimeRangeToNewTrack();
            break;
        case KeyPressCommandIDs::transposeClipUp:
            GUIHelpers::log("perform: transposeClipUp");
            transposeSelectedClips( + 1.f);
            break;
        case KeyPressCommandIDs::transposeClipDown:
            transposeSelectedClips( - 1.f);
            break;
        case KeyPressCommandIDs::reverseClip:
            reverseSelectedClips();
            break;
        default:
            return false;
    }
    return true;
}

bool SongEditorView::isInterestedInDragSource (const SourceDetails& dragSourceDetails) 
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
        return true;
    }

    return false;
}

void SongEditorView::itemDragEnter (const SourceDetails& dragSourceDetails)
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
    }
}

void SongEditorView::itemDragMove (const SourceDetails& dragSourceDetails) 
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {

        auto pos = dragSourceDetails.localPosition;
        bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
        auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnapedTime(xtoTime(pos.x)); 
        auto f = fileTreeComp->getSelectedFile();

        m_dragItemRect.valid = false;
        if (auto targetTrack = getTrackAt(pos.y))
        {

            te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
            auto x = timeToX(dropTime);
            auto y = getYForTrack(targetTrack);
            auto w = timeToX(audioFile.getLength());
            auto h = getTrackHeight(targetTrack, m_editViewState, false);
            m_dragItemRect.drawRect = {x, y, w, h};
            m_dragItemRect.name = f.getFileNameWithoutExtension();

            m_dragItemRect.visible = true;
            if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack.get()))
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
            auto lastTrack = getShowedTracks().getLast(); 
            te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
            auto x = timeToX(dropTime);
            auto y = lastTrack ? getYForTrack(lastTrack) + getTrackHeight(lastTrack, m_editViewState) : 0;
            auto w = timeToX(audioFile.getLength());
            auto h = static_cast<int>(m_editViewState.m_trackDefaultHeight);
            m_dragItemRect.drawRect = {x, y, w, h};
            m_dragItemRect.name = f.getFileNameWithoutExtension();
            m_dragItemRect.valid = true;
            m_dragItemRect.visible = true;
            m_dragItemRect.colour = juce::Colours::greenyellow;
        }
    }

    repaint();
}

void SongEditorView::itemDragExit (const SourceDetails& dragSourceDetails) 
{
    m_dragItemRect.visible = false;
    repaint();
}

void SongEditorView::itemDropped (const SourceDetails& dragSourceDetails) 
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
        auto pos = dragSourceDetails.localPosition;
        bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
        auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnapedTime(xtoTime(pos.x)); 
        auto f = fileTreeComp->getSelectedFile();

        if (auto targetTrack = getTrackAt(pos.y))
        {
            if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack.get()))
            {
                if (!at->state.getProperty(IDs::isMidiTrack))
                {
                    te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
                    addWaveFileToTrack(audioFile, dropTime, at);
                }
            }
        }
        else 
        {
            EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, m_editViewState.m_applicationState.getRandomTrackColour(), dropTime);
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
        clipPos.time = {dropPos,length};

        EngineHelpers::loadAudioFileToTrack(audioFile.getFile(), track, clipPos);
    }
}

te::Track::Ptr SongEditorView::getTrackAt(int y)
{
    for (auto t : getShowedTracks())
    {
        auto s = getYForTrack(t);
        auto e = s + getTrackHeight(t, m_editViewState);
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
        
        scrollY += getTrackHeight(t, m_editViewState);
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
            int trackHeight = getTrackHeight(t, m_editViewState, false);

            if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
            {
                for (auto ap : t->getAllAutomatableParams())
                {
                    if (ap->getTrack() == t && GUIHelpers::isAutomationVisible(*ap))
                    {
                        int h = ap->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));

                        if (juce::Range<int>(scrollY + trackHeight, scrollY + trackHeight + h).contains(y))
                        {
                            return ap;
                        }

                        scrollY += h;
                    }
                }
            }

            scrollY += trackHeight;
        }
    }

    return nullptr;
}

int SongEditorView::getYForAutomatableParam(te::AutomatableParameter::Ptr ap)
{
    double scrollY = m_editViewState.m_viewY;
    for (auto t : getShowedTracks())
    {
        auto trackHeight = getTrackHeight(t, m_editViewState, false); 
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
    if (ap->getTrack() == nullptr)
        return 0;
    if (getTrackHeight(ap->getTrack(), m_editViewState) == 0 || ap->getTrack()->state.getProperty(IDs::isTrackMinimized))
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

void SongEditorView::duplicateSelectedClipsOrTimeRange()
{
    if (m_selectedRange.selectedTracks.size() == 0)
    {
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto range = te::getTimeRangeForSelectedItems(selectedClips);
        auto delta = range.getLength().inSeconds();

        moveSelectedClips(true, delta, 0);
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
            if (modifierKeys.isCommandDown())
            {

                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            }
            else
            {
                setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            }
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

void SongEditorView::moveSelectedClips(bool copy,  double delta, int verticalOffset)
{
    EngineHelpers::moveSelectedClips(copy, delta, verticalOffset,m_editViewState); 
}

juce::Array<te::Track::Ptr> SongEditorView::getShowedTracks ()
{
    juce::Array<te::Track::Ptr> showedTracks;

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
        if (EngineHelpers::isTrackShowable(t))
            if (getTrackHeight(t, m_editViewState) > 0)
                showedTracks.add(t);

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
    juce::Range<int> lassoRangeY = m_lassoComponent.getLassoRect().m_verticalRange; 

    for (auto t: getShowedTracks())
    {
        auto trackVRange = getVerticalRangeOfTrack(t, false);
        if (trackVRange.intersects (lassoRangeY))
            m_selectedRange.selectedTracks.add(t);
    }

    for (auto& ap : te::getAllAutomatableParameter(m_editViewState.m_edit))
    {
        if (GUIHelpers::isAutomationVisible(*ap))
        {
            auto rect = getAutomationRect(ap);
            juce::Range<int> vRange = {rect.getY(), rect.getBottom()};
            if (vRange.intersects(lassoRangeY))
                m_selectedRange.selectedAutomations.addIfNotAlreadyThere(ap);
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

void SongEditorView::reverseSelectedClips()
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        
    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
        {
            auto reversed = wac->getIsReversed();
            wac->setIsReversed(!reversed);
            removeThumbnail(wac);
        }
    }
}
void SongEditorView::transposeSelectedClips(float pitchChange)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        
    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
        {
            auto pitch = wac->getPitchChange();
            wac->setPitchChange(pitch + pitchChange);
            removeThumbnail(wac);
        }
    }
}

void SongEditorView::setNewTempoOfClipByNewLength(te::WaveAudioClip::Ptr wac, double newLength)
{
    auto& engine = m_editViewState.m_edit.engine;

    wac->setTimeStretchMode(te::TimeStretcher::soundtouchBetter);

    const auto audioFileInfo = wac->getAudioFile().getInfo();

    const auto loopInfo = audioFileInfo.loopInfo;
    const auto tempo = loopInfo.getBpm (audioFileInfo);

    GUIHelpers::log("file's BPM: ", tempo);

    if (newLength != 0)
    {
        const double newSpeedRatio =  audioFileInfo.getLengthInSeconds() / newLength;

        wac->setSpeedRatio(newSpeedRatio);
        wac->setLength(tracktion::TimeDuration::fromSeconds(audioFileInfo.getLengthInSeconds()) / wac->getSpeedRatio(), true);
    }
}

void SongEditorView::updateAutomationSelection(bool add)
{
    auto& sm = m_editViewState.m_selectionManager;
    sm.deselectAll();

    clearSelectedTimeRange();

    for (auto t : getShowedTracks())
    {
        auto trackHeight = getTrackHeight(t, m_editViewState, false);

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
                    auto pointPos = getPointOnAutomationRect(p.time, p.value, ap, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
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
    auto trackHeight = getTrackHeight(track, m_editViewState, withAutomation);

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

    for (auto ap : m_selectedRange.selectedAutomations)
    {
        auto as = EngineHelpers::getTrackAutomationSection(ap, m_selectedRange.timeRange);
        EngineHelpers::moveAutomationOrCopy(as, td, copy);
    }
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
void SongEditorView::removeThumbnail(te::WaveAudioClip::Ptr wac)
{
    for (int i = m_thumbnails.size(); --i >= 0;)
    {
        auto& clipThumbnail = *m_thumbnails.getUnchecked(i);

        if (clipThumbnail.waveAudioClip == wac)
        {
            m_thumbnails.remove(i);
            return;
        }
    }
}

void SongEditorView::drawTrack(juce::Graphics& g, juce::Rectangle<int> displayedRect, te::ClipTrack::Ptr clipTrack, tracktion::TimeRange etr, bool forDragging)
{
    if (!getLocalBounds().intersects(displayedRect))
        return;


    double x1beats = m_editViewState.timeToBeat(etr.getStart().inSeconds());
    double x2beats = m_editViewState.timeToBeat(etr.getEnd().inSeconds());

    g.setColour(juce::Colour(0xff252525));
    g.fillRect(displayedRect);

    auto ba = m_editViewState.xToBeats(displayedRect.getX(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    auto be = m_editViewState.xToBeats(displayedRect.getRight(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, ba, be, displayedRect);

    for (auto clipIdx = 0; clipIdx < clipTrack->getNumTrackItems(); clipIdx++)
    {
        auto clip = clipTrack->getTrackItem(clipIdx);

        if (clip->getPosition().time.intersects(etr))
        {
            int x = displayedRect.getX() + m_editViewState.timeToX(clip->getPosition().getStart().inSeconds(), displayedRect.getWidth(), x1beats, x2beats);
            int y = displayedRect.getY();
            int w = (displayedRect.getX() + m_editViewState.timeToX(clip->getPosition().getEnd().inSeconds(), displayedRect.getWidth(), x1beats, x2beats)) - x;
            int h = displayedRect.getHeight();

            juce::Rectangle<int> clipRect = {x,y,w,h};

            auto color = clip->getTrack()->getColour();

            if (forDragging)
                color = color.withAlpha(0.7f);

            if (auto c = dynamic_cast<te::Clip*>(clip))
                drawClip(g,clipRect, c, color, displayedRect, x1beats, x2beats); 
        }
    }

    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(displayedRect.getX(),displayedRect.getBottom(), displayedRect.getRight(), displayedRect.getBottom());
}

juce::Rectangle<int> SongEditorView::getClipRect (te::Clip::Ptr clip)
{
    int x = timeToX(clip->getPosition().getStart().inSeconds());
    int y = getYForTrack(clip->getClipTrack());
    int w = timeToX(clip->getPosition().getEnd().inSeconds()) - x;
    int h = getTrackHeight(clip->getClipTrack(), m_editViewState, false);

    juce::Rectangle<int> clipRect = {x,y,w,h};

    return clipRect;
}

void SongEditorView::drawClipBody(juce::Graphics& g, juce::String name, juce::Rectangle<int> clipRect,bool isSelected, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat)
{
    auto area = clipRect;
    if (clipRect.getX() < displayedRect.getX())
        area.removeFromLeft(displayedRect.getX() - clipRect.getX());
    if (clipRect.getRight() > displayedRect.getRight())
        area.removeFromRight(clipRect.getRight() - displayedRect.getRight());

    auto& evs = m_editViewState;
    auto header = area.withHeight(evs.m_clipHeaderHeight);

    auto clipColor = color;
    auto innerGlow = clipColor.brighter(0.5f);
    auto borderColour = clipColor.darker(0.95f);
    auto backgroundColor = borderColour.withAlpha(0.6f);

    auto labelBackground = juce::Colour(0x00ffffff);
    auto labelTextColor = clipColor.getPerceivedBrightness() < 0.5f
                        ? clipColor.withLightness(.75f)
                        : clipColor.darker(.75f);


    g.setColour(backgroundColor);
    g.fillRect(area.reduced(1, 1));

    g.setColour(innerGlow);
    g.drawRect(header);
    g.drawRect(area);
    g.setColour(clipColor);
    if (isSelected)
        g.setColour(clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f));

    g.fillRect(header.reduced(2,2));

    if (isSelected)
        labelTextColor = juce::Colours::black;
    g.setColour(labelTextColor);
    juce::Font labelFont (14.0f, juce::Font::bold);
    g.setFont(labelFont);
    g.drawText(name, header.reduced(4, 0), juce::Justification::centredLeft, false);
}
void SongEditorView::drawClip(juce::Graphics& g, juce::Rectangle<int> clipRect, te::Clip * clip, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat)
{

    auto& evs = m_editViewState;
    auto isSelected = evs.m_selectionManager.isSelected (clip);
    drawClipBody(g, clip->getName(), clipRect, isSelected, color, displayedRect, x1Beat, x2beat);

    auto header = clipRect.withHeight(evs.m_clipHeaderHeight);
    clipRect.removeFromTop(header.getHeight());
    clipRect.reduce(1,1);

    if (auto wac = dynamic_cast<te::WaveAudioClip*>(clip))
    {
        // m_thumbnails.clear();
        if (auto& thumb = getOrCreateThumbnail(wac))
            drawWaveform(g, *wac, *thumb, color, clipRect , displayedRect, x1Beat, x2beat);
    }
    else if (auto mc = dynamic_cast<te::MidiClip*>(clip))
    {
        drawMidiClip(g, mc, clipRect, displayedRect, color, x1Beat, x2beat);
    }
}

void SongEditorView::drawWaveform(juce::Graphics& g,
                                      te::AudioClipBase& c,
                                      te::SmartThumbnail& thumb,
                                      juce::Colour colour,
                                      juce::Rectangle<int> clipRect,
                                      juce::Rectangle<int> displayedRect, double x1Beat, double x2beat)
{
    if (m_editViewState.m_drawWaveforms == false)
        return;

    auto getTimeRangeForDrawing = [this] (const te::AudioClipBase& clip, const juce::Rectangle<int> clRect, const juce::Rectangle<int> displRect, double x1Beats, double x2Beats)
        -> tracktion::core::TimeRange
    {
        auto t1 = EngineHelpers::getTimePos(0.0);
        auto t2 = t1 + clip.getPosition().getLength();

        double displStart = m_editViewState.beatToTime(x1Beats);
        double displEnd = m_editViewState.beatToTime(x2Beats);
             
        if (clRect.getX() < displRect.getX())
            t1 = t1 + tracktion::TimeDuration::fromSeconds(displStart - clip.getPosition().getStart().inSeconds()); 

        if (clRect.getRight() > displRect.getRight())
            t2 = t2 - tracktion::TimeDuration::fromSeconds(clip.getPosition().getEnd().inSeconds() - displEnd);

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
                       , getTimeRangeForDrawing(c, clipRect, displayedRect, x1Beat, x2beat)
                       , c.isLeftChannelActive() && showBothChannels
                       , c.isRightChannelActive()
                       , gainL
                       , gainR);
        }
    }
    else if (c.getLoopLength().inSeconds() == 0)
    {
        auto region = getTimeRangeForDrawing (c, clipRect, displayedRect, x1Beat, x2beat);

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
                        , time
                        , 0
                        , leftGain);
        thumb.drawChannel(g, area,  time, 1, rightGain);
    }
    else if (useLeft)
        thumb.drawChannel (g, area, time, 0, leftGain);
    else if (useRight)
        thumb.drawChannel (g, area, time, 1, rightGain);
}


void  SongEditorView::drawMidiClip (juce::Graphics& g,te::MidiClip::Ptr clip, juce::Rectangle<int> clipRect, juce::Rectangle<int> displayedRect, juce::Colour color, double x1Beat, double x2beat)
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

        auto startX = m_editViewState.beatsToX(sBeat, displayedRect.getWidth(), x1Beat, x2beat);
        auto endX = m_editViewState.beatsToX(eBeat, displayedRect.getWidth(), x1Beat, x2beat);
        auto scrollOffset = m_editViewState.beatsToX(0.0, displayedRect.getWidth(), x1Beat, x2beat) * -1;

        int x1 = clipRect.getX() + startX + scrollOffset;
        int x2 = clipRect.getX() + endX + scrollOffset;

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
//reimplemented from te::AutomatonCurve because we 
//need to return -1 if there is no point after this position 
int SongEditorView::nextIndexAfter (tracktion::TimePosition t,te::AutomatableParameter::Ptr ap) const
{
    auto num = ap->getCurve().getNumPoints();

    for (int i = 0; i < num; ++i)
        if (ap->getCurve().getPointTime (i) >= t)
            return i;

    return -1;
}

juce::Point<float> SongEditorView::getPointOnAutomation(te::AutomatableParameter::Ptr ap, int index, juce::Rectangle<int> drawRect, double startBeat, double endBeat)
{
    auto time = ap->getCurve().getPoint(index).time;
    auto value = ap->getCurve().getPoint(index).value;
    auto point = getPointOnAutomationRect(time, value, ap, drawRect.getWidth(), startBeat, endBeat).translated((float) drawRect.getX(),(float) drawRect.getY());

    return point;
}

juce::Point<float> SongEditorView::getPointOnAutomationRect (tracktion::TimePosition t, double v, te::AutomatableParameter::Ptr ap, int w, double x1b, double x2b) 
{
   return {static_cast<float>(m_editViewState.timeToX(t.inSeconds(), w, x1b, x2b))
           , static_cast<float>(getYPos(v, ap))}; 
}

juce::Point<float> SongEditorView::getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve)
{
    auto controlX = p1.x + ( (p2.x - p1.x) * (.5f + curve) ) ;
    auto controlY = p1.y + ( (p2.y - p1.y) * (.5f - curve) ) ;

    auto curveControlPoint = juce::Point<float>(controlX, controlY);

    return curveControlPoint;
}

int SongEditorView::getYPos (double value, te::AutomatableParameter::Ptr ap)
{
    double pixelRangeStart = getAutomationPointWidth(ap) * .5;
    double pixelRangeEnd = getAutomationRect(ap).getHeight() - (getAutomationPointWidth(ap) * .5);

    double valueRangeStart = ap->valueRange.start;
    double valueRangeEnd = ap->valueRange.end;

    return static_cast<int>(juce::jmap(value, valueRangeStart, valueRangeEnd, pixelRangeEnd, pixelRangeStart));
}

double SongEditorView::getValue (int y, te::AutomatableParameter::Ptr ap)
{
    double pixelRangeStart = getAutomationPointWidth(ap) * .5;
    double pixelRangeEnd = getAutomationRect(ap).getHeight() - (getAutomationPointWidth(ap)  * .5);

    double valueRangeStart = ap->valueRange.start;
    double valueRangeEnd = ap->valueRange.end;

    return juce::jmap(static_cast<double>(y), pixelRangeStart, pixelRangeEnd, valueRangeEnd, valueRangeStart);
}

int SongEditorView::getAutomationPointWidth (te::AutomatableParameter::Ptr ap)
{
    if (getAutomationRect(ap).getHeight() <= 50)
        return 4;

    return 8;
}

void SongEditorView::drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<int> drawRect, te::AutomatableParameter::Ptr ap, bool forDragging)
{
    g.saveState();
    g.reduceClipRegion(drawRect);
    double startBeat = m_editViewState.timeToBeat(drawRange.getStart().inSeconds());
    double endBeat = m_editViewState.timeToBeat(drawRange.getEnd().inSeconds());
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(drawRect);

    auto ba = m_editViewState.xToBeats(drawRect.getX(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    auto be = m_editViewState.xToBeats(drawRect.getRight(), getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, ba, be, drawRect);

    juce::Path pointsPath;
    juce::Path selectedPointsPath;
    juce::Path hoveredPointPath;
    juce::Path curvePath;
    juce::Path hoveredCurvePath;
    juce::Path hoveredDotOnCurvePath;
    juce::Path fillPath;

    float startX =  static_cast<float>(drawRect.getX());
    float startY =  static_cast<float>(getYPos(ap->getCurve().getValueAt(drawRange.getStart()),ap) + drawRect.getY());
    float endX = drawRect.toFloat().getRight();
    float endY = static_cast<float>(getYPos(ap->getCurve().getValueAt(drawRange.getEnd()),ap) + drawRect.getY());

    auto pointBeforeDrawRange = ap->getCurve().indexBefore(drawRange.getStart());
    auto pointAfterDrawRange = nextIndexAfter(drawRange.getEnd(), ap);
    auto numPointsInDrawRange = ap->getCurve().countPointsInRegion(drawRange);
    auto numPoints = ap->getCurve().getNumPoints();

    auto ellipseRect = juce::Rectangle<int>();
    if (numPoints < 2)
    {
        curvePath.startNewSubPath({startX, startY});
        if (numPointsInDrawRange > 0)
        {
            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(ap, 0,drawRect, startBeat, endBeat).toInt(), getAutomationPointWidth(ap));
            pointsPath.addEllipse(ellipseRect.toFloat());
            if (m_hoveredAutomationPoint == 0)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(ap, 0))
                selectedPointsPath.addEllipse(ellipseRect.toFloat());
        }
        curvePath.lineTo({endX, endY});
    }
    else
    {
        if (pointBeforeDrawRange == -1)
        {
            curvePath.startNewSubPath({startX, startY});
            pointBeforeDrawRange = 0;
        }
        else
        {
            curvePath.startNewSubPath(getPointOnAutomation(ap, pointBeforeDrawRange, drawRect, startBeat, endBeat));
        }

        if (pointAfterDrawRange == -1)
            pointAfterDrawRange = ap->getCurve().getNumPoints() -1;

        for (auto i = pointBeforeDrawRange; i <= pointAfterDrawRange; i++)
        {
            auto pointXY = getPointOnAutomation(ap, i, drawRect, startBeat, endBeat);
            
            auto curve = juce::jlimit(-.5f, .5f, ap->getCurve().getPoint(i - 1).curve);
            auto curveControlPoint = getCurveControlPoint(curvePath.getCurrentPosition(), pointXY, curve);

            if (m_hoveredCurve == i && m_hoveredAutamatableParam == ap)
            {
                hoveredCurvePath.startNewSubPath (curvePath.getCurrentPosition());
                hoveredCurvePath.quadraticTo (curveControlPoint , pointXY);
                if (!m_isDragging) 
                    hoveredDotOnCurvePath.addEllipse (m_hoveredRectOnAutomation.toFloat());
            }

            curvePath.quadraticTo(curveControlPoint, pointXY);

            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(ap, i,drawRect, startBeat, endBeat).toInt(), getAutomationPointWidth(ap));
            pointsPath.addEllipse(ellipseRect.toFloat());

            if (m_hoveredAutomationPoint == i && m_hoveredAutamatableParam == ap)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(ap, i))
                selectedPointsPath.addEllipse(ellipseRect.toFloat());
        }

        if (ap->getCurve().getPoint(pointAfterDrawRange).time < drawRange.getEnd())
            curvePath.lineTo({endX, endY});
    }

    float currentX = curvePath.getCurrentPosition().getX();
    float currentY = curvePath.getCurrentPosition().getY();

    if (m_hoveredCurve == ap->getCurve().getNumPoints())
    {
        hoveredCurvePath.startNewSubPath (currentX, currentY);
        hoveredCurvePath.lineTo(endX, endY);
        if (!m_isDragging) 
            hoveredDotOnCurvePath.addEllipse (m_hoveredRectOnAutomation.toFloat());
    }

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
    
    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(drawRect.getX(),drawRect.getBottom(), drawRect.getRight(), drawRect.getBottom());

    g.restoreState();
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

void SongEditorView::updateTrackHeights(EditViewState& evs)
{
    m_trackInfos.clear();

    for (auto* track : te::getAllTracks(m_editViewState.m_edit))
    {
        if ((track != nullptr) && (EngineHelpers::isTrackShowable(track)))
        {
            TrackHeightInfo info;
            info.m_track = track;
            info.m_isMinimized = track->state.getProperty(IDs::isTrackMinimized, false);
            info.m_height = info.m_isMinimized || track->isFolderTrack()
                ? evs.m_trackHeightMinimized
                : static_cast<int>(track->state.getProperty(tracktion_engine::IDs::height, 50));

            int automationHeight = 0;
            if (!info.m_isMinimized)
            {
                for (auto apEditItems : track->getAllAutomatableEditItems())
                {
                    for (auto ap : apEditItems->getAutomatableParameters())
                    {
                        if (GUIHelpers::isAutomationVisible(*ap))
                        {
                            auto& curveState = ap->getCurve().state;
                            automationHeight += static_cast<int>(curveState.getProperty(tracktion_engine::IDs::height, 50));
                        }
                    }
                }
            }

            auto it = track;
            while (it->isPartOfSubmix())
            {
                it = it->getParentFolderTrack();
                if (it->state.getProperty(IDs::isTrackMinimized, false))
                {
                    info.m_height = 0;
                    break;
                }
            }

            info.m_automationHeight = automationHeight;
            m_trackInfos.add(info);
        }
    }
}

int SongEditorView::getTrackHeight(tracktion_engine::Track* track, EditViewState& evs, bool withAutomation)
{
    if (track == nullptr)
        return 0;

    for (const auto& info : m_trackInfos)
    {
        if (info.m_track == track)
        {
            if (!info.m_isMinimized && withAutomation)
            {
                return info.m_height + info.m_automationHeight;
            }
            else
            {
                return info.m_height;
            }
        }
    }

    return 0; 
}

void SongEditorView::logMousePositionInfo()
{
    GUIHelpers::log("------------------------------------------------------------");
    GUIHelpers::log("ToolMode    : ", (int) m_toolMode);
    GUIHelpers::log("TimeRange   : ", m_hoveredTimeRange);
    GUIHelpers::log("TimeRange L : ", m_hoveredTimeRangeLeft);
    GUIHelpers::log("TimeRange R : ", m_hoveredTimeRangeRight);
     GUIHelpers::log("Track       : ", m_hoveredTrack != nullptr);
    if (m_hoveredTrack)
    {
        GUIHelpers::log("Track : ", m_hoveredTrack->getName());
    }
    GUIHelpers::log("Clip        : ", m_hoveredClip != nullptr);
    GUIHelpers::log("Clip      L : ", m_leftBorderHovered);
    GUIHelpers::log("Clip      R : ", m_rightBorderHovered);
    if (m_hoveredAutamatableParam)
    {
        GUIHelpers::log("Automation  : ", m_hoveredAutamatableParam != nullptr);
        GUIHelpers::log("Track: ", m_hoveredAutamatableParam->getTrack()->getName());
        std::cout << "Automation name: " << m_hoveredAutamatableParam->getParameterName() << std::endl;

        std::cout << "Automation Y: " << getYForAutomatableParam(m_hoveredAutamatableParam) << " Height: " << getHeightOfAutomation(m_hoveredAutamatableParam) << std::endl;
        GUIHelpers::log("Point       : ", m_hoveredAutomationPoint) ;
        GUIHelpers::log("Curve       : ", m_hoveredCurve);

    }
    repaint();
}

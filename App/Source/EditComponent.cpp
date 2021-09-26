#include "EditComponent.h"
#include "NextLookAndFeel.h"

EditComponent::EditComponent (te::Edit& e, te::SelectionManager& sm, juce::Array<juce::Colour> tc)
    : m_edit (e)
  , m_editViewState (e, sm)
  , m_scrollbar_v (true)
  , m_scrollbar_h (false)
  , m_lassoComponent (m_editViewState)
  , m_trackColours(tc)
{
    m_edit.state.addListener (this);

    m_scrollbar_v.setAlwaysOnTop (true);
    m_scrollbar_v.setAutoHide (false);
    m_scrollbar_v.addListener (this);

    m_scrollbar_h.setAlwaysOnTop (true);
    m_scrollbar_h.setAutoHide (false);
    m_scrollbar_h.addListener (this);

    m_timeLine.setAlwaysOnTop (true);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
    m_playhead.setAlwaysOnTop (true);
    m_footerbar.setAlwaysOnTop (true);
    m_footerbar.toFront (true);


    addAndMakeVisible (m_timeLine);
    addAndMakeVisible (m_scrollbar_v);
    addAndMakeVisible (m_scrollbar_h);
    addAndMakeVisible (m_playhead);
    addChildComponent (m_lassoComponent);
    addAndMakeVisible (m_footerbar);

    markAndUpdate (m_updateTracks);
    m_editViewState.m_selectionManager.selectOnly (
                te::getAllTracks (m_edit).getLast ());
}

EditComponent::~EditComponent()
{
    m_edit.state.removeListener (this);
}

void EditComponent::paint (juce::Graphics &g)
{
    g.setColour(juce::Colour(0xff181818));
    g.fillRect (m_songeditorRect);
    g.setColour(juce::Colour(0xff555555));
    g.drawRect(m_editViewState.m_trackHeaderWidth - 1
             , 0
             , 1
             , (int) (m_songeditorRect.getHeight ()
                      + m_editViewState.m_timeLineHeight)
               );

}

void EditComponent::paintOverChildren(juce::Graphics &g)
{
    //rounded corners
    g.setColour(juce::Colour(0xff555555));

    juce::Path fakeRoundedCorners;
    auto bounds = getLocalBounds ();

    const float cornerSize = 10.f;
    fakeRoundedCorners.addRectangle(bounds);
    fakeRoundedCorners.setUsingNonZeroWinding(false);
    fakeRoundedCorners.addRoundedRectangle(bounds, cornerSize);

    g.fillPath(fakeRoundedCorners);
}


void EditComponent::resized()
{
    jassert (m_headers.size() == m_trackComps.size());
    const int timelineHeight = m_editViewState.m_timeLineHeight;
    const int trackGap = 0;
    const int trackHeaderWidth = m_editViewState.m_showHeaders
                          ? m_editViewState.m_trackHeaderWidth
                          : 10;
    auto area = getLocalBounds();
    int y = juce::roundToInt (m_editViewState.m_viewY.get()) + timelineHeight;
    int allTracksHeight = 0;
    for (int i = 0; i < juce::jmin (m_headers.size(), m_trackComps.size()); i++)
    {
        auto track = m_trackComps[i]->getTrack();
        auto trackHeader = m_headers[i];
        auto trackComp = m_trackComps[i];
        bool isMinimized = (bool)track->state.getProperty (IDs::isTrackMinimized);
        int trackHeight =
                isMinimized
                ? m_editViewState.m_trackHeightMinimized
                : (int) m_trackComps[i]->getTrack()->state.getProperty(
                      tracktion_engine::IDs::height
                      , (int) m_editViewState.m_trackDefaultHeight);

        auto trackHeaderHeight = trackHeight;
        if (!isMinimized)
        {
            for (auto apEditItems : track.get()->getAllAutomatableEditItems())
            {
                for (auto ap : apEditItems->getAutomatableParameters())
                {
                    if (ap->getCurve().getNumPoints() > 0)
                    {
                        int height = ap->getCurve ().state.getProperty(
                                    tracktion_engine::IDs::height
                                  , (int) m_editViewState.m_trackHeightMinimized);
                        trackHeaderHeight = trackHeaderHeight + height;
                    }
                }
            }
        }

        trackHeader->setBounds (2, y, trackHeaderWidth-2, trackHeaderHeight);
        trackComp->setBounds (trackHeaderWidth + 1
                              , y
                              , getWidth() - trackHeaderWidth
                              , trackHeaderHeight);
        y += trackHeaderHeight + trackGap;
        allTracksHeight += trackHeaderHeight;
    }

    for (auto t : m_trackComps)
        t->resized();

    m_playhead.setBounds (
                area.withTrimmedLeft (trackHeaderWidth)
                    .withTrimmedBottom(m_editViewState.m_footerBarHeight));
    m_lassoComponent.setBounds (
                area.withTrimmedBottom (m_editViewState.m_footerBarHeight)
                    .withTrimmedTop (timelineHeight)
                    .withTrimmedLeft (trackHeaderWidth));
    m_timeLine.setBounds(getLocalBounds ().removeFromTop(timelineHeight));

    auto songeditorHeight = getHeight()
                            - timelineHeight
                            - m_editViewState.m_footerBarHeight;
    area.removeFromTop (timelineHeight);
    m_songeditorRect = area.toFloat ();

    m_scrollbar_v.setBounds (getWidth () - 20
                           , timelineHeight
                           , 20
                           , songeditorHeight);
    m_scrollbar_v.setRangeLimits (0, allTracksHeight + (songeditorHeight/2));
    m_scrollbar_v.setCurrentRange (-m_editViewState.m_viewY, songeditorHeight);

    m_scrollbar_h.setBounds (trackHeaderWidth
                           , songeditorHeight
                             + timelineHeight
                             - m_editViewState.m_footerBarHeight
                           , getWidth () - trackHeaderWidth, 20);
    m_scrollbar_h.setRangeLimits (
                {0.0, m_editViewState.getEndScrollBeat ()});
    m_scrollbar_h.setCurrentRange ({m_editViewState.m_viewX1
                                  , m_editViewState.m_viewX2});
    m_footerbar.setBounds (area.removeFromBottom (
                               m_editViewState.m_footerBarHeight));
}

void EditComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem (10, "Add instrument track");
        m.addItem (11, "Add AudioTrack");
        m.addSeparator();

        const int res = m.show();
        auto colour = m_trackColours[m_trackComps.size () % m_trackColours.size ()];
        if (res == 10)
        {
            addAudioTrack (true, colour);
        }
        else if (res == 11)
        {
            addAudioTrack (false, colour);
        }
    }
    else
    {
        m_lassoComponent.setVisible (true);
        m_lassoComponent.mouseDown (e.getEventRelativeTo (&m_lassoComponent));
    }
}

void EditComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (m_lassoComponent.isVisible ())
    {
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
        m_lassoComponent.mouseDrag (e.getEventRelativeTo (&m_lassoComponent));
    }
}

void EditComponent::mouseUp(const juce::MouseEvent &e)
{
    if (m_lassoComponent.isVisible ())
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        m_lassoComponent.mouseUp (e.getEventRelativeTo (&m_lassoComponent));
        m_lassoComponent.setVisible (false);
    }
}

void EditComponent::mouseWheelMove(const juce::MouseEvent &event
                                   , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown())
    {
        auto rangeBegin = m_editViewState.beatsToX(
                                m_editViewState.m_viewX1
                              , m_timeLine.getWidth());
        auto visibleLength = m_editViewState.m_viewX2
                              - m_editViewState.m_viewX1;

        rangeBegin -=
                #if JUCE_MAC
                static_cast<int>(wheel.deltaX * 300);
                #else
                static_cast<int>(wheel.deltaY * 300);
                #endif

        m_editViewState.m_viewX1 = juce::jmax (0.0
                                     , m_editViewState.xToBeats(
                                         rangeBegin, m_timeLine.getWidth()));
        m_editViewState.m_viewX2 = m_editViewState.m_viewX1 + visibleLength;
    }
    else if (event.mods.isCtrlDown())
    {

    }
    else
    {
        m_scrollbar_v.setCurrentRangeStart(
                    m_scrollbar_v.getCurrentRangeStart() - wheel.deltaY * 300);
    }
}

void EditComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved
                                   , double newRangeStart)
{
    if (scrollBarThatHasMoved == &m_scrollbar_v)
    {
        m_editViewState.m_viewY = -newRangeStart;
    }
    else if(scrollBarThatHasMoved == &m_scrollbar_h)
    {
        auto zoom = m_editViewState.m_viewX2 - m_editViewState.m_viewX1;
        m_editViewState.m_viewX1 = juce::jmax(0.0, newRangeStart);
        m_editViewState.m_viewX2 = m_editViewState.m_viewX1 + zoom;
    }
}

bool EditComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::createFromDescription("CTRL + Z"))
    {
        m_editViewState.m_edit.undo ();
        return true;
    }
    if (key == juce::KeyPress::createFromDescription("CTRL + SHIFT + Z"))
    {
        m_editViewState.m_edit.redo ();
        return true;
    }
    return false;
}

void EditComponent::turnoffAllTrackOverlays()
{
    for (auto &tc : getTrackComps ())
    {
        tc->getTrackOverlay ().setVisible (false);
    }
}

void EditComponent::itemDragMove(
        const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    turnoffAllTrackOverlays();

    const auto dropPos = dragSourceDetails.localPosition;
    const auto songEditorWidth = m_timeLine.getTimeLineWidth ();
    auto targetTrackComp = getTrackComp (dropPos.getY ());
    auto draggedClip = dynamic_cast<ClipComponent*>(
                dragSourceDetails.sourceComponent.get ());
    auto sourceTrackComp = dynamic_cast<TrackComponent*>
            (dragSourceDetails.sourceComponent.get()->getParentComponent ());

    if (targetTrackComp && sourceTrackComp && draggedClip)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        auto verticalOffset = targetTrackComp->
                                getTrack ()->getIndexInEditTrackList ()
                            - sourceTrackComp->
                                getTrack ()->getIndexInEditTrackList ();

        for (auto tc : m_trackComps)
        {
            juce::Array<TrackOverlayComponent::OverlayImage> imageList;
            auto selectedClips = m_editViewState.m_selectionManager
                                    .getItemsOfType<te::Clip>();
            for (auto c : selectedClips)
            {
                bool isValid = false;
                auto idx = tc->getTrack ()->getIndexInEditTrackList ();
                if (auto target = m_edit.getTrackList ().at (idx + verticalOffset))
                {
                    isValid = (bool)target->state.getProperty (IDs::isMidiTrack)
                            == c->isMidi ();
                }

                if (auto cc = getClipComponentForClip (c))
                {
                    if (tc->getClipComponents ().contains (cc))
                    {
                        auto pos = cc->getPosition ().x
                                 - draggedClip->getPosition ().x;
                        TrackOverlayComponent::OverlayImage ovl =
                            {cc->createComponentSnapshot (
                                {0,0, cc->getWidth (), cc->getHeight ()}, false)
                              , pos
                              , isValid};
                        imageList.add (ovl);
                    }
                }
            }
            if (!imageList.isEmpty ())
            {
                auto idx = tc->getTrack ()->getIndexInEditTrackList ();

                if(auto target = getTrackCompForTrack (
                            m_edit.getTrackList ().at (idx + verticalOffset)))
                {
                    auto insertPos = dropPos.getX ()
                                   - draggedClip->getClipPosOffsetX ()
                                   - m_editViewState.m_trackHeaderWidth;
                    auto snapType = m_editViewState.getBestSnapType (
                                m_editViewState.m_viewX1
                              , m_editViewState.m_viewX2
                              , songEditorWidth);
                    insertPos = draggedClip->isShiftDown ()
                              ? insertPos
                              : m_editViewState.snapedX (insertPos
                                                       , songEditorWidth
                                                       , snapType);
                    target->getTrackOverlay ().addOverlayImageList (imageList);
                    target->getTrackOverlay ()
                            .drawImages (insertPos);
                }
            }
        }
    }
}

void EditComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto dropPos = dragSourceDetails.localPosition;

    auto dropTime = m_editViewState.xToTime (
                         dropPos.getX() - m_editViewState.m_trackHeaderWidth
                       , getWidth() - m_editViewState.m_trackHeaderWidth);
    dropTime = juce::jlimit(0.0,(double) m_editViewState.m_viewX2, dropTime);
    auto targetTrack = getTrackComp (dropPos.getY ());
    auto targetIndex = targetTrack->getTrack()->getIndexInEditTrackList();


    if (targetTrack)
    {

        if (auto lb = dynamic_cast<juce::ListBox*>(dragSourceDetails.sourceComponent.get()))
        {
            if (auto fileListComp =
                    dynamic_cast<FileListBoxComponent*>(lb->getModel ()))
            {
                tracktion_engine::AudioFile audioFile(
                            m_editViewState.m_edit.engine
                          , fileListComp->getFileList ()[lb->getLastRowSelected ()]);
                if (audioFile.isValid ())
                {
                    targetTrack->inserWave(
                                fileListComp->getFileList ()[lb->getLastRowSelected ()]
                              , dropTime);
                }
            }
        }

        if (auto clipComp = dynamic_cast<ClipComponent*>
                (dragSourceDetails.sourceComponent.get()))
        {
            auto sourceIndex = clipComp->getClip()->getTrack()->getIndexInEditTrackList();
            const auto songEditorWidth = m_timeLine.getTimeLineWidth ();
            const auto firstClipTime = clipComp->getClip ()->getPosition ().getStart ();
            const auto clickOffset = clipComp->getClickPosTime ();
            const auto xOffset = dropTime - firstClipTime - clickOffset + m_editViewState.beatToTime(m_editViewState.m_viewX1);
            auto snapType = m_editViewState.getBestSnapType (
                        m_editViewState.m_viewX1
                      , m_editViewState.m_viewX2
                      , songEditorWidth);
            const auto snapedOffsetX = m_editViewState.getSnapedTime (
                        dropTime - clickOffset + m_editViewState.beatToTime(m_editViewState.m_viewX1)
                      , snapType) - firstClipTime;
            const auto removeSource = !clipComp->isCtrlDown ();
            const auto snap = !clipComp->isShiftDown ();
            auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
            for (auto clip : selectedClips)
            {
                const auto sourceRange = clip->getEditTimeRange();
                auto verticalOffset = clip->getTrack()
                        ->getIndexInEditTrackList() - sourceIndex;
                bool isValid = (bool) m_edit.getTrackList().at(targetIndex + verticalOffset)
                        ->state.getProperty (IDs::isMidiTrack)
                        == clip->isMidi ();
                if (isValid)
                {
                    const auto targetTime = clip
                            ->getEditTimeRange().getStart() + xOffset;
                    const auto snapedTime = clip
                            ->getEditTimeRange().getStart() + snapedOffsetX;
                    const te::EditTimeRange targetRange (te::EditTimeRange::withStartAndLength(
                                snap ? snapedTime : targetTime
                                       , clip->getPosition().getLength()));
                    //move or copy
                    if (!removeSource)
                    {
                        te::duplicateClip(*clip);
                    }
                    //save clip for damage
                    clip->setStart(targetRange.end, false, true);
                    //clear region under the target
                    if (auto at = dynamic_cast<te::AudioTrack*>(
                                m_edit.getTrackList().at(
                                    targetIndex + verticalOffset)))
                    {
                        at->deleteRegion(targetRange
                                       , &m_editViewState.m_selectionManager);
                    }
                    //copy Automation if not changed the track
                    if (m_edit.getTrackList().at(
                                targetIndex + verticalOffset) == clip->getTrack()
                            && m_editViewState.m_automationFollowsClip)
                    {
                        for (auto ap : clip->getTrack()->getAllAutomatableParams())
                        {
                            if (ap->isAutomationActive())
                            {
                                auto sourcePoints = ap->getCurve()
                                        .getPointsInRegion(sourceRange);
                                auto valueAtSourceStart
                                        = ap->getCurve()
                                        .getValueAt(sourceRange.getStart());
                                auto valueAtSourceEnd
                                        = ap->getCurve()
                                        .getValueAt(sourceRange.getEnd());
                                auto valueAtTargetStart = ap->getCurve().getValueAt(targetRange.getStart());
                                auto valueAtTargetEnd = ap->getCurve().getValueAt(targetRange.getEnd());
                                //delete source region
                                if (removeSource)
                                {

                                    ap->getCurve().movePoint(
                                                ap->getCurve().addPoint(
                                                    sourceRange.getStart()
                                                  , valueAtSourceStart
                                                  , 0.0)
                                              , sourceRange.getEnd()
                                              , valueAtSourceStart
                                              ,true);
                                    ap->getCurve().addPoint(
                                        sourceRange.getStart()
                                      , valueAtSourceStart
                                      , 0.0);
                                }
                                //delete target region

                                ap->getCurve().movePoint(
                                            ap->getCurve().addPoint(
                                                targetRange.getStart()
                                              , valueAtTargetStart
                                              , 0.0)
                                          , targetRange.getEnd()
                                          , valueAtTargetStart
                                          , true);
                                ap->getCurve().addPoint(
                                    targetRange.getStart()
                                  , valueAtTargetStart
                                  , 0.0);

//                                ap->getCurve().removePointsInRegion(targetRange);
//                                ap->getCurve().addPoint(targetRange.getStart()
//                                                      , valueAtTargetStart
//                                                      , 0.0);
                                for (auto oldPoint : sourcePoints)
                                {
                                    double pointTime = oldPoint.time
                                            - sourceRange.getStart();
                                    ap->getCurve().addPoint(
                                                targetRange.getStart() + pointTime
                                                , oldPoint.value
                                                , oldPoint.curve);
                                }
                                ap->getCurve().removeRedundantPoints({sourceRange.start, targetRange.end});
//                                ap->getCurve().addPoint(targetRange.getEnd()
//                                                        , valueAtSourceEnd
//                                                        , 0.0);
//                                ap->getCurve().addPoint(targetRange.getEnd()
//                                                      , valueAtTargetEnd
//                                                      , 0.0);
                            }
                        }
                    }

                    clip->moveToTrack(*m_edit.getTrackList().at(targetIndex + verticalOffset));
                    clip->setStart(snap ? snapedTime : targetTime, false, true);
                }
            }
        }
        if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
                (dragSourceDetails.sourceComponent.get()))
        {
            auto f = fileTreeComp->getSelectedFile();
            targetTrack->inserWave(f, dropTime);
        }
    }
    if (dragSourceDetails.description == "Track")
    {
        if (auto tc = dynamic_cast<TrackHeaderComponent*>(dragSourceDetails.sourceComponent.get ()))
        {

            m_editViewState.m_edit.moveTrack (
                        tc->getTrack ()
                        , { nullptr
                          , m_edit.getTrackList ().at
                                    (m_edit.getTrackList ().size ()-1)});
        }
    }
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void EditComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails &)
{
}


void EditComponent::valueTreePropertyChanged (
        juce::ValueTree& v, const juce::Identifier& i)
{
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY
            || i == IDs::isPianoRollVisible
            || i == IDs::pianorollHeight)
        {
            markAndUpdate (m_updateZoom);
        }
        else if (i == IDs::showHeaders
                 || i == IDs::showFooters)
        {
            markAndUpdate (m_updateZoom);
        }
        else if (i == IDs::drawWaveforms)
        {
            repaint();
        }
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&v, juce::ValueTree& c)
{
    if (te::MidiClip::isClipState (c))
    {
        markAndUpdate (m_updateZoom);
    }
    if (te::TrackList::isTrack (c))
    {
        markAndUpdate (m_updateTracks);
    }
    if (c.hasType(te::IDs::AUTOMATIONCURVE))
    {
        GUIHelpers::log(c.toXmlString());
        markAndUpdate (m_updateTracks);
    }
}

void EditComponent::valueTreeChildRemoved (
        juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::MidiClip::isClipState (c))
    {
        resized ();
    }
    if (te::TrackList::isTrack (c))
    {
        m_lowerRange.removePluginRackwithTrack (m_edit.getTrackList ().getTrackFor (c));
        markAndUpdate (m_updateTracks);
    }
    if (c.hasType(te::IDs::POINT))
    {
        markAndUpdate(m_updateTracks);
    }
}

void EditComponent::valueTreeChildOrderChanged (
        juce::ValueTree& v, int a, int b)
{
    if (te::TrackList::isTrack (v.getChild (a)))
        markAndUpdate (m_updateTracks);
    else if (te::TrackList::isTrack (v.getChild (b)))
        markAndUpdate (m_updateTracks);
}

void EditComponent::handleAsyncUpdate()
{
    if (compareAndReset (m_updateTracks))
        buildTracks();
    if (compareAndReset (m_updateZoom))
    {
        refreshSnaptypeDesc ();
        m_timeLine.repaint ();
        for (auto t : m_trackComps)
            t->repaint ();
        resized();
    }
}


void EditComponent::refreshSnaptypeDesc()
{
    m_footerbar.m_snapTypeDesc =
            m_timeLine.getEditViewState ().getSnapTypeDescription (
                m_timeLine.getBestSnapType ().level);
    m_footerbar.repaint ();
}



tracktion_engine::AudioTrack::Ptr EditComponent::addAudioTrack(
        bool isMidiTrack
      , juce::Colour trackColour)
{
    if (auto track = EngineHelpers::addAudioTrack(
                isMidiTrack
              , trackColour
              , m_editViewState))
    {
         markAndUpdate (m_updateTracks);
         return track;
    }
    return nullptr;
}

void EditComponent::buildTracks()
{
    m_lowerRange.clearPluginRacks ();
    m_trackComps.clear();
    m_headers.clear();

    for (auto t : getAllTracks (m_edit))
    {
        TrackComponent* trackcomp = nullptr;

        if (t->isTempoTrack())
        {
            if (m_editViewState.m_showGlobalTrack)
                trackcomp = new TrackComponent (m_editViewState, t);
        }
        else if (t->isMarkerTrack())
        {
            if (m_editViewState.m_showMarkerTrack)
                trackcomp = new TrackComponent (m_editViewState, t);
        }
        else if (t->isChordTrack())
        {
            if (m_editViewState.m_showChordTrack)
                trackcomp = new TrackComponent (m_editViewState, t);
        }
        else if (t->isArrangerTrack())
        {
            if (m_editViewState.m_showArrangerTrack)
                trackcomp = new TrackComponent (m_editViewState, t);
        }
        else if (t->isMasterTrack())
        {
            if (m_editViewState.m_showMasterTrack)
                trackcomp = new TrackComponent (m_editViewState, t);
        }
        else
        {
            trackcomp = new TrackComponent (m_editViewState, t);
        }

        if (trackcomp != nullptr)
        {
            m_trackComps.add (trackcomp);
            addAndMakeVisible (trackcomp);

            auto trackheader = new TrackHeaderComponent (m_editViewState, t);
            m_headers.add (trackheader);
            addAndMakeVisible (trackheader);

            auto pluginrack = new PluginRackComponent (m_editViewState, t);
            m_lowerRange.addPluginRackComp(pluginrack);
            trackheader->addChangeListener (&m_lowerRange);
        }
    }

    m_playhead.toFront (false);
    resized();
}

LassoComponent* EditComponent::getLasso()
{
    return &m_lassoComponent;
}

juce::OwnedArray<TrackComponent> &EditComponent::getTrackComps()
{
    return m_trackComps;
}

TrackComponent *EditComponent::getTrackComp(int y)
{
    auto tcHeight = 0 + m_editViewState.m_timeLineHeight;
    for (auto & tc : m_trackComps)
    {
        if (y - m_editViewState.m_viewY > tcHeight
        && y - m_editViewState.m_viewY <= tcHeight + tc->getHeight ())
        {
            return tc;
        }
        tcHeight = tcHeight + tc->getHeight ();
    }
    return nullptr;
}

TrackComponent *EditComponent::getTrackCompForTrack(tracktion_engine::Track::Ptr track)
{
    for (auto &tc : m_trackComps)
    {
        if (tc->getTrack () == track)
        {
            return tc;
        }
    }
    return nullptr;
}

ClipComponent *EditComponent::getClipComponentForClip(tracktion_engine::Clip::Ptr clip)
{
    for (auto& track : m_trackComps)
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


LowerRangeComponent& EditComponent::lowerRange()
{
    return m_lowerRange;
}

void LassoComponent::paint(juce::Graphics &g)
{
    if (m_isLassoSelecting)
    {
        g.setColour (juce::Colour(0x99FFFFFF));
        g.drawRect (m_lassoRect.getRect (m_editViewState, getWidth ()));
        g.setColour (juce::Colour(0x22FFFFFF));
        g.fillRect (m_lassoRect.getRect (m_editViewState, getWidth ()));
    }
}

void LassoComponent::mouseDown(const juce::MouseEvent &e)
{
    m_clickedTime = m_editViewState.xToTime (e.getMouseDownX (), getWidth ());
    m_cachedY = m_editViewState.m_viewY;
    m_cachedX = m_editViewState.m_viewX1;
}

void LassoComponent::mouseDrag(const juce::MouseEvent &e)
{
    m_isLassoSelecting = true;
    auto offsetY = m_editViewState.m_viewY - m_cachedY;

    te::EditTimeRange timeRange(
                juce::jmin(
                    m_editViewState.xToTime (e.getPosition ().x, getWidth ())
                    , m_clickedTime)
                , juce::jmax(
                    m_editViewState.xToTime (e.getPosition ().x, getWidth ())
                    , m_clickedTime));
    double top = juce::jmin(
                e.getMouseDownY () + offsetY
                , (double) e.getPosition ().y );
    double bottom = juce::jmax(
                e.getMouseDownY () + offsetY
                , (double) e.getPosition ().y );

    m_lassoRect = {timeRange, top, bottom};
    updateSelection(e.mods.isCtrlDown ());
    repaint ();
}

void LassoComponent::mouseUp(const juce::MouseEvent &e)
{
    m_isLassoSelecting = false;
    repaint();
}

void LassoComponent::updateSelection(bool add)
{
    if (auto editComp = dynamic_cast<EditComponent*>(getParentComponent ()))
    {
        if (!add)
            m_editViewState.m_selectionManager.deselectAll ();
        if (!editComp->getTrackComps ().isEmpty ())
        {
            for (auto t : editComp->getTrackComps ())
            {
                juce::Range<double> trackVerticalRange = {
                    (double) t->getPosition ().y - m_editViewState.m_timeLineHeight
                  , (double) t->getPosition ().y - m_editViewState.m_timeLineHeight
                        + (double) t->getHeight () };
                if (trackVerticalRange.intersects (m_lassoRect.verticalRange))
                {
                    for (auto c : t->getClipComponents ())
                    {
                        if (m_lassoRect.startTime < c->getClip ()->getPosition ().getEnd ()
                                && m_lassoRect.endTime > c->getClip ()->getPosition ().getStart ())
                        {
                            m_editViewState.m_selectionManager.addToSelection (c->getClip ());
                        }
                    }
                }
            }
        }
    }
}

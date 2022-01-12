#include "EditComponent.h"

#include <utility>
#include "NextLookAndFeel.h"


//--------------------------------------------------------------------------------------------------------






EditComponent::EditComponent (te::Edit& e, te::SelectionManager& sm, juce::Array<juce::Colour> tc)
    : m_edit (e)
  , m_editViewState (e, sm)
  , m_scrollbar_v (true)
  , m_scrollbar_h (false)
  , m_lassoComponent (m_editViewState)
  , m_trackColours(std::move(tc))
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
             , (int) (m_songeditorRect.getHeight ())
                   + (int) m_editViewState.m_timeLineHeight);
}

void EditComponent::paintOverChildren(juce::Graphics &g)
{
    //cover left from timeline
    g.setColour (juce::Colour(0xff181818));
    g.fillRect (0, 0
                , m_editViewState.m_trackHeaderWidth
                , m_timeLine.getHeight ());
    g.setColour (juce::Colour(0xff444444));
    g.fillRect (m_editViewState.m_trackHeaderWidth - 1
                , 0, 1
                , m_timeLine.getHeight ());
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
            for (auto apEditItems : track->getAllAutomatableEditItems())
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
        trackComp->setBounds (trackHeaderWidth
                              , y
                              , getWidth() - trackHeaderWidth
                              , trackHeaderHeight);
        y += trackHeaderHeight;
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
    auto tlr = getLocalBounds ().removeFromTop(timelineHeight);
    tlr.removeFromLeft (trackHeaderWidth);
    m_timeLine.setBounds(tlr);

    auto songeditorHeight = getHeight()
                            - timelineHeight
                            - m_editViewState.m_footerBarHeight;
    area.removeFromTop (timelineHeight);
    m_songeditorRect = area.toFloat ();

    m_scrollbar_v.setBounds (getWidth () - 20
                           , timelineHeight
                           , 20
                           , songeditorHeight);
    m_scrollbar_v.setRangeLimits (0, allTracksHeight + ((double)songeditorHeight/2));
    m_scrollbar_v.setCurrentRange (-m_editViewState.m_viewY, songeditorHeight);

    m_scrollbar_h.setBounds (trackHeaderWidth
                           , songeditorHeight
                             + timelineHeight
                             - m_editViewState.m_footerBarHeight
                           , getWidth () - trackHeaderWidth, 20);
    updateHorizontalScrollBar();
    m_footerbar.setBounds (area.removeFromBottom (
                               m_editViewState.m_footerBarHeight));
}
void EditComponent::updateHorizontalScrollBar()
{
    m_scrollbar_h.setRangeLimits (
                {0.0, m_editViewState.getEndScrollBeat ()});
    m_scrollbar_h.setCurrentRange ({m_editViewState.m_viewX1
                                  , m_editViewState.m_viewX2});
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
                              , m_timeLine.getWidth()
                              , m_editViewState.m_viewX1
                              , m_editViewState.m_viewX2);
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
                                         rangeBegin, m_timeLine.getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
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
        GUIHelpers::moveView(m_editViewState, juce::jmax(0.0, newRangeStart));
    }
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
    const auto songEditorWidth = m_timeLine.getWidth ();
    auto targetTrackComp = getTrackComponent (dropPos.getY ());
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
                        TrackOverlayComponent::OverlayImage overlayImage =
                            {cc->createComponentSnapshot (
                                {0,0, cc->getWidth (), cc->getHeight ()}, false)
                              , pos
                              , isValid};
                        imageList.add (overlayImage);
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
                                                       , snapType
                                                       , m_editViewState.m_viewX1
                                                       , m_editViewState.m_viewX2);
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
                , getWidth() - m_editViewState.m_trackHeaderWidth
                , m_editViewState.m_viewX1
                , m_editViewState.m_viewX2);
    dropTime = juce::jlimit(0.0,(double) m_editViewState.m_viewX2, dropTime);
    auto destinationTrack = getTrackComponent (dropPos.getY ());


    if (destinationTrack)
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
                    destinationTrack->insertWave(
                        fileListComp->getFileList()[lb->getLastRowSelected()],
                        dropTime);
                }
            }
        }

        //copy/moving selected clips by drag and drop
        if (auto clipComp = dynamic_cast<ClipComponent*>
                        (dragSourceDetails.sourceComponent.get()))
                {
                    const auto songEditorWidth = m_timeLine.getWidth ();
                    const auto firstClipTime = clipComp->getClip ()->getPosition ().getStart ();
                    const auto offset = clipComp->getClickPosTime ();
                    const auto removeSource = !clipComp->isCtrlDown ();
                    const auto snap = !clipComp->isShiftDown ();
                    const auto xTime = m_editViewState.beatToTime(m_editViewState.m_viewX1);
                    const auto rawTime = juce::jmax(0.0, dropTime - offset + xTime);
                    auto snapType = m_editViewState.getBestSnapType (
                                m_editViewState.m_viewX1
                              , m_editViewState.m_viewX2
                              , songEditorWidth);
                    const auto snapedTime = m_editViewState.getSnapedTime (rawTime, snapType);
                    const auto pasteTime = !snap
                            ? rawTime - firstClipTime
                            : snapedTime - firstClipTime;
                    if (clipComp->getClip ()->getTrack () == destinationTrack->getTrack ().get ())
                    {
                        EngineHelpers::copyAutomationForSelectedClips (pasteTime, m_editViewState.m_selectionManager, !removeSource);
                    }
                    EngineHelpers::pasteClipboardToEdit (pasteTime
                                                       , firstClipTime
                                                       , destinationTrack->getTrack ()
                                                       , m_editViewState
                                                       , removeSource);
                }

        //wave file dropped
        if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
                (dragSourceDetails.sourceComponent.get()))
        {
            auto f = fileTreeComp->getSelectedFile();
            destinationTrack->insertWave(f, dropTime);
        }
    }
    else
    {
        if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
                (dragSourceDetails.sourceComponent.get()))
        {
            auto f = fileTreeComp->getSelectedFile();
            EngineHelpers::loadAudioFileAsClip (m_editViewState
                                              , f
                                              , juce::Colour(0xffff33cc)
                                              , dropTime);
        }
    }
    //track dropped
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
            || i == IDs::isPianoRollVisible
            || i == IDs::pianorollHeight
            || i == IDs::showHeaders
            || i == IDs::showFooters)
        {
            markAndUpdate (m_updateZoom);
        }
        else if (i == IDs::viewY)
        {
            resized ();
        }
        else if (i == IDs::drawWaveforms)
        {
            repaint();
        }
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
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
    if (te::TrackList::isTrack (v.getChild (a))
        || te::TrackList::isTrack (v.getChild (b)))
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
        {
            t->repaint ();
            t->resized ();
        }
        updateHorizontalScrollBar();
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
        TrackComponent* tc = nullptr;

        if (t->isTempoTrack())
        {
            if (m_editViewState.m_showGlobalTrack)
                tc = new TrackComponent (m_editViewState, t);
        }
        else if (t->isMarkerTrack())
        {
            if (m_editViewState.m_showMarkerTrack)
                tc = new TrackComponent (m_editViewState, t);
        }
        else if (t->isChordTrack())
        {
            if (m_editViewState.m_showChordTrack)
                tc = new TrackComponent (m_editViewState, t);
        }
        else if (t->isArrangerTrack())
        {
            if (m_editViewState.m_showArrangerTrack)
                tc = new TrackComponent (m_editViewState, t);
        }
        else if (t->isMasterTrack())
        {
            if (m_editViewState.m_showMasterTrack)
                tc = new TrackComponent (m_editViewState, t);
        }
        else
        {
            tc = new TrackComponent (m_editViewState, t);
        }

        if (tc != nullptr)
        {
            m_trackComps.add (tc);
            addAndMakeVisible (tc);

            auto th = new TrackHeaderComponent (m_editViewState, t);
            m_headers.add (th);
            addAndMakeVisible (th);

            auto pr = new PluginRackComponent (m_editViewState, t);
            m_lowerRange.addPluginRackComp(pr);
            th->addChangeListener (&m_lowerRange);
        }
    }

    m_playhead.toFront (false);
    resized();
}

LassoSelectionTool* EditComponent::getLasso()
{
    return &m_lassoComponent;
}

juce::OwnedArray<TrackComponent> &EditComponent::getTrackComps()
{
    return m_trackComps;
}

TrackComponent *EditComponent::getTrackComponent(int y)
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

TrackComponent *EditComponent::getTrackCompForTrack(const tracktion_engine::Track::Ptr& track)
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

ClipComponent *EditComponent::getClipComponentForClip(const tracktion_engine::Clip::Ptr& clip)
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



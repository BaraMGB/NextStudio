#include "SongEditorView.h"


SongEditorView::SongEditorView(EditViewState& evs)
        : m_editViewState(evs)
        , m_lassoComponent(evs, evs.m_viewX1, evs.m_viewX2)
{
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
}
int SongEditorView::getTrackHeight(const TrackComponent* tc) const
{
    return GUIHelpers::getTrackHeight(tc->getTrack(),m_editViewState);
}
void SongEditorView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    for (auto trackView : m_trackViews)
    {
        auto trackHeight = getTrackHeight(trackView);
        if (trackView->getTrack()->isFolderTrack())
            trackHeight = m_editViewState.m_folderTrackHeight;

        if (auto ft = trackView->getTrack()->getParentFolderTrack())
            if (ft->state.getProperty(IDs::isTrackMinimized))
                trackHeight = 0;

        trackView->setBounds (0, y, getWidth(), trackHeight);
        y += trackHeight;
    }
    m_lassoComponent.setBounds(getLocalBounds());
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
const TrackComponent& SongEditorView::getTrackView(te::Track::Ptr track)
{
    for (auto tv : m_trackViews)
        if (tv->getTrack() == track)
            return *tv;
}
void SongEditorView::clear()
{
    m_trackViews.clear(true);
    resized();
}
int SongEditorView::getSize()
{
    return m_trackViews.size();
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

    const auto dropPos = dragSourceDetails.localPosition;
    auto targetTrackComp = getTrackComponent (dropPos.getY ());
    auto draggedClip = dynamic_cast<ClipComponent*>(
        dragSourceDetails.sourceComponent.get ());
    auto sourceTrackComp = dynamic_cast<TrackComponent*>
        (dragSourceDetails.sourceComponent.get()->getParentComponent ());

    if (targetTrackComp && sourceTrackComp && draggedClip)
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        auto verticalOffset = m_trackViews.indexOf(targetTrackComp)
                              - m_trackViews.indexOf(sourceTrackComp);

        auto selectedClips =
            m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        for (auto selClip: selectedClips)
        {
            auto trackOfSelectedClip = selClip->getTrack();
            auto IndexOfSelectedClipsTrack =
                m_trackViews.indexOf(&getTrackView(trackOfSelectedClip));
            
            if (auto newTrackForSelectedClip =
                m_trackViews[IndexOfSelectedClipsTrack + verticalOffset])
            {
                bool isValid = trackWantsClip(selClip, newTrackForSelectedClip);
                
                if (auto selectedClipView = getClipComponentForClip(selClip))
                {
                    auto xDelta = selectedClipView->getPosition().x
                               - draggedClip->getPosition().x;

                    newTrackForSelectedClip->getTrackOverlay().m_imageList.add(
                        getClipOverlayImage(isValid, selectedClipView, xDelta));
                    
                    auto insertPos =
                        dropPos.getX() - draggedClip->getClipPosOffsetX();
                    int snapedX = getSnapedX(insertPos);

                    insertPos = draggedClip->isShiftDown() ? insertPos : snapedX;

                    newTrackForSelectedClip->getTrackOverlay().drawImages(insertPos);
                }
            }
        }
    }
}
int SongEditorView::getSnapedX(int x) const
{
    auto snapType = m_editViewState.getBestSnapType(
        m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth());
    int snapedX = m_editViewState.snapedX(x, getWidth(),
                                          snapType, m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    return snapedX;
}
TrackOverlayComponent::OverlayImage SongEditorView::getClipOverlayImage(bool isValid,
                                                      ClipComponent* clipView,
                                                      int xDelta) const
{
    auto clipViewRect = juce::Rectangle<int>(0,
                                             0,
                                             clipView->getWidth(),
                                             clipView->getHeight());
    const juce::Image& image =
        clipView->createComponentSnapshot(clipViewRect,
                                                  false);
    TrackOverlayComponent::OverlayImage
        overlayImage (image, xDelta, isValid);

    return overlayImage;
}
bool SongEditorView::trackWantsClip(const te::Clip* clip,
                                    const TrackComponent* track) const
{
    if (track->getTrack()->isFolderTrack())
        return false;

    return
        clip->isMidi() ==
        static_cast<bool>(track->getTrack()->state.getProperty( IDs::isMidiTrack));
}
void SongEditorView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    auto dropPos = dragSourceDetails.localPosition;

    auto dropTime = m_editViewState.xToTime (
        dropPos.getX()
        , getWidth()
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
        const auto dropPos = dragSourceDetails.localPosition;
        auto targetTrackComp = getTrackComponent (dropPos.getY ());
        auto draggedClip = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get ());
        auto sourceTrackComp = dynamic_cast<TrackComponent*> (dragSourceDetails.sourceComponent.get()->getParentComponent ());

        if (targetTrackComp && sourceTrackComp && draggedClip)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            auto verticalOffset = m_trackViews.indexOf(targetTrackComp)
                                  - m_trackViews.indexOf(sourceTrackComp);
            auto selectedClips =
                m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

            if (verticalOffset == 0)
            {
                EngineHelpers::copyAutomationForSelectedClips(
                    getPasteTime(dropTime, draggedClip),
                    m_editViewState.m_selectionManager,
                    draggedClip->isCtrlDown ());
            }

            juce::Array<te::Clip*> copyOfSelectedClips =
                cacheSelectedClips(selectedClips);

            for (auto selClip: selectedClips)
            {
                auto trackOfSelectedClip = selClip->getTrack();
                auto IndexOfSelectedClipsTrack =
                    m_trackViews.indexOf(&getTrackView(trackOfSelectedClip));
                if (auto newTrackForSelectedClip =
                        m_trackViews[IndexOfSelectedClipsTrack + verticalOffset])
                {
                    bool isValid = trackWantsClip(selClip, newTrackForSelectedClip);

                    auto selectionStart = draggedClip->getClip()->getPosition().getStart();
                    auto selClipstart = selClip->getPosition().getStart();
                    auto pasteTime =  selClipstart + getPasteTime(dropTime, draggedClip);

                    if (!draggedClip->isCtrlDown ())
                        selClip->removeFromParentTrack();

                    if (auto at = dynamic_cast<te::AudioTrack*>(newTrackForSelectedClip->getTrack().get()))
                        at->deleteRegion({pasteTime, pasteTime + selClip->getPosition().getLength()}, &m_editViewState.m_selectionManager);

                    auto newClip = copyOfSelectedClips[selectedClips.indexOf(selClip)];

                    newClip->moveToTrack(*(newTrackForSelectedClip->getTrack()));
                    newClip->setStart(pasteTime , false, true);

                    m_editViewState.m_selectionManager.addToSelection(newClip);
                }
            }
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

    setMouseCursor (juce::MouseCursor::NormalCursor);
}
juce::Array<te::Clip*> SongEditorView::cacheSelectedClips(
    juce::Array<te::Clip*>& selectedClips) const
{
    juce::Array<te::Clip*> copyOfSelectedClips;
    for (auto sc : selectedClips)
    {
        copyOfSelectedClips.add(te::duplicateClip(*sc));
        copyOfSelectedClips.getLast()->setStart(
            m_editViewState.m_edit.getLength(), false, true);
    }
            return copyOfSelectedClips;
}
double SongEditorView::getPasteTime(double dropTime,
                                    ClipComponent* draggedClip) const
{
    const auto firstClipTime = draggedClip->getClip ()->getPosition ().getStart ();
    const auto offset = draggedClip->getClickPosTime ();
    const auto removeSource = !draggedClip->isCtrlDown ();
    const auto snap = !draggedClip->isShiftDown ();
    const auto xTime = m_editViewState.beatToTime(m_editViewState.m_viewX1);
    const auto rawTime = juce::jmax(0.0, dropTime - offset + xTime);
    auto snapType = m_editViewState.getBestSnapType (
        m_editViewState.m_viewX1
        , m_editViewState.m_viewX2
        , getWidth());
    const auto snapedTime = m_editViewState.getSnapedTime (rawTime, snapType);
    const auto pasteTime = !snap
                               ? rawTime - firstClipTime
                               : snapedTime - firstClipTime;
    return pasteTime;
}
void SongEditorView::itemDragExit(const juce::DragAndDropTarget::SourceDetails&)
{
}
TrackComponent* SongEditorView::getTrackComponent(int y)
{
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
    return nullptr;
}
ClipComponent*
    SongEditorView::getClipComponentForClip(const tracktion_engine::Clip::Ptr& clip)
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
TrackComponent*
    SongEditorView::getTrackCompForTrack(const tracktion_engine::Track::Ptr& track)
{
    for (auto &tc : m_trackViews)
    {
        if (tc->getTrack () == track)
        {
            return tc;
        }
    }
    return nullptr;
}
void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_editViewState.m_selectionManager.deselectAll();
    startLasso(e);
}
void SongEditorView::updateClipCache()
{
    m_cachedSelectedClips.clear();

    for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        m_cachedSelectedClips.add(c);
}
void SongEditorView::mouseDrag(const juce::MouseEvent&e)
{
    updateLasso (e);
}
void SongEditorView::updateSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    double scrollY = m_editViewState.m_viewY;
    for (auto tv: m_trackViews)
    {
        if (auto track = tv->getTrack())
        {
            auto lassoRangeY = m_lassoComponent.getLassoRect ().m_verticalRange;
            if (getVerticalRangeOfTrack(scrollY, track).intersects (lassoRangeY) && !(track->isFolderTrack()))
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
    double scrollY, tracktion_engine::Track* track) const
{
    auto trackTop = (double) scrollY;
    auto trackBottom = trackTop + (double) GUIHelpers::getTrackHeight(track, m_editViewState, false);

    return {trackTop , trackBottom};
}
void SongEditorView::selectCatchedClips(const tracktion_engine::Track *track)
{
    for (auto ti = 0; ti < track->getNumTrackItems(); ti++)
    {
        auto item = track->getTrackItem(ti);
        if (m_lassoComponent.getLassoRect ().m_startTime < item->getPosition ().getEnd ()
                && m_lassoComponent.getLassoRect ().m_endTime > item->getPosition ().getStart ())
        {
            m_editViewState.m_selectionManager.addToSelection(item);
        }
    }
}
void SongEditorView::mouseUp(const juce::MouseEvent&)
{
    if (m_lassoComponent.isVisible ())
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        m_lassoComponent.stopLasso();
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
        tc->getTrackOverlay ().m_imageList.clear();
        tc->getTrackOverlay ().setVisible (false);
    }
}

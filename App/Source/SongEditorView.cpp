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
    return GUIHelpers::getTrackHeight(
        EngineHelpers::getAudioTrack(tc->getTrack(),m_editViewState)
        , m_editViewState, true);
}
void SongEditorView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    for (auto trackView : m_views)
    {
        auto trackHeight = getTrackHeight(trackView);
        trackView->setBounds (0, y, getWidth(), trackHeight);
        y += trackHeight;
    }
    m_lassoComponent.setBounds(getLocalBounds());
}
juce::OwnedArray<TrackComponent>& SongEditorView::getTrackViews()
{
    return m_views;
}
void SongEditorView::addTrackView(TrackComponent& tc)
{
    m_views.add(&tc);
}
void SongEditorView::updateViews()
{
    for (auto v : m_views)
    {
        addAndMakeVisible(v);
    }
    resized();
}
const TrackComponent& SongEditorView::getTrackView(te::Track::Ptr track)
{
    for (auto tv : m_views)
        if (tv->getTrack() == track)
            return *tv;
}
void SongEditorView::clear()
{
    m_views.clear(true);
    resized();
}
int SongEditorView::getSize()
{
    return m_views.size();
}
bool SongEditorView::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails&)
{ return true; }
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
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        auto verticalOffset = targetTrackComp->
                              getTrack ()->getIndexInEditTrackList ()
                              - sourceTrackComp->
                                getTrack ()->getIndexInEditTrackList ();

        for (auto tc : getTrackViews())
        {
            juce::Array<TrackOverlayComponent::OverlayImage> imageList;
            auto selectedClips = m_editViewState.m_selectionManager
                                     .getItemsOfType<te::Clip>();
            for (auto c : selectedClips)
            {
                bool isValid = false;
                auto idx = tc->getTrack ()->getIndexInEditTrackList ();
                if (auto target = m_editViewState.m_edit.getTrackList ().at (idx + verticalOffset))
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
                        m_editViewState.m_edit.getTrackList ().at (idx + verticalOffset)))
                {
                    auto insertPos = dropPos.getX ()
                                     - draggedClip->getClipPosOffsetX ();
                    auto snapType = m_editViewState.getBestSnapType (
                        m_editViewState.m_viewX1
                        , m_editViewState.m_viewX2
                        , getWidth());
                    insertPos = draggedClip->isShiftDown ()
                                    ? insertPos
                                    : m_editViewState.snapedX (insertPos
                                                              , getWidth()
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
        if (auto clipComp = dynamic_cast<ClipComponent*>
            (dragSourceDetails.sourceComponent.get()))
        {
            const auto firstClipTime = clipComp->getClip ()->getPosition ().getStart ();
            const auto offset = clipComp->getClickPosTime ();
            const auto removeSource = !clipComp->isCtrlDown ();
            const auto snap = !clipComp->isShiftDown ();
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

    setMouseCursor (juce::MouseCursor::NormalCursor);
}
void SongEditorView::itemDragExit(const juce::DragAndDropTarget::SourceDetails&)
{

}
TrackComponent* SongEditorView::getTrackComponent(int y)
{
    auto tcHeight = 0;
    for (auto & tc : m_views)
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
    for (auto& track : m_views)
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
    for (auto &tc : m_views)
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

    double trackPosY = m_editViewState.m_viewY;
    for (auto track: te::getAudioTracks(m_editViewState.m_edit))
    {

        auto lassoRangeY = m_lassoComponent.getLassoRect ().m_verticalRange;
        if (getVerticalRangeOfTrack(trackPosY, track).intersects (lassoRangeY))
        {
            selectCatchedClips(track);
        }
        trackPosY += GUIHelpers::getTrackHeight(track, m_editViewState);
    }

    if (add)
    {
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
    }
}
juce::Range<double> SongEditorView::getVerticalRangeOfTrack(
    double trackPosY, tracktion_engine::AudioTrack* track) const
{
    auto trackTop = (double) trackPosY;
    auto trackBottom = trackTop + (double) GUIHelpers::getTrackHeight(track, m_editViewState, false);

    return {trackTop , trackBottom};
}
void SongEditorView::selectCatchedClips(const tracktion_engine::AudioTrack *track)
{
    for (auto c : track->getClips())
    {
        if (m_lassoComponent.getLassoRect ().m_startTime < c->getPosition ().getEnd ()
                && m_lassoComponent.getLassoRect ().m_endTime > c->getPosition ().getStart ())
        {
            m_editViewState.m_selectionManager.addToSelection(c);
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

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
    return GUIHelpers::getTrackHeight(tc->getTrack(),m_editViewState, true);
}

void SongEditorView::paint(juce::Graphics& g)
{
	auto area = getLocalBounds();
	g.setColour(juce::Colour(0xff222222));
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
  //  || sourceDetails.description == "")
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
        m_draggedTimeDelta = xtoTime(dropPos.getX() - draggedClip->getClipPosOffsetX()) - draggedClip->getClip()->getPosition().getStart();

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

void SongEditorView::drawDraggingOverlays (const ClipComponent *draggedClip, const juce::Point<int> &dropPos,
                                           int verticalOffset)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
    for (auto sc: selectedClips)
    {
        if (auto newTrackForSelectedClip = getTrackForClip (verticalOffset, sc))
        {
            bool isValid = trackWantsClip (sc, newTrackForSelectedClip);

            if (auto selectedClipView = getClipComponentForClip (sc))
            {
                auto startOffsetX = selectedClipView->getPosition().x - draggedClip->getPosition().x;

                newTrackForSelectedClip->getTrackOverlay().m_imageList.add(
                        getClipOverlayImage (isValid, selectedClipView, startOffsetX));

                auto insertPos = dropPos.getX() - draggedClip->getClipPosOffsetX();
                int snapedX = getSnapedX (insertPos);

                insertPos = draggedClip->isShiftDown() ? insertPos : snapedX;

                newTrackForSelectedClip->getTrackOverlay().drawImages(insertPos);
            }
        }
    }
}

void SongEditorView::drawResizingOverlays (const ClipComponent *draggedClip)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
    for (auto sc: selectedClips)
    {
        auto newStart = sc->getPosition().getStart();
        auto newEnd = sc->getPosition().getEnd();

        if (draggedClip->isResizeRight())
        {
            newEnd = newEnd + m_draggedTimeDelta;
            if (!draggedClip->isShiftDown())
                newEnd = getSnapedTime (newEnd);
        }
        else
        {
            double clipStart = sc->getPosition ().getStart () - sc->getPosition ().getOffset ();
            newStart = juce::jmax (clipStart, newStart + m_draggedTimeDelta);

            if (!draggedClip->isShiftDown())
                newStart = juce::jmax (clipStart, getSnapedTime (newStart));
        }

        if (auto track = getTrackForClip (0, sc))
        {
            track->getTrackOverlay().addResizingImage (timeToX (newStart), timeToX (newEnd));
            track->getTrackOverlay().drawImages (0);
        }
    }
}

TrackComponent *SongEditorView::getTrackForClip(int verticalOffset, const te::Clip *clip)
{
    auto idx = m_trackViews.indexOf(&getTrackView(clip->getTrack()));
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
    m_cachedEditLength = m_editViewState.m_edit.getLength() * 100 ;
    auto dropTime = m_editViewState.xToTime (
        dropPos.getX()
        , getWidth()
        , m_editViewState.m_viewX1
        , m_editViewState.m_viewX2);
    dropTime = juce::jlimit(0.0,(double) m_editViewState.m_viewX2, dropTime);
    
    if (auto destinationTrack = getTrackComponent (dropPos.getY ()))
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
        addWaveFileToNewTrack(dragSourceDetails, dropTime);
    }
	//copy/moving selected clips by drag and drop // resizing
    if (auto draggedClip = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get ()))
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        auto verticalOffset = getVerticalOffset(dragSourceDetails, dropPos);
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

        if (draggedClip->isResizeRight())
            resizeSelectedClips(!draggedClip->isShiftDown());
        else if (draggedClip->isResizeLeft())
            resizeSelectedClips(!draggedClip->isShiftDown(), true);
        else
            moveSelectedClips(dropTime, draggedClip, verticalOffset);

        draggedClip->setResizeLeft(false);
        draggedClip->setResizeRight(false);
    }

    m_draggedTimeDelta = 0;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void SongEditorView::moveSelectedClips(double dropTime, ClipComponent *draggedClip, int verticalOffset)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

    if (verticalOffset == 0)
        EngineHelpers::copyAutomationForSelectedClips(
                getPasteTime(dropTime, draggedClip),
                m_editViewState.m_selectionManager,
                draggedClip->isCtrlDown ());

    juce::Array<te::Clip*> copyOfSelectedClips;
    for (auto sc : selectedClips)
    {
        auto sourceTrack = sc->getTrack();
        auto idxSourceTrack = m_trackViews.indexOf(&getTrackView(sourceTrack));
        if (auto targetTrack = m_trackViews[idxSourceTrack + verticalOffset])
        {
            if (trackWantsClip(sc, targetTrack))
            {
                auto newClip = te::duplicateClip(*sc);
                copyOfSelectedClips.add(newClip);
                newClip->setStart(newClip->getPosition().getStart() + m_cachedEditLength, false, true);

                if (!draggedClip->isCtrlDown())
                    sc->removeFromParentTrack();
                else
                    m_editViewState.m_selectionManager.deselect(sc);
            }
        }
    }

    for (auto newClip: copyOfSelectedClips)
    {
        auto sourceTrack = newClip->getTrack();
        auto idxSourceTrack = m_trackViews.indexOf(&getTrackView(sourceTrack));
        if (auto targetTrack = m_trackViews[idxSourceTrack + verticalOffset])
        {
            auto pasteTime = newClip->getPosition().getStart()
                             - m_cachedEditLength
                             + m_draggedTimeDelta;

            if (!draggedClip->isShiftDown())
                pasteTime = getSnapedTime(pasteTime);

            if (trackWantsClip(newClip, targetTrack))
            {
                auto ct = newClip->getClipTrack();
                ct->deleteRegion({pasteTime,
                                  pasteTime + newClip->getPosition().getLength()},
                                  &m_editViewState.m_selectionManager);

                newClip->moveToTrack(*targetTrack->getTrack());
                newClip->setStart(pasteTime, false, true);

                m_editViewState.m_selectionManager.addToSelection(newClip);
            }
        }
    }
}
void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();

    if (fromLeftEdge)
	{
        for (auto sc : selectedClips)
        {
            auto newStart = juce::jmax(sc->getPosition().getStart() - sc->getPosition().getOffset(),
                                       sc->getPosition().getStart() + m_draggedTimeDelta);
            if (snap)
                newStart = getSnapedTime (newStart);
            sc->setStart(newStart, true, false);

			//save clip for damage
            sc->setStart(sc->getPosition().getStart() + m_cachedEditLength, false, true);
        }
	}
    else
    {
        for (auto sc : selectedClips)
        {
            double newEnd = sc->getPosition().getEnd() + m_draggedTimeDelta;

            if (snap)
                newEnd = getSnapedTime (newEnd);
            sc->setEnd(newEnd, true);
			//save clip for damage
            sc->setStart(sc->getPosition().getStart() + m_cachedEditLength, false, true);
        }
    }

    for (auto sc : selectedClips)
    {
        auto ct = sc->getClipTrack();
		const juce::Range<double> range = {sc->getPosition().getStart() - m_cachedEditLength,
										   sc->getPosition().getEnd() - m_cachedEditLength};
        ct->deleteRegion(range, &m_editViewState.m_selectionManager);

		//restore clip
        sc->setStart(sc->getPosition().getStart() - m_cachedEditLength, false, true);
    }
}
void
SongEditorView::addWaveFileToNewTrack(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails,
                                      double dropTime) const
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
void SongEditorView::mouseUp(const juce::MouseEvent& e)
{
    if (m_lassoComponent.isVisible ())
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        m_lassoComponent.stopLasso();
    }
	if(!e.mouseWasDraggedSinceMouseDown())
	{
        m_editViewState.m_selectionManager.deselectAll ();
		m_editViewState.m_edit.getTransport().setCurrentPosition(getSnapedTime(xtoTime(e.x), true));	
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

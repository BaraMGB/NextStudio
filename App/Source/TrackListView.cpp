//
// Created by Zehn on 14.01.2022.
//

#include "TrackListView.h"
int TrackListView::getTrackHeight(const TrackHeaderComponent* header) const
{
    return GUIHelpers::getTrackHeight(
            EngineHelpers::getAudioTrack(
                    header->getTrack(), m_editViewState)
            , m_editViewState, true);
}
void TrackListView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    int allTracksHeight = 0;
    for (auto header : m_views)
    {
        auto trackHeaderHeight = getTrackHeight(header);

        header->setBounds (0, y, getWidth(), trackHeaderHeight);

        y += trackHeaderHeight;
        allTracksHeight += trackHeaderHeight;
    }
}
void TrackListView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "Track")
    {
        if (auto tc = dynamic_cast<TrackHeaderComponent*>(dragSourceDetails.sourceComponent.get ()))
        {

            m_editViewState.m_edit.moveTrack (
                tc->getTrack ()
                , { nullptr
                    , m_editViewState.m_edit.getTrackList ().at
                        (m_editViewState.m_edit.getTrackList ().size ()-1)});
        }
    }
}
void TrackListView::addHeaderViews(TrackHeaderComponent& th)
{
    m_views.add(&th);
}
void TrackListView::updateViews()
{
    for (auto v : m_views)
    {
        addAndMakeVisible(v);
    }
    resized();
}
void TrackListView::clear()
{
    m_views.clear(true);
    resized();
}
int TrackListView::getSize()
{
    return m_views.size();
}


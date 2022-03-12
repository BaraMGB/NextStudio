//
// Created by Zehn on 14.01.2022.
//

#include "TrackListView.h"

int TrackListView::getTrackHeight(const TrackHeaderComponent* header) const
{
    auto at = EngineHelpers::getAudioTrack(header->getTrack(), m_editViewState);

    return GUIHelpers::getTrackHeight(at, m_editViewState, true);
}
void TrackListView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    int allTracksHeight = 0;
    for (auto header : m_trackHeaders)
    {
        auto trackHeaderHeight = getTrackHeight(header);

        header->setBounds (0, y, getWidth(), trackHeaderHeight);

        y += trackHeaderHeight;
        allTracksHeight += trackHeaderHeight;
    }
}
void TrackListView::mouseDown(const juce::MouseEvent &e)
{
    auto colour = m_trackColours[m_trackHeaders.size () % m_trackColours.size ()];

    if (e.mods.isPopupMenu())
    {
        const int res = getPopupResult();

        if (res == 10)
            addTrack (true, colour);
        else if (res == 11)
            addTrack (false, colour);
        else if (res == 12)
            collapseTracks(true);
        else if (res == 13)
            collapseTracks(false);
    }
    else
    {
        m_editViewState.m_selectionManager.deselectAll();
    }
}
void TrackListView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    te::TrackInsertPoint ip {nullptr,
                             m_editViewState.m_edit.getTrackList().at(
                                 m_editViewState.m_edit.getTrackList().size() - 1)};

    if (dragSourceDetails.description == "Track")
        if (auto thc = dynamic_cast<TrackHeaderComponent*>(dragSourceDetails.sourceComponent.get()))
            m_editViewState.m_edit.moveTrack(thc->getTrack(), ip);
}
void TrackListView::addHeaderViews(TrackHeaderComponent& th)
{
    m_trackHeaders.add(&th);
}
void TrackListView::updateViews()
{
    removeAllChildren();
    for (auto v : m_trackHeaders)
    {
        addAndMakeVisible(v);
    }
    resized();
}
void TrackListView::clear()
{
    m_trackHeaders.clear(true);
    resized();
}
te::AudioTrack::Ptr TrackListView::addTrack(bool isMidiTrack, juce::Colour trackColour)
{
    if (auto track = EngineHelpers::addAudioTrack(
            isMidiTrack, trackColour, m_editViewState))
        return track;

    return nullptr;
}
const int TrackListView::getPopupResult()
{
    juce::PopupMenu m;
    m.addItem (10, "Add instrument track");
    m.addItem (11, "Add audio track");
    m.addSeparator();
    m.addItem (12, "collapse all tracks");
    m.addItem (13, "expand all tracks");

    return m.show();
}
void TrackListView::collapseTracks(bool minimize)
{
    for (auto th : m_trackHeaders)
        th->collapseTrack(minimize);
}
void TrackListView::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &m_editViewState.m_selectionManager)
        for(auto th : m_trackHeaders)
            th->repaint();
}

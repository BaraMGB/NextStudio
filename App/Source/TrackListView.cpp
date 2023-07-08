
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by Zehn on 14.01.2022.
//

#include "TrackListView.h"
#include "Utilities.h"
#include <algorithm>

int TrackListView::getTrackHeight(TrackHeaderComponent* header) const
{
    auto track = header->getTrack();
    if (track == nullptr)
        return 0;

    auto& trackState = track->state;

    bool isMinimized = trackState.getProperty(IDs::isTrackMinimized, false);
    auto trackHeight = isMinimized || track->isFolderTrack()
        ? m_editViewState.m_trackHeightMinimized
        : static_cast<int>(trackState.getProperty(tracktion_engine::IDs::height, 50));

    if (!isMinimized)
    {
        int automationHeight = 0;

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

        trackHeight += automationHeight;
    }

    auto it = track;
    while (it->isPartOfSubmix())
    {
        it = it->getParentFolderTrack();
        if (it->state.getProperty(IDs::isTrackMinimized, false))
        {
            trackHeight = 0;
            break;
        }
    }

    return trackHeight;
}

void TrackListView::resized()
{
    int y = juce::roundToInt (m_editViewState.m_viewY.get());
    int allTracksHeight = 0;
    auto folderIndent = static_cast<int>(m_editViewState.m_applicationState.m_folderTrackIndent);
    for (auto header : m_trackHeaders)
    {
        auto trackHeaderHeight = getTrackHeight(header);
        auto leftEdge = 0;
        auto w = getWidth();

        if (auto ft = header->getTrack()->getParentFolderTrack() )
        {
            if (auto ftv = getTrackHeaderView(ft))
            {
                leftEdge = ftv->getX() + folderIndent;
                w = ftv->getWidth() - folderIndent;
            }
            if (ft->state.getProperty(IDs::isTrackMinimized))
                trackHeaderHeight = 0;
        }

        header->setBounds(leftEdge, y, w, trackHeaderHeight);
        y += trackHeaderHeight;
        allTracksHeight += trackHeaderHeight;
    }
}
void TrackListView::mouseDown(const juce::MouseEvent &e)
{
    auto colour = m_editViewState.m_applicationState.getRandomTrackColour();

    if (e.mods.isPopupMenu())
    {
        const int res = getPopupResult();

        if (res == 10)
            addTrack (true, false, colour);
        else if (res == 11)
            addTrack (false, false, colour);
        else if (res == 12)
            addTrack (false, true, colour);
        else if (res == 13)
            collapseTracks(true);
        else if (res == 14)
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


void TrackListView::getAllCommands (juce::Array<juce::CommandID>& commands) 
{

    juce::Array<juce::CommandID> ids {

            KeyPressCommandIDs::deleteSelectedTracks,
            KeyPressCommandIDs::duplicateSelectedTracks
        };

    commands.addArray(ids);
}


void TrackListView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) 
{

    switch (commandID)
    { 
        case KeyPressCommandIDs::deleteSelectedTracks :
            result.setInfo("delete selected tracks", "delete selected", "Tracks", 0);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey , 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::duplicateSelectedTracks :
            result.setInfo("duplicate selected tracks", "duplicate selected tracks", "Tracks", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        default:
            break;
        }

}

bool TrackListView::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{

    GUIHelpers::log("TrackListView perform");
    switch (info.commandID)
    { 
        case KeyPressCommandIDs::deleteSelectedTracks:
        {
            GUIHelpers::log("deleteSelectedTracks");

            for (auto t : m_editViewState.m_selectionManager.getItemsOfType<te::Track>())
            {
                m_editViewState.m_edit.deleteTrack (t);
            }
            break;
        }
        case KeyPressCommandIDs::duplicateSelectedTracks :
        {   
            auto trackContent = std::make_unique<te::Clipboard::Tracks>();
            auto selectedTracks =  m_editViewState.m_selectionManager.getItemsOfType<te::Track>();
            for (auto t : selectedTracks) 
            {
                trackContent->tracks.push_back (t->state);
            }
            te::EditInsertPoint insertPoint(m_editViewState.m_edit);
            te::Clipboard::Tracks::EditPastingOptions options(m_editViewState.m_edit
                                                              ,insertPoint
                                                              , &m_editViewState.m_selectionManager);
            options.startTrack = selectedTracks.getLast();
            trackContent->pasteIntoEdit (options);
            break;
        }
        default:
            return false;
    }
    return true;
}

void TrackListView::addHeaderViews(std::unique_ptr<TrackHeaderComponent> header)
{
    m_trackHeaders.add(std::move(header));
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
te::AudioTrack::Ptr TrackListView::addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour)
{
    if (isFolderTrack)
        EngineHelpers::addFolderTrack(trackColour, m_editViewState);
    else if (auto track = EngineHelpers::addAudioTrack(isMidiTrack, trackColour, m_editViewState))
        return track;

    return nullptr;
}
const int TrackListView::getPopupResult()
{
    juce::PopupMenu m;
    m.addItem (10, "Add instrument track");
    m.addItem (11, "Add audio track");
    m.addItem (12, "Add folder track");
    m.addSeparator();
    m.addItem (13, "collapse all tracks");
    m.addItem (14, "expand all tracks");

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
TrackHeaderComponent *
    TrackListView::getTrackHeaderView(tracktion_engine::Track::Ptr track)
{
    for (auto thv : m_trackHeaders)
    {
        if (thv->getTrack() == track)
            return thv;
    }
    return nullptr;
}

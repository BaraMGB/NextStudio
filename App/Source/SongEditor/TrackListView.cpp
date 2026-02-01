
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

//
// Created by Zehn on 14.01.2022.
//

#include "SongEditor/TrackListView.h"
#include "Utilities/ApplicationViewState.h"
#include "SideBrowser/InstrumentEffectChooser.h"
#include "Utilities/Utilities.h"

void TrackListView::resized()
{
    auto &trackHeightManager = m_editViewState.m_trackHeightManager;
    const int yScroll = juce::roundToInt(m_editViewState.getViewYScroll(m_timeLineID));
    int y = yScroll;
    const int folderIndent = static_cast<int>(m_editViewState.m_applicationState.m_folderTrackIndent);

    for (auto header : m_trackHeaders)
    {
        auto trackHeaderHeight = trackHeightManager->getTrackHeight(header->getTrack(), true);
        auto leftEdge = 0;
        auto w = getWidth();

        if (auto ft = header->getTrack()->getParentFolderTrack())
        {
            if (auto ftv = getTrackHeaderView(ft))
            {
                leftEdge = ftv->getX() + folderIndent;
                w = ftv->getWidth() - folderIndent;
            }
        }

        header->setBounds(leftEdge, y, w, trackHeaderHeight);
        y += trackHeaderHeight;
    }
}
void TrackListView::mouseDown(const juce::MouseEvent &e)
{
    auto colour = m_editViewState.m_applicationState.getRandomTrackColour();

    if (e.mods.isPopupMenu())
    {
        const int res = getPopupResult();

        if (res == 10)
            addTrack(true, false, colour);
        else if (res == 11)
            addTrack(false, false, colour);
        else if (res == 12)
            addTrack(false, true, colour);
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
void TrackListView::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    te::TrackInsertPoint ip{nullptr, m_editViewState.m_edit.getTrackList().at(m_editViewState.m_edit.getTrackList().size() - 1)};

    if (dragSourceDetails.description == "Track")
        if (auto thc = dynamic_cast<TrackHeaderComponent *>(dragSourceDetails.sourceComponent.get()))
            m_editViewState.m_edit.moveTrack(thc->getTrack(), ip);

    if (dragSourceDetails.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<PluginListbox *>(dragSourceDetails.sourceComponent.get()))
        {
            auto track = EngineHelpers::addAudioTrack(true, m_editViewState.m_applicationState.getRandomTrackColour(), m_editViewState);
            EngineHelpers::insertPluginWithPreset(m_editViewState, track, listbox->getSelectedPlugin(m_editViewState.m_edit));
        }
    }

    if (dragSourceDetails.description == "Instrument or Effect")
    {
        if (auto lb = dynamic_cast<InstrumentEffectTable *>(dragSourceDetails.sourceComponent.get()))
        {
            auto track = EngineHelpers::addAudioTrack(true, m_editViewState.m_applicationState.getRandomTrackColour(), m_editViewState);
            EngineHelpers::insertPluginWithPreset(m_editViewState, track, lb->getSelectedPlugin(m_editViewState.m_edit));
        }
    }
}

void TrackListView::getAllCommands(juce::Array<juce::CommandID> &commands)
{

    juce::Array<juce::CommandID> ids{

        KeyPressCommandIDs::deleteSelectedTracks, KeyPressCommandIDs::duplicateSelectedTracks};

    commands.addArray(ids);
}

void TrackListView::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result)
{

    switch (commandID)
    {
    case KeyPressCommandIDs::deleteSelectedTracks:
        result.setInfo("delete selected tracks", "delete selected", "Tracks", 0);
        result.addDefaultKeypress(juce::KeyPress::backspaceKey, 0);
        result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::duplicateSelectedTracks:
        result.setInfo("duplicate selected tracks", "duplicate selected tracks", "Tracks", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    default:
        break;
    }
}

bool TrackListView::perform(const juce::ApplicationCommandTarget::InvocationInfo &info)
{

    GUIHelpers::log("TrackListView perform");
    switch (info.commandID)
    {
    case KeyPressCommandIDs::deleteSelectedTracks:
    {
        GUIHelpers::log("deleteSelectedTracks");

        for (auto t : m_editViewState.m_selectionManager.getItemsOfType<te::Track>())
        {
            m_editViewState.m_edit.deleteTrack(t);
        }
        break;
    }
    case KeyPressCommandIDs::duplicateSelectedTracks:
    {
        auto trackContent = std::make_unique<te::Clipboard::Tracks>();
        auto selectedTracks = m_editViewState.m_selectionManager.getItemsOfType<te::Track>();
        for (auto t : selectedTracks)
        {
            trackContent->tracks.push_back(t->state);
        }
        te::EditInsertPoint insertPoint(m_editViewState.m_edit);
        te::Clipboard::Tracks::EditPastingOptions options(m_editViewState.m_edit, insertPoint, &m_editViewState.m_selectionManager);
        options.startTrack = selectedTracks.getLast();
        trackContent->pasteIntoEdit(options);
        break;
    }
    default:
        return false;
    }
    return true;
}

void TrackListView::addHeaderView(std::unique_ptr<TrackHeaderComponent> header) { m_trackHeaders.add(std::move(header)); }

void TrackListView::updateViews()
{
    removeAllChildren();

    for (auto v : m_trackHeaders)
        addAndMakeVisible(v);

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
    m.addItem(10, "Add instrument track");
    m.addItem(11, "Add audio track");
    m.addItem(12, "Add folder track");
    m.addSeparator();
    m.addItem(13, "collapse all tracks");
    m.addItem(14, "expand all tracks");

    return m.show();
}

void TrackListView::collapseTracks(bool minimize)
{
    for (auto th : m_trackHeaders)
        th->collapseTrack(minimize);
}

void TrackListView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_editViewState.m_selectionManager)
        for (auto th : m_trackHeaders)
            th->repaint();

    if (source == m_editViewState.m_trackHeightManager.get())
    {
        resized();
        repaint();
    }
}

TrackHeaderComponent *TrackListView::getTrackHeaderView(tracktion_engine::Track::Ptr track)
{
    for (auto thv : m_trackHeaders)
        if (thv->getTrack() == track)
            return thv;

    return nullptr;
}

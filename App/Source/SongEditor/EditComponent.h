
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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "LowerRange/LowerRangeComponent.h"
#include "LowerRange/PluginChain/RackView.h"
#include "SideBrowser/FileBrowser.h"
#include "SongEditor/PlayHeadComponent.h"
#include "SongEditor/SongEditorView.h"
#include "SongEditor/TimeLineComponent.h"
#include "SongEditor/TrackHeadComponent.h"
#include "SongEditor/TrackLaneComponent.h"
#include "SongEditor/TrackListView.h"
#include "Tools/tools/LassoSelectionTool.h"
#include "UI/MenuBar.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

//------------------------------------------------------------------------------

class FooterBarComponent : public juce::Component
{
public:
    explicit FooterBarComponent() = default;
    void paint(juce::Graphics &g) override
    {
        // g.setColour (juce::Colour (0xff181818));
        // g.fillRect (
        //             0
        //           , 0
        //           , getWidth ()
        //           , getHeight ());
        g.setColour(juce::Colour(0xffffffff));
        g.drawText(m_snapTypeDesc, getWidth() - 100, 0, 90, getHeight(), juce::Justification::centredRight);
        g.setColour(juce::Colour(0xff555555));
    }
    juce::String m_snapTypeDesc;
};

//------------------------------------------------------------------------------

class EditComponent
    : public juce::Component
    , public juce::ApplicationCommandTarget
    , private te::ValueTreeAllEventListener
    , private FlaggedAsyncUpdater
    , private juce::ScrollBar::Listener
    , public juce::ChangeListener
    , private juce::Timer
    , public juce::Button::Listener
    , public juce::DragAndDropTarget
{
public:
    EditComponent(te::Edit &, EditViewState &m_editViewState, ApplicationViewState &avs, te::SelectionManager &, juce::ApplicationCommandManager &cm);
    ~EditComponent() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;
    void scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void buttonClicked(juce::Button *button) override;

    ApplicationCommandTarget *getNextCommandTarget() override { return findFirstTargetParentComponent(); }
    void getAllCommands(juce::Array<juce::CommandID> &commands) override;

    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result) override;

    bool perform(const juce::ApplicationCommandTarget::InvocationInfo &info) override;

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override { return false; };

    EditViewState &getEditViewState() { return m_editViewState; }
    void loopAroundSelection();
    SongEditorView &getSongEditor() { return m_songEditor; }
    TrackListView &getTrackListView() { return m_trackListView; }
    TimeLineComponent &getTimeLineComponent() { return m_timeLine; }
    void saveTempFile();
    void updateButtonIcons();

private:
    void timerCallback() override;
    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;
    void handleAsyncUpdate() override;

    void buildTracks();

    void refreshSnapTypeDesc();

    tracktion::core::TimeRange getSelectedClipRange();

    void updateHorizontalScrollBar();
    void updateVerticalScrollbar()
    {
        m_scrollbar_v.setRangeLimits(0, getSongHeight());
        m_scrollbar_v.setCurrentRange(-m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()), getScrollableSongEditorRect().getHeight() / 2.0);
    }

    void trimMidiNotesToClipStart()
    {
        auto &edit = m_editViewState.m_edit;

        for (auto track : te::getAllTracks(edit))
        {
            if (auto audioTrack = dynamic_cast<te::AudioTrack *>(track))
            {
                if (audioTrack->state.getProperty(IDs::isMidiTrack))
                {
                    for (auto clip : audioTrack->getClips())
                    {
                        if (auto midiClip = dynamic_cast<te::MidiClip *>(clip))
                        {
                            auto &sequence = midiClip->getSequence();
                            auto clipStartBeat = -midiClip->getOffsetInBeats().inBeats();
                            auto &um = edit.getUndoManager();

                            for (auto note : sequence.getNotes())
                            {
                                if (note->getStartBeat().inBeats() < clipStartBeat)
                                {
                                    // Startpunkt der Note auf den Clip-Start setzen
                                    auto newStartBeat = tracktion::BeatPosition::fromBeats(clipStartBeat);
                                    auto newLength = note->getEndBeat() - newStartBeat;
                                    note->setStartAndLength(newStartBeat, newLength, &um);
                                    GUIHelpers::log("Attention: note start corrected!");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    juce::Rectangle<int> getToolBarRect();
    juce::Rectangle<int> getEditorHeaderRect();
    juce::Rectangle<int> getTimeLineRect();
    juce::Rectangle<int> getTrackListToolsRect();
    juce::Rectangle<int> getTrackListRect();
    juce::Rectangle<int> getSongEditorRect();
    juce::Rectangle<int> getScrollableTrackListRect();
    juce::Rectangle<int> getScrollableSongEditorRect();
    juce::Rectangle<int> getBottomMixerRect();
    juce::Rectangle<int> getMasterHeaderRect();
    juce::Rectangle<int> getMasterLaneRect();
    juce::Rectangle<int> getSendsHeaderRect();
    juce::Rectangle<int> getSendsLaneRect();
    juce::Rectangle<int> getHorizontalScrollbarRect();
    juce::Rectangle<int> getFooterRect();
    juce::Rectangle<int> getPlayHeadRect();
    int getMasterAreaHeight();
    te::Track::Ptr findMasterTrack();
    int getSongHeight();

    void sendAllNotedOff();

    te::Edit &m_edit;
    EditViewState &m_editViewState;
    TimeLineComponent m_timeLine{m_editViewState, "SongEditor"};
    SongEditorView m_songEditor;
    std::unique_ptr<TrackHeaderComponent> m_masterHeader;
    std::unique_ptr<TrackLaneComponent> m_masterLane;
    juce::ApplicationCommandManager &m_commandManager;
    TrackListView m_trackListView;
    FooterBarComponent m_footerbar;

    MenuBar m_toolBar;
    juce::DrawableButton m_selectButton, m_lassoSelectButton, m_timeRangeSelectButton, m_splitClipButton, m_timeStretchButton, m_reverseClipButton, m_deleteClipButton;

    MenuBar m_trackListToolsMenu;
    juce::DrawableButton m_addAudioTrackBtn, m_addMidiTrackBtn, m_addFolderTrackBtn;

    MenuBar m_trackListControlMenu;
    juce::DrawableButton m_expandAllBtn, m_collapseAllBtn;

    juce::ScrollBar m_scrollbar_v, m_scrollbar_h;
    PlayheadComponent m_playhead{m_edit, m_editViewState, m_timeLine};

    bool m_updateTracks = false, m_updateZoom = false, m_verticalUpdateSongEditor = false, m_dragOver = false, m_noteOffAll = false;
    int m_sendsAreaHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditComponent)
};

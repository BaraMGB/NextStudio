
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

#include "Browser_Base.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"

class TrackListView
    : public juce::Component
    , public juce::DragAndDropTarget
    , private juce::ChangeListener
    , public juce::ApplicationCommandTarget
{
public:
    TrackListView(EditViewState &evs, juce::String timeLineID)
        : m_editViewState(evs),
          m_timeLineID(timeLineID)
    {
        m_editViewState.m_trackHeightManager->addChangeListener(this);
        m_editViewState.m_selectionManager.addChangeListener(this);
        setWantsKeyboardFocus(true);
    }
    ~TrackListView()
    {
        m_editViewState.m_selectionManager.removeChangeListener(this);
        m_editViewState.m_trackHeightManager->removeChangeListener(this);
    }

    void resized() override;

    void mouseDown(const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
    {

        if (auto b = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
        {
            if (b->getSelectedFile().getFileName().endsWith(".tracktionedit"))
                return false;
        }
        return true;
    }
    void itemDragMove(const SourceDetails &dragSourceDetails) override {}
    void itemDragExit(const SourceDetails &) override {}
    void itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    ApplicationCommandTarget *getNextCommandTarget() override { return findFirstTargetParentComponent(); }
    void getAllCommands(juce::Array<juce::CommandID> &commands) override;

    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result) override;

    bool perform(const juce::ApplicationCommandTarget::InvocationInfo &info) override;

    void addHeaderView(std::unique_ptr<TrackHeaderComponent> header);
    void updateViews();

    void clear();

    void collapseTracks(bool minimize);

private:
    void changeListenerCallback(juce::ChangeBroadcaster *) override;

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour);

    EditViewState &m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    const int getPopupResult();
    TrackHeaderComponent *getTrackHeaderView(tracktion_engine::Track::Ptr track);
    juce::String m_timeLineID;
};

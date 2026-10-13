
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
#include "ProjectLifecycle.h"
#include "TrackHeadComponent.h"
#include "EditViewState.h"
#include "DragEdgeScroll.h"
#include "MouseGestureInput.h"

#include <functional>
#include <utility>

class TrackListView
    : public juce::Component
    , public juce::DragAndDropTarget
    , private juce::ChangeListener
    , private juce::Timer
    , public juce::ApplicationCommandTarget
{
public:
    TrackListView(EditViewState &evs, juce::String timeLineID)
        : m_editViewState(evs),
          m_timeLineID(timeLineID)
    {
        m_editViewState.m_trackHeightManager->addChangeListener(this);
        setWantsKeyboardFocus(true);
    }
    ~TrackListView()
    {
        stopTrackDragAutoScroll();
        m_editViewState.m_trackHeightManager->removeChangeListener(this);
    }

    void resized() override;

    void mouseDown(const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
    {

        if (auto *browser = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
            if (ProjectLifecycle::isPersistentProjectFile(browser->getSelectedFile()))
                return false;
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
    void repaintTrackHeaders();

    void clear();

    void collapseTracks(bool minimize);

    using TrackDragScrollHandler = std::function<double(double)>;
    void setTrackDragScrollHandler(TrackDragScrollHandler handler) { m_trackDragScrollHandler = std::move(handler); }
    void updateTrackDragAutoScroll(const juce::MouseEvent&, TrackHeaderComponent* source);
    void stopTrackDragAutoScroll();

private:
    void changeListenerCallback(juce::ChangeBroadcaster *) override;
    void timerCallback() override;
    void performTrackDragAutoScroll();
    void refreshTrackDragTargetFeedback(const juce::MouseEvent&);
    void clearTrackDragTargetFeedback();

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour);

    EditViewState &m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    const int getPopupResult();
    TrackHeaderComponent *getTrackHeaderView(tracktion_engine::Track::Ptr track);
    juce::String m_timeLineID;
    TrackDragScrollHandler m_trackDragScrollHandler;
    MouseGestureInput m_trackDragInput;
    juce::Component::SafePointer<TrackHeaderComponent> m_trackDragSource;
    DragEdgeScroll::Config m_trackDragScrollConfig;
    double m_lastTrackDragScrollTime = 0.0;
};

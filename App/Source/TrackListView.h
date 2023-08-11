
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


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
                     , private juce::ChangeListener
                     , public juce::ApplicationCommandTarget
{
public:
    TrackListView(EditViewState& evs)
        : m_editViewState (evs)
    {
        m_editViewState.m_selectionManager.addChangeListener(this);
        setWantsKeyboardFocus(true);
    }
    ~TrackListView()
    {
        m_editViewState.m_selectionManager.removeChangeListener(this);
    }

    void resized() override;

    void mouseDown (const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource (const SourceDetails&) override{return true;}
    void itemDragMove (const SourceDetails& dragSourceDetails) override{}
    void itemDragExit (const SourceDetails&) override{}
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;

    
    ApplicationCommandTarget* getNextCommandTarget() override   { return findFirstTargetParentComponent(); }
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;

    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    
    bool perform (const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    int getTrackHeight(TrackHeaderComponent* header) const;
    void addHeaderViews(std::unique_ptr<TrackHeaderComponent> header);
    void updateViews();

    void clear();

    void collapseTracks(bool minimize);

private:

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour);

    EditViewState& m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    const int getPopupResult();
    TrackHeaderComponent* getTrackHeaderView(tracktion_engine::Track::Ptr track);
};

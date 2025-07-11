
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
#include "EditViewState.h"
#include "MenuBar.h"
#include "TimeLineComponent.h"
#include "TimelineOverlayComponent.h"
#include "PlayHeadComponent.h"
#include "MidiViewport.h"
#include "VelocityEditor.h"
#include "KeyboardView.h"

class PianoRollEditor
    : public juce::Component
    , private te::ValueTreeAllEventListener
    , private FlaggedAsyncUpdater
    , public juce::ApplicationCommandTarget
    , public juce::Button::Listener
{
public:
    explicit PianoRollEditor(EditViewState&);
    ~PianoRollEditor() override;

    void paint( juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;
    void mouseMove(const juce::MouseEvent &event) override;

    
    ApplicationCommandTarget* getNextCommandTarget() override   { return findFirstTargetParentComponent(); }
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform (const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    void updateButtonColour();
    void buttonClicked(juce::Button* button) override;

    void setTrack(tracktion_engine::Track::Ptr track);
    void clearTrack();

    TimeLineComponent& getTimeLineComponent() { return m_timeLine; }

private:

    void valueTreePropertyChanged(
        juce::ValueTree &treeWhosePropertyHasChanged
        , const juce::Identifier &property) override;
    void valueTreeChanged() override {}
    void valueTreeChildAdded(juce::ValueTree& tree,
                             juce::ValueTree& property) override;
    void valueTreeChildRemoved(juce::ValueTree& tree,
                               juce::ValueTree& property,
                               int ) override;

    EditViewState& m_editViewState;
    TimeLineComponent m_timeLine;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    std::unique_ptr<MidiViewport> m_pianoRollViewPort{nullptr};
    std::unique_ptr<VelocityEditor> m_velocityEditor{nullptr};
    std::unique_ptr<KeyboardView> m_keyboard;
    PlayheadComponent m_playhead;
    MenuBar m_toolBar;

    juce::DrawableButton    m_selectionBtn,
                            m_drawBtn,
                            m_rangeSelectBtn,
                            m_erasorBtn,
                            m_splitBtn;

    juce::String m_NoteDescUnderCursor;
    void handleAsyncUpdate() override;

    bool m_updateKeyboard {false}
        , m_updateVelocity {false}
        , m_updateNoteEditor{false}
        , m_updateClips{false}
        , m_updateTracks{false}
        , m_updateButtonColour {false};

    juce::Rectangle<int> getHeaderRect();
    juce::Rectangle<int> getToolBarRect();
    juce::Rectangle<int> getTimeLineRect();
    juce::Rectangle<int> getTimelineHelperRect();
    juce::Rectangle<int> getKeyboardRect();
    juce::Rectangle<int> getMidiEditorRect();
    juce::Rectangle<int> getParameterToolbarRect();
    juce::Rectangle<int> getVelocityEditorRect();
    juce::Rectangle<int> getFooterRect();
    juce::Rectangle<int> getPlayHeadRect();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollEditor)
};

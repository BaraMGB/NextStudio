#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "TimelineOverlayComponent.h"
#include "PlayHeadComponent.h"
#include "MidiViewport.h"
#include "VelocityEditor.h"
#include "KeyboardView.h"

class PianoRollEditor
    : public juce::Component
                         , public juce::MidiKeyboardStateListener
                         , private te::ValueTreeAllEventListener
                         , private FlaggedAsyncUpdater
{
public:
    explicit PianoRollEditor(EditViewState&);
    ~PianoRollEditor() override;

    void paint( juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;
    void mouseMove(const juce::MouseEvent &event) override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    void setTrack(tracktion_engine::Track::Ptr track);
    void clearTrack();

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
    juce::MidiKeyboardState m_keybordstate;
    KeyboardView m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    std::unique_ptr<MidiViewport> m_pianoRollViewPort{nullptr};
    std::unique_ptr<VelocityEditor> m_velocityEditor{nullptr};
    PlayheadComponent m_playhead;
    juce::String m_NoteDescUnderCursor;
    void handleAsyncUpdate() override;

    bool m_updateKeyboard {false}
        , m_updateVelocity {false}
        , m_updateNoteEditor{false}
        , m_updateClips{false}
        , m_updateTracks{false};

    juce::Rectangle<int> getHeaderRect();
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

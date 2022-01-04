#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "TimelineOverlayComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"
#include "Utilities.h"
namespace te = tracktion_engine;

class PianoRollEditorComponent
    : public juce::Component
                         , public juce::MidiKeyboardStateListener
                         , public te::ValueTreeAllEventListener
{
public:
    explicit PianoRollEditorComponent(EditViewState&);
    ~PianoRollEditorComponent() override;

    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;
    void mouseMove(const juce::MouseEvent &event) override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    void setTrack(const tracktion_engine::Track::Ptr& track);
    void clearPianoRollClip();

    void valueTreePropertyChanged(
            juce::ValueTree &treeWhosePropertyHasChanged
          , const juce::Identifier &property) override;
    void valueTreeChanged() override {}

private:

    EditViewState& m_editViewState;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    std::unique_ptr<PianoRollContentComponent>
                        m_pianoRollContentComponent{nullptr};
    PlayheadComponent m_playhead;
    juce::String m_NoteDescUnderCursor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollEditorComponent)
};

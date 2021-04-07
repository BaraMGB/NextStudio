#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "TimelineOverlayComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"

namespace te = tracktion_engine;

class PianoRollComponent : public juce::Component
                         , public juce::MidiKeyboardStateListener
                         , public te::ValueTreeAllEventListener
{
public:
    PianoRollComponent (EditViewState&);
    ~PianoRollComponent() override;

    void paintOverChildren(juce::Graphics &g);
    void focusLost (juce::Component::FocusChangeType cause) override;
    void focusGained (juce::Component::FocusChangeType cause) override;
    void resized () override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    void setPianoRollClip(std::unique_ptr<PianoRollContentComponent>);
    void clearPianoRollClip();

    void valueTreePropertyChanged(
            juce::ValueTree &treeWhosePropertyHasChanged
          , const juce::Identifier &property);
    void valueTreeChanged(){}

private:
    EditViewState& m_editViewState;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    std::unique_ptr<PianoRollContentComponent>
                        m_pianoRollContentComponent{nullptr};
    PlayheadComponent m_playhead;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};

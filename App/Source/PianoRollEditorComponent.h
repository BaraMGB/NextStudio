#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"

namespace te = tracktion_engine;

class PianoRollComponent : public juce::Component
                         , public juce::MidiKeyboardStateListener
                         , public te::ValueTreeAllEventListener
                         , public juce::ChangeListener
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

    void changeListenerCallback(juce::ChangeBroadcaster *source);

private:
    EditViewState& m_editViewState;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<PianoRollContentComponent> m_pianoRollContentComponent{nullptr};

    PlayheadComponent m_playhead;

};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"

namespace te = tracktion_engine;

class PianoRollComponent : public juce::Component
                         , public te::ValueTreeAllEventListener
                         , public juce::MidiKeyboardStateListener
{
public:
    PianoRollComponent (EditViewState&);
    ~PianoRollComponent();

    void focusLost (juce::Component::FocusChangeType cause) override;
    void focusGained (juce::Component::FocusChangeType cause) override;
    void resized () override;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    te::Clip::Ptr getClip();
    te::MidiClip* getMidiClip();

    void setPianoRollClip(std::unique_ptr<PianoRollContentComponent>);
    void clearPianoRollClip();


private:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<PianoRollContentComponent> m_pianoRollContentComponent{nullptr};

    PlayheadComponent m_playhead;

};

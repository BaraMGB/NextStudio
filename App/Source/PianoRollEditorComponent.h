#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollClipComponent.h"

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

    te::Clip::Ptr getClip()
    {
        return m_clip;
    }

    te::MidiClip* getMidiClip()
    {
        return dynamic_cast<te::MidiClip*> (m_clip.get());
    }
    void centerView();

    void setPianoRollClip(std::unique_ptr<PianoRollClipComponent> pianoRollClip)
    {
        m_pianoRollClip = std::move (pianoRollClip);
        addAndMakeVisible (m_pianoRollClip.get());
        resized ();
    }

    void clearPianoRollClip()
    {
        m_pianoRollClip.reset (nullptr);
        resized ();
    }

    juce::MidiKeyboardComponent &getKeyboard();

private:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<PianoRollClipComponent> m_pianoRollClip{nullptr};
    PlayheadComponent m_playhead;

};

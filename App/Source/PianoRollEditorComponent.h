#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"

namespace te = tracktion_engine;


class PianoRollDisplay : public juce::Component
                       , public juce::MidiKeyboardStateListener
{
public:

        PianoRollDisplay (EditViewState&
                          , te::MidiClip&
                          , juce::MidiKeyboardComponent &
                          , TimeLineComponent &);
        ~PianoRollDisplay();

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent &) override;
        void mouseUp (const juce::MouseEvent &) override;
        void mouseWheelMove (const juce::MouseEvent &event
                             , const juce::MouseWheelDetails &wheel) override;

        void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
        void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

        te::MidiClip * getMidiClip ()  { return &m_clip; }

private:
        EditViewState& m_editViewState;
        te::MidiClip & m_clip;
        juce::MidiKeyboardComponent & m_keyboard;
        TimeLineComponent & m_timeline;
    };

//------------------------------------------------------------------------------

class PianoRollComponent : public juce::Component
                          , public te::ValueTreeAllEventListener
{
public:
    PianoRollComponent (EditViewState&, te::MidiClip&);

    void resized () override;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;

    te::MidiClip * getMidiClip ()  { return &m_clip; }
private:
    EditViewState& m_editViewState;
    te::MidiClip & m_clip;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    PianoRollDisplay m_pianoRoll;
    PlayheadComponent m_playhead;

};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"

namespace te = tracktion_engine;


class PianoRollDisplay : public juce::Component
{
public:

        PianoRollDisplay (EditViewState&
                          , te::Clip::Ptr
                          , juce::MidiKeyboardComponent &
                          , TimeLineComponent &);
        ~PianoRollDisplay();

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent &) override;
        void mouseMove (const juce::MouseEvent &) override;
        void mouseExit (const juce::MouseEvent &) override;
        void mouseUp (const juce::MouseEvent &) override;
        void mouseWheelMove (const juce::MouseEvent &event
                             , const juce::MouseWheelDetails &wheel) override;

        te::MidiClip* getMidiClip()
        {
            return dynamic_cast<te::MidiClip*> (m_clip.get());
        }

private:
        void drawVerticalLines (juce::Graphics& g);
        int getNoteNumber (int y);
        te::MidiNote* getNoteByPos (juce::Point<float> pos);
        EditViewState& m_editViewState;
        te::Clip::Ptr m_clip;
        juce::MidiKeyboardComponent & m_keyboard;
        TimeLineComponent & m_timeline;
        te::MidiNote * m_clickedNote {nullptr};
        double m_clickOffset{0};
        bool m_expandLeft {false}
           , m_expandRight{false};
    };

//------------------------------------------------------------------------------

class PianoRollComponent : public juce::Component
                         , public te::ValueTreeAllEventListener
                         , public juce::MidiKeyboardStateListener
{
public:
    PianoRollComponent (EditViewState&, te::Clip::Ptr);
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
    juce::MidiKeyboardComponent &getKeyboard();

private:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;
    juce::MidiKeyboardComponent m_keyboard;
    juce::MidiKeyboardState m_keybordstate;
    TimeLineComponent m_timeline;
    PianoRollDisplay m_pianoRoll;
    PlayheadComponent m_playhead;

};

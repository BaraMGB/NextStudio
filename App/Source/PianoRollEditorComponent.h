#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace te = tracktion_engine;

class PianoRollEditorComponent : public juce::Component
                               , public juce::MidiKeyboardStateListener
                               , public juce::ChangeListener
                               , public te::ValueTreeAllEventListener
{
public:

        PianoRollEditorComponent (EditViewState&, te::MidiClip&);
        ~PianoRollEditorComponent();

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent &) override;
        void mouseUp (const juce::MouseEvent &) override;
        void mouseWheelMove (const juce::MouseEvent &event
                             , const juce::MouseWheelDetails &wheel) override;
        void resized () override;

        void changeListenerCallback (juce::ChangeBroadcaster *source) override;
        void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
        void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

        void valueTreeChanged() override {}
        void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

        te::MidiClip * getMidiClip ()  { return &m_clip; }

    protected:
        EditViewState& m_editViewState;
        te::MidiClip & m_clip;
        juce::MidiKeyboardComponent m_keyboard;

        void showContextMenu();
    private:

    };


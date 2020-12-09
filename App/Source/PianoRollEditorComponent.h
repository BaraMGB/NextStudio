#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace te = tracktion_engine;

class PianoRollEditorComponent : public juce::Component
{
public:

        PianoRollEditorComponent (EditViewState&, te::MidiClip&);

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent &) override;
        void mouseUp (const juce::MouseEvent &) override;

        te::MidiClip * getMidiClip ()  { return &m_clip; }

    protected:
        EditViewState& m_editViewState;
        te::MidiClip & m_clip;

        void showContextMenu();
    private:

    };


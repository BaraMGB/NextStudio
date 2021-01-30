#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace te = tracktion_engine;


class PianoRollClipComponent : public juce::Component
{
public:

        PianoRollClipComponent (EditViewState&, te::Clip::Ptr);
        ~PianoRollClipComponent();

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

        void setKeyWidth(float noteHeight);

        int beatsToX(double beats)
        {
            return juce::roundToInt (((beats - m_editViewState.m_pianoX1)
                                      *  getWidth())
                                      / (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1));
        }

        double xToBeats(int x)
        {
            return (double (x) / getWidth())
                    * (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1)
                                + m_editViewState.m_pianoX1;
        }

private:
        void drawVerticalLines (juce::Graphics& g);
        int getNoteNumber (int y);
        te::MidiNote* getNoteByPos (juce::Point<float> pos);
        EditViewState& m_editViewState;
        te::Clip::Ptr m_clip;
        float m_keyWidth{0};
        te::MidiNote * m_clickedNote {nullptr};
        double m_clickOffset{0};
        bool m_expandLeft {false}
           , m_expandRight{false};
    };

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PianoRollVertKeyboard : public juce::KeyboardComponentBase
{
    public: 
    PianoRollVertKeyboard()
        : juce::KeyboardComponentBase(juce::MidiKeyboardComponent::Orientation::verticalKeyboardFacingRight)
    {
        setBlackNoteWidthProportion (0.5f);
        setBlackNoteLengthProportion (0.6f);
        setScrollButtonsVisible (false);

        setInterceptsMouseClicks(false, false);
    }

    ~PianoRollVertKeyboard() = default;
    void drawKeyboardBackground(juce::Graphics &g, juce::Rectangle<float> area) override
    {
    }
    void drawWhiteKey(int midiNoteNumberm, juce::Graphics& g, juce::Rectangle<float> area) override
    {
        g.setColour(juce::Colour(0xffdddddd));
        g.fillRect(area);
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(0, area.getX(), area.getRight());
        if (juce::MidiMessage::getMidiNoteName(midiNoteNumberm, true, false, 3) == "C")
        {
            auto name = juce::MidiMessage::getMidiNoteName(midiNoteNumberm, true , true, 3);

            g.drawText(name, area, juce::Justification::right);
        }

    }
    void drawBlackKey(int midiNoteNumberm, juce::Graphics& g, juce::Rectangle<float> area) override
    {
        g.setColour(juce::Colours::black);
        g.fillRect(area);
    }
};




class KeyboardView: public juce::Component
{
public:
    explicit KeyboardView(EditViewState& evs, te::Track::Ptr track) 
        : m_editViewState(evs)
        , m_track(track)
    {
        EngineHelpers::updateMidiInputs(evs, track);
        addAndMakeVisible(&m_keyboard);
    }
    ~KeyboardView() = default;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void resized() override;

private:

    float getKey(int y);

    EditViewState& m_editViewState;
    PianoRollVertKeyboard m_keyboard;

    te::Track::Ptr m_track;

    float m_clickedKey;
    double m_keyWidthCached;
};


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
        
        g.setColour(juce::Colours::white);
        g.fillRect(area);
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
    void resized() override
    {
        double firstVisibleNote = m_editViewState.m_pianoStartKey;
        double pianoRollNoteWidth = m_editViewState.m_pianoKeyWidth;

        m_keyboard.setKeyWidth (juce::jmax(0.1f, (float) pianoRollNoteWidth * 12 / 7));
        m_keyboard.setBounds (getWidth() - 50
                             , (getHeight () - (int) m_keyboard.getTotalKeyboardWidth ()
                              + (int) (firstVisibleNote * pianoRollNoteWidth)) + 2
                             , 50
                             , (int) m_keyboard.getTotalKeyboardWidth ());
    }

private:

    float getKey(int y)
    {
        auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
        auto noteNumb =
            static_cast<float>(m_editViewState.m_pianoStartKey
                               + ((getHeight() - y) / noteHeight));
        return noteNumb;
    }

    EditViewState& m_editViewState;
    PianoRollVertKeyboard m_keyboard;

    te::Track::Ptr m_track;

    float m_clickedKey;
    double m_keyWidthCached;
};


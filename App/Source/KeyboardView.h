#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class KeyboardView
    : public juce::Component
{
public:
    explicit KeyboardView(juce::MidiKeyboardState& mks, EditViewState& evs)
        : m_keyboard(mks,  juce::MidiKeyboardComponent::Orientation::verticalKeyboardFacingRight)
        , m_editViewState(evs)
    {
        addAndMakeVisible(m_keyboard);
    }
    ~KeyboardView() override = default;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
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

    juce::MidiKeyboardComponent& getKeyboard() { return m_keyboard; }


private:

    float getKey(int y)
    {
        auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
        auto noteNumb =
            static_cast<float>(m_editViewState.m_pianoStartKey
                               + ((getHeight() - y) / noteHeight));
        return noteNumb;
    }

    juce::MidiKeyboardComponent m_keyboard;
    EditViewState& m_editViewState;

    float m_clickedKey;
    double m_keyWidthCached;
};


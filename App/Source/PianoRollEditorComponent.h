#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "TimelineOverlayComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"
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
    double m_startKeyCached;
    double m_keyWidthCached;
};


class PianoRollEditorComponent
    : public juce::Component
                         , public juce::MidiKeyboardStateListener
                         , public te::ValueTreeAllEventListener
{
public:
    explicit PianoRollEditorComponent(EditViewState&);
    ~PianoRollEditorComponent() override;

    void paint( juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;
    void mouseMove(const juce::MouseEvent &event) override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    void setTrack(const tracktion_engine::Track::Ptr& track);
    void clearPianoRollClip();

    void valueTreePropertyChanged(
            juce::ValueTree &treeWhosePropertyHasChanged
          , const juce::Identifier &property) override;
    void valueTreeChanged() override {}

private:

    EditViewState& m_editViewState;
    juce::MidiKeyboardState m_keybordstate;
    KeyboardView m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    std::unique_ptr<PianoRollContentComponent>
                        m_pianoRollContentComponent{nullptr};
    PlayheadComponent m_playhead;
    juce::String m_NoteDescUnderCursor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollEditorComponent)
};

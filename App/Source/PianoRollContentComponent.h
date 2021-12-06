#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace te = tracktion_engine;


class PianoRollContentComponent : public juce::Component
{
public:

    PianoRollContentComponent (EditViewState&, te::Track::Ptr);
    ~PianoRollContentComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseMove (const juce::MouseEvent &) override;
    void mouseExit (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;
    void mouseWheelMove (const juce::MouseEvent &event
                         , const juce::MouseWheelDetails &wheel) override;
    void setKeyWidth(float noteHeight);
    te::Track::Ptr getTrack();
    std::vector<tracktion_engine::MidiClip*> getMidiClipsOfTrack();
    te::MidiNote* getNoteByPos (juce::Point<float> pos);
private:
    void drawVerticalLines (juce::Graphics& g);
    int getNoteNumber (int y);
    te::MidiClip *getMidiclipByPos(int y);
    juce::Point<float> m_clickedPos;
    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    float m_keyWidth{0};
    te::MidiNote * m_clickedNote {nullptr};
    double m_clickOffsetBeats{0};
    bool m_expandLeft {false}
    , m_expandRight{false}
    , m_noteAdding {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContentComponent)
};

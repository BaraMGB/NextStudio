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

    te::Track::Ptr getTrack();
    std::vector<tracktion_engine::MidiClip*> getMidiClipsOfTrack();
    te::MidiNote* getNoteByPos (juce::Point<float> pos);
    [[nodiscard]] float getFirstNoteHeight() const;
    int getNoteNumber (int y);

private:

    void drawVerticalLines (juce::Graphics& g, juce::Colour colour);
    void drawNoteLines(juce::Graphics& g, juce::Rectangle<int>& area) const;
    void drawClipRange(juce::Graphics& g,
                       tracktion_engine::MidiClip* const& midiClip) const;
    void drawNote(juce::Graphics& g,
                  tracktion_engine::MidiClip* const& midiClip,
                  const tracktion_engine::MidiNote* n) const;

    void drawBottomNoteLine(juce::Graphics& g, juce::Rectangle<int>& area) const;


    [[nodiscard]] float    getfirstNote() const;
    [[nodiscard]] float    getNoteHeight() const;
    te::MidiClip*          getMidiClipByPos(int y);
    juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip* const& midiClip
                           , const tracktion_engine::MidiNote* n) const;
    static double          getNoteStartBeat(te::MidiClip* const& midiClip
                           , const te::MidiNote* n) ;
    static double          getNoteEndBeat(te::MidiClip* const& midiClip
                           , const te::MidiNote* n) ;
    juce::Colour           getNoteColour(tracktion_engine::MidiClip* const& midiClip
                           , const tracktion_engine::MidiNote* n) const;
    static float           getVelocity(const tracktion_engine::MidiNote* note);

    [[nodiscard]] double   xToBeats(const int& x) const;
    [[nodiscard]] te::TimecodeSnapType getBestSnapType() const;
    [[nodiscard]] double   getQuantizedBeat(double beat) const;

    void scrollPianoRoll(float delta);

    static bool isBeforeClipStart(double beats) ;
    static bool isAfterClipEnd(tracktion_engine::MidiClip* const& midiClip,
                               double beats) ;

    juce::Point<float>                  m_clickedPos;
    EditViewState&                      m_editViewState;
    te::Track::Ptr                      m_track;
    te::MidiNote*                       m_clickedNote {nullptr};
    double                              m_clickOffsetBeats{0};
    bool                                m_expandLeft {false}
                                        , m_expandRight{false}
                                        , m_noteAdding {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContentComponent)
};

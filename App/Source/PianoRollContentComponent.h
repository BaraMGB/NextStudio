#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "LassoSelectionTool.h"

namespace te = tracktion_engine;


class PianoRollContentComponent : public juce::Component
{
public:

    PianoRollContentComponent (EditViewState&, te::Track::Ptr);
    ~PianoRollContentComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseMove (const juce::MouseEvent &) override;
    void mouseExit (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;
    void mouseWheelMove (const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;

    te::Track::Ptr getTrack();
    std::vector<tracktion_engine::MidiClip*> getMidiClipsOfTrack();

    int getNoteNumber (int y);

private:

    void drawBarsAndBeatLines(juce::Graphics& g, juce::Colour colour);
    void drawKeyLines(juce::Graphics& g, juce::Rectangle<int>& area) const;
    void drawClipRange(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip) const;
    void drawNote(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n) const;

    [[nodiscard]] float    getStartKey() const;
    [[nodiscard]] float    getKeyWidth() const;

    [[nodiscard]] juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n) const;
    [[nodiscard]] juce::Rectangle<float> getNoteRect(int noteNum, int x1, int x2) const;

    static double          getNoteStartBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    static double          getNoteEndBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    juce::Colour           getNoteColour(tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n) const;

    te::MidiNote*          addNote(int noteNumb, const te::MidiClip* clip, double beat);
    te::MidiClip*          getMidiClipByPos(int y);
    te::MidiNote*          getNoteByPos (juce::Point<float> pos);
    void                   removeNote(te::MidiClip* clip, te::MidiNote* note);
    void                   playNote(const te::MidiClip* clip, te::MidiNote* note) const;
    static float           getVelocity(const tracktion_engine::MidiNote* note);
    void                   expandClickedNoteLeft(int targetX, bool snap);

    bool                   clipContains(const te::MidiClip* clip, te::MidiNote* note);

    juce::Array<te::MidiNote*> getSelectedNotes();

    [[nodiscard]] double   xToBeats(const int& x) const;
    [[nodiscard]] te::TimecodeSnapType getBestSnapType() const;
    [[nodiscard]] double   getQuantizedBeat(double beat) const;

    void scrollPianoRoll(float delta);

    static bool isBeforeClipStart(double beats) ;
    static bool isAfterClipEnd(tracktion_engine::MidiClip* const& midiClip, double beats) ;

    void startLasso(const juce::MouseEvent &e);
    void updateLasso(const juce::MouseEvent &e);
    void stopLasso();

    void setNoteSelected(te::MidiNote &n, bool selected);


    juce::Point<float>                  m_clickedPos;
    EditViewState&                      m_editViewState;
    te::Track::Ptr                      m_track;
    LassoSelectionTool                  m_lassoTool;
    te::MidiNote*                       m_clickedNote {nullptr};
    double                              m_clickOffsetBeats{0};

    bool                                m_expandLeft {false}
                                        , m_expandRight{false}
                                        , m_noteAdding {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContentComponent)
};

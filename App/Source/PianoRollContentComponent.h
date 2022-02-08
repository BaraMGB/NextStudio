#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "LassoSelectionTool.h"

namespace te = tracktion_engine;
class PianoRollContentComponent : public juce::Component
                                , public juce::Timer
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

    void timerCallback() override;

    te::Track::Ptr getTrack();
    juce::Array<te::MidiClip*> getMidiClipsOfTrack();

    int getNoteNumber (int y);

    void updateSelectedEvents();

private:

    void                   drawClipRange(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip);
    void                   drawNote(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip,te::MidiNote* n, double timeDelta=0.0, int noteDelta=0, double timeLeftDelta=0.0, double timeRightDelta=0.0);
    void                   drawBarsAndBeatLines(juce::Graphics& g, juce::Colour colour);
    void                   drawKeyLines(juce::Graphics& g) const;
    void                   drawKeyNum(juce::Graphics& g, const tracktion_engine::MidiNote* n, int noteDelta, juce::Rectangle<float>& noteRect) const;

    [[nodiscard]] float    getStartKey() const;
    [[nodiscard]] float    getKeyWidth() const;

    [[nodiscard]] juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n);
    [[nodiscard]] juce::Rectangle<float> getNoteRect(int noteNum, int x1, int x2) const;

    static double          getNoteStartBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    static double          getNoteEndBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    juce::Colour           getNoteColour(tracktion_engine::MidiClip* const& midiClip, tracktion_engine::MidiNote* n);

    void                   insertNote(te::MidiNote* note, te::MidiClip* clip);
    te::MidiNote*          addNewNote(int noteNumb, const te::MidiClip* clip, double beat);

    te::MidiClip*          getMidiClipAt(int x);
    te::MidiClip*          getNearestClipAfter(int x);
    te::MidiClip*          getNearestClipBefor(int x);

    double                 getKeyForY(int y);
    juce::Array<te::MidiNote*>
                           getNotesByPos(juce::Point<float> pos);

    juce::Array<te::MidiNote*>
                           getNotesInRange(juce::Range<double> beatRange, te::MidiClip* clip);

    void                   cleanUnderNote(int noteNumb, juce::Range<double> beatRange, te::MidiClip * clip);
    void                   removeNote(te::MidiClip* clip, te::MidiNote* note);
    static float           getVelocity(const tracktion_engine::MidiNote* note);

    void                   playGuideNote(const te::MidiClip* clip,const int noteNumb, int vel= 100);

    void                   expandSelectedNotesLeft(const juce::MouseEvent& e);
    void                   expandSelectedNotesRight(const juce::MouseEvent& e);
    void                   moveSelectedNotesToMousePos(const juce::MouseEvent& e);

    [[nodiscard]] double   beatsToTime(double beats);
    [[nodiscard]] int      beatsToX(double beats);
    [[nodiscard]] double   xToBeats(const int& x) const;
    [[nodiscard]] double   timeToX(const double& time) const;
    [[nodiscard]] double   timeToBeat(double time);
    [[nodiscard]] te::TimecodeSnapType
                           getBestSnapType() const;
    [[nodiscard]] double   getQuantizedBeat(double beat) const;

    void                   scrollPianoRoll(float delta);

    void                   startLasso(const juce::MouseEvent &e);
    void                   updateLasso(const juce::MouseEvent &e);
    void                   stopLasso();
    void                   updateLassoSelection();
    juce::Range<double>    getLassoVerticalKeyRange();
    bool                   isInLassoRange(const te::MidiClip* clip, const tracktion_engine::MidiNote* midiNote);

    void                   setNoteSelected(te::MidiNote* n, bool addToSelection);
    void                   deleteSelectedNotes();
    juce::Array<te::MidiNote*>
                           getSelectedNotes();
    bool                   isSelected(tracktion_engine::MidiNote* note);
    void                   unselectAll();

    int                    getYForKey(double key);

    void                   moveSelectedNotesToTemp(const double startDelta, const double lengthDelta, juce::Array<std::pair<te::MidiNote*, te::MidiClip*>>& temp);
    void                   cleanUpFlags();

    EditViewState&                              m_editViewState;
    te::Track::Ptr                              m_track;
    LassoSelectionTool                          m_lassoTool;
    te::MidiNote*                               m_clickedNote {nullptr};
    std::unique_ptr<te::SelectedMidiEvents>     m_selectedEvents;
    double                                      m_clickedKey{0.0};
    double                                      m_clickOffsetBeats{0};
    te::MidiClip*                               m_clickedClip{nullptr};
    double                                      m_draggedTimeDelta {0.0};
    int                                         m_draggedNoteDelta {0};

    double                                      m_leftTimeDelta{0.0};
    double                                      m_rightTimeDelta{0.0};
    bool m_expandLeft {false}, m_expandRight {false}, m_noteAdding {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContentComponent)
};

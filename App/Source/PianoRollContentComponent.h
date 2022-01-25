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
    juce::Array<te::MidiClip*> getMidiClipsOfTrack();

    int getNoteNumber (int y);

    void drawClipRange(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip) const;
    void drawNote(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip,te::MidiNote* n
                  , double timeDelta=0.0, int noteDelta=0, double timeLeftDelta=0.0, double timeRightDelta=0.0);
private:

    void drawBarsAndBeatLines(juce::Graphics& g, juce::Colour colour);
    void drawKeyLines(juce::Graphics& g, juce::Rectangle<int>& area) const;

    [[nodiscard]] float    getStartKey() const;
    [[nodiscard]] float    getKeyWidth() const;

    [[nodiscard]] juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n) const;
    [[nodiscard]] juce::Rectangle<float> getNoteRect(int noteNum, int x1, int x2) const;

    static double          getNoteStartBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    static double          getNoteEndBeat(te::MidiClip* const& midiClip, const te::MidiNote* n) ;
    juce::Colour           getNoteColour(tracktion_engine::MidiClip* const& midiClip, tracktion_engine::MidiNote* n);

    te::MidiNote*          addNote(int noteNumb, const te::MidiClip* clip, double beat);
    te::MidiClip*          getMidiClipByPos(int y);
    te::MidiNote*          getNoteByPos (juce::Point<float> pos);
    double getKeyFromY(int y);
    void                   removeNote(te::MidiClip* clip, te::MidiNote* note);
    void                   playNote(const te::MidiClip* clip, te::MidiNote* note) const;
    static float           getVelocity(const tracktion_engine::MidiNote* note);
    bool                   clipContains(const te::MidiClip* clip, te::MidiNote* note);

    void expandSelectedNotesLeft(const juce::MouseEvent& e);
    void expandSelectedNotesRight(const juce::MouseEvent& e);
    void moveSelectedNotesToMousePos(const juce::MouseEvent& e);

    [[nodiscard]] double beatsToTime(double beats);
    [[nodiscard]] int beatsToX(double beats);
    [[nodiscard]] double   xToBeats(const int& x) const;
    [[nodiscard]] double   timeToX(const double& time) const;
    [[nodiscard]] te::TimecodeSnapType getBestSnapType() const;
    [[nodiscard]] double   getQuantizedBeat(double beat) const;

    void scrollPianoRoll(float delta);

    static bool isBeforeClipStart(double beats) ;
    static bool isAfterClipEnd(tracktion_engine::MidiClip* const& midiClip, double beats) ;

    void startLasso(const juce::MouseEvent &e);
    void updateLasso(const juce::MouseEvent &e);
    void stopLasso();

    void updateSelection();

    juce::Array<te::MidiNote*> getSelectedNotes();
    void setNoteSelected(te::MidiNote &n, tracktion_engine::MidiClip& c);
    void unselectAll();

    juce::Range<double> getLassoVerticalRange()
    {
        if (m_lassoTool.isVisible())
        {
            auto top = m_lassoTool.getLassoRect().m_top;
            auto bottom = m_lassoTool.getLassoRect().m_bottom;
            juce::Range<double> range (juce::jmin(getKeyFromY(top), getKeyFromY(bottom))
                                      ,juce::jmax(getKeyFromY(top), getKeyFromY(bottom)));
            return range;
        }
        return {0,0};
    }

    juce::Point<float>                  m_clickedPos;
    EditViewState&                      m_editViewState;
    te::Track::Ptr                      m_track;
    LassoSelectionTool                  m_lassoTool;
    te::MidiNote*                       m_clickedNote {nullptr};
    std::unique_ptr<te::SelectedMidiEvents>    m_selectedEvents;
    double                              m_clickedKey;
    double                              m_clickOffsetBeats{0};

    bool                                m_expandLeft            {false}
                                        , m_expandRight         {false}
                                        , m_noteAdding          {false};
    int getYfromKey(double key);
    void deleteSelectedNotes();
    bool isSelected(tracktion_engine::MidiNote* note);
    te::MidiClip* m_clickedClip;
    double m_draggedTimeDelta {0.0};
    int m_draggedNoteDelta {0};
    double m_leftTimeDelta{0.0};
    double m_rightTimeDelta{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContentComponent)

};

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/


#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "LassoSelectionTool.h"
#include "Utilities.h"
#include "TimeLineComponent.h"

namespace te = tracktion_engine;

struct MidiNote 
{
    tracktion::BeatPosition startBeat;
    tracktion::BeatDuration Lenght;
    int noteNumber;
    int colourIndex;
    int velocity;
    tracktion::EditItemID clip;
};

class MidiViewport : public juce::Component
                                , public juce::Timer
{
public:

    MidiViewport (EditViewState&, te::Track::Ptr, TimeLineComponent& timeLine);
    ~MidiViewport() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent &) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;
    void mouseExit (const juce::MouseEvent &) override;
    void mouseWheelMove (const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;

    void timerCallback() override;

    te::Track::Ptr getTrack();

    int getNoteNumber (int y);

    void updateSelectedEvents();
    void duplicateSelectedNotes();

    void deleteSelectedNotes();
    
    te::SelectedMidiEvents& getSelectedEvents() { return *m_selectedEvents;}

    [[nodiscard]] te::TimecodeSnapType getBestSnapType() const;

    void setToolMode (Tool mode){ m_toolMode = mode; }
    Tool getToolMode () { return m_toolMode; }

private:

    void                   drawClipRange(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip);
    void                   drawNote(juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip,te::MidiNote* n);
    void                   drawDraggedNotes(juce::Graphics& g, te::MidiNote* n, te::MidiClip* clip);
    void                   drawBarsAndBeatLines(juce::Graphics& g, juce::Colour colour);
    void                   drawKeyLines(juce::Graphics& g) const;
    void                   drawKeyNum(juce::Graphics& g, const tracktion_engine::MidiNote* n,juce::Rectangle<float>& noteRect) const;

    [[nodiscard]] float    getStartKey() const;
    [[nodiscard]] float    getKeyWidth() const;

    [[nodiscard]] juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip* const& midiClip, const tracktion_engine::MidiNote* n);
    [[nodiscard]] juce::Rectangle<float> getNoteRect(int noteNum, int x1, int x2) const;

    juce::Colour           getNoteColour(tracktion_engine::MidiClip* const& midiClip, tracktion_engine::MidiNote* n);

    void                   insertNote(MidiNote note);
    te::MidiClip*          getMidiClipForNote(MidiNote note);
    te::MidiNote*          addNewNote(int noteNumb, const te::MidiClip* clip, double beat, double length=-1);

    double                 getKeyForY(int y);
    int                    getYForKey(double key);
    te::MidiNote*          getNoteByPos(juce::Point<float> pos);
    juce::Array<te::MidiNote*>
                           getNotesInRange(juce::Range<double> beatRange, const te::MidiClip* clip);
    void                   cleanUnderNote(int noteNumb, juce::Range<double> beatRange, const te::MidiClip* clip);
    void                   removeNote(te::MidiClip* clip, te::MidiNote* note);
    void                   splitNoteAt(te::MidiClip* clip, te::MidiNote* note, double time);
    static float           getVelocity(const tracktion_engine::MidiNote* note);

    void                   playGuideNote(const te::MidiClip* clip,const int noteNumb, int vel= 100);

    void                   updateViewOfMoveSelectedNotes(const juce::MouseEvent& e);
    double                 getDraggedTimeDelta(const juce::MouseEvent& e, double oldTime);

    te::MidiClip*          getMidiClipAt(int x);
    te::MidiClip*          getNearestClipAfter(int x);
    te::MidiClip*          getNearestClipBefore(int x);
    juce::Rectangle<float> getClipRect(te::Clip* clip);


    [[nodiscard]] double   getQuantisedBeat(double beat, bool down=true) const;
    [[nodiscard]] double   getQuantisedNoteBeat(double beat,const te::MidiClip* c, bool down=true) const;
    void                   snapToGrid(te::MidiNote* note, const te::MidiClip* clip) const;
    double                 getSnapedTime(double time);
    void                   scrollPianoRoll(float delta);

    void                   startLasso(const juce::MouseEvent &e, bool isRangeTool=false);
    void                   updateLasso(const juce::MouseEvent &e);
    void                   stopLasso();
    void                   updateLassoSelection();
    juce::Range<double>    getLassoVerticalKeyRange();
    bool                   isInLassoRange(const te::MidiClip* clip, const tracktion_engine::MidiNote* midiNote);

    juce::Array<te::MidiNote*> getSelectedNotes();
    void                   setNoteSelected(te::MidiNote* n, bool addToSelection);
    bool                   isSelected(tracktion_engine::MidiNote* note);
    void                   unselectAll();
    bool                   areNotesDragged() const;

    bool                   isHovered(te::MidiNote* note);
    void                   setHovered(te::MidiNote* note, bool hovered);
    void                   moveSelectedNotesToTemp(const double startDelta, const double lengthDelta, bool copy=false);

    void                   cleanUpFlags();


    juce::MouseCursor getRecommendedMouseCursor();
    Tool                                        m_toolMode {Tool::pointer};

    EditViewState&                              m_evs;
    te::Track::Ptr                              m_track;
    TimeLineComponent&                          m_timeLine;
    LassoSelectionTool                          m_lassoTool;
    te::MidiNote*                               m_clickedNote {nullptr};
    std::unique_ptr<te::SelectedMidiEvents>     m_selectedEvents;
    double                                      m_clickedKey{0.0};
    te::MidiClip*                               m_clickedClip{nullptr};
    juce::OwnedArray<MidiNote>                  m_temp;
    double                                      m_draggedTimeDelta {0.0};
    int                                         m_draggedNoteDelta {0};

    double                                      m_hoveredTime;
    te::MidiNote*                               m_hoveredNote {nullptr};

    double                                      m_leftTimeDelta{0.0};
    double                                      m_rightTimeDelta{0.0};
    bool m_expandLeft {false}, m_expandRight {false}, m_noteAdding {false}, m_snap {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiViewport)

};
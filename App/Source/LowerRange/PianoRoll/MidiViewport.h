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
#include "Utilities/EditViewState.h"
#include "Tools/tools/LassoSelectionTool.h"
#include "SongEditor/TimeLineComponent.h"
#include "Tools/ToolStrategy.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class ToolStrategy;

class MidiViewport
    : public juce::Component
    , public juce::ChangeBroadcaster
    , public juce::Timer
    , private juce::ValueTree::Listener
{
public:
    MidiViewport(EditViewState &, te::Track::Ptr, TimeLineComponent &timeLine);
    ~MidiViewport() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;

    void timerCallback() override;

    te::Track::Ptr getTrack();
    TimeLineComponent *getTimeLine() { return &m_timeLine; }

    float getCursorScale() { return m_evs.m_applicationState.m_mouseCursorScale; }

    tracktion::MidiClip *getClipAt(int x);
    te::MidiNote *getNoteByPos(juce::Point<float> pos);
    int getNoteNumber(int y);

    void setTool(Tool tool);
    ToolStrategy *getCurrentTool() { return m_currentTool.get(); }
    Tool getCurrentToolType() { return m_currentTool->getToolId(); }
    void setSnap(bool snap) { m_snap = snap; }
    bool isSnapping() { return m_snap; }
    void setClickedNote(te::MidiNote *note) { m_clickedNote = note; }
    void setClickedClip(te::MidiClip *clip) { m_clickedClip = clip; }
    te::MidiNote *getClickedNote() const { return m_clickedNote; }
    te::MidiClip *getClickedClip() const { return m_clickedClip; }

    juce::Array<te::MidiNote *> getSelectedNotes();
    void updateSelectedEvents();
    void duplicateSelectedNotes();
    void deleteSelectedNotes();
    te::SelectedMidiEvents &getSelectedEvents();
    const juce::Array<te::MidiClip *> &getCachedMidiClips();
    juce::Rectangle<float> getNoteRect(tracktion_engine::MidiClip *const &midiClip, const tracktion_engine::MidiNote *n);
    void setNoteSelected(te::MidiNote *n, bool addToSelection);
    bool isSelected(tracktion_engine::MidiNote *note);
    void unselectAll();
    void cleanUpFlags();

    // Methods needed by Tools and other components
    void cleanUnderNote(int noteNumb, tracktion::BeatRange beatRange, const te::MidiClip *clip);
    te::MidiNote *addNewNoteAt(int x, int y, te::MidiClip *clip);
    te::MidiNote *addNewNote(int noteNumb, const te::MidiClip *clip, double beat, double length = -1);
    void playGuideNote(const te::MidiClip *clip, const int noteNumb, int vel = 100);
    double getKeyForY(int y);
    int getYForKey(double key);
    void startLasso(const juce::MouseEvent &e, bool isRangeTool = false);
    void updateLasso(const juce::MouseEvent &e);
    void stopLasso();

private:
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;

    void drawClipRange(juce::Graphics &g, tracktion_engine::MidiClip *const &midiClip);
    void drawNote(juce::Graphics &g, tracktion_engine::MidiClip *const &midiClip, te::MidiNote *n);
    void drawDraggedNotes(juce::Graphics &g, te::MidiNote *n, te::MidiClip *clip);
    void drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour);
    void drawKeyLines(juce::Graphics &g) const;
    void drawKeyNum(juce::Graphics &g, const tracktion_engine::MidiNote *n, juce::Rectangle<float> &noteRect) const;

    float getStartKey() const;
    float getKeyWidth() const;

    juce::Rectangle<float> getNoteRect(int noteNum, int x1, int x2) const;

    juce::Colour getNoteColour(tracktion_engine::MidiClip *const &midiClip, tracktion_engine::MidiNote *n);

    juce::Array<te::MidiNote *> getNotesInRange(juce::Range<double> beatRange, const te::MidiClip *clip);
    void removeNote(te::MidiClip *clip, te::MidiNote *note);
    static float getVelocity(const tracktion_engine::MidiNote *note);

    te::MidiClip *getMidiClipAt(int x);
    te::MidiClip *getNearestClipAfter(int x);
    te::MidiClip *getNearestClipBefore(int x);
    juce::Rectangle<float> getClipRect(te::Clip *clip);

    void snapToGrid(te::MidiNote *note, const te::MidiClip *clip) const;
    void scrollPianoRoll(float delta);

    void updateLassoSelection();
    juce::Range<double> getLassoVerticalKeyRange();
    bool isInLassoRange(const te::MidiClip *clip, const tracktion_engine::MidiNote *midiNote);

    bool isHovered(te::MidiNote *note);
    void setHovered(te::MidiNote *note, bool hovered);

    void refreshClipCache();
    void invalidateClipCache();

    EditViewState &m_evs;
    std::unique_ptr<ToolStrategy> m_currentTool;

    te::Track::Ptr m_track;
    TimeLineComponent &m_timeLine;
    LassoSelectionTool m_lassoTool;

    // states
    te::MidiNote *m_clickedNote{nullptr};
    te::MidiClip *m_clickedClip{nullptr};
    std::unique_ptr<te::SelectedMidiEvents> m_selectedEvents;
    bool m_snap{false};
    te::MidiNote *m_hoveredNote{nullptr};

    bool m_expandLeft{false}, m_expandRight{false}, m_noteAdding{false};

    // Cached clips for performance optimization
    juce::Array<te::MidiClip *> m_cachedClips;
    bool m_clipCacheValid{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiViewport)
};

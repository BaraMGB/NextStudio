
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <utility>

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


class VelocityEditor : public juce::Component
{
public:
    VelocityEditor(EditViewState& evs, te::Track::Ptr t, juce::String timeLineID)
        : m_editViewState(evs)
        , m_track(t)
        , m_timeLineID(timeLineID)
    {
    }
    ~VelocityEditor() override
    {
    }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

private:

    std::vector<tracktion_engine::MidiClip*> getMidiClipsOfTrack();

    tracktion_engine::MidiNote * getNote(juce::Point<int> p);

    void drawBarsAndBeatLines(juce::Graphics& g, juce::Colour colour);
    double getNoteStartBeat(te::MidiClip* const& pClip,
                            const te::MidiNote* pNote) const;
    double getNoteEndBeat(te::MidiClip* const& pClip,
                          const te::MidiNote* pNote) const;
    juce::Range<int> getXLineRange(te::MidiClip* const& midiClip, const te::MidiNote* n) const;

    int getVelocityPixel(const te::MidiNote* n) const;

    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    void drawVelocityRuler(juce::Graphics& graphics,
                           tracktion_engine::MidiClip*& midiClip,
                           tracktion_engine::MidiNote* n);
    int beatsToX (double beats);
    int getVelocity(int y);
    void clearNotesFlags();
    te::MidiNote* getHoveredNote();
    int m_cachedVelocity;
    juce::String m_timeLineID;
};


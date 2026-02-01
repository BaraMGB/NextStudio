
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

#include <utility>

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

class VelocityEditor : public juce::Component
{
public:
    VelocityEditor(EditViewState &evs, te::Track::Ptr t, juce::String timeLineID)
        : m_editViewState(evs),
          m_track(t),
          m_timeLineID(timeLineID)
    {
    }
    ~VelocityEditor() override {}

    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;

private:
    EditViewState &m_editViewState;
    te::Track::Ptr m_track;

    tracktion_engine::MidiNote *getNote(juce::Point<float> p);

    void drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour);
    juce::Range<float> getXLineRange(te::MidiClip *const &midiClip, const te::MidiNote *n) const;
    int getVelocityPixel(const te::MidiNote *n) const;

    void drawVelocityRuler(juce::Graphics &graphics, tracktion_engine::MidiClip *&midiClip, tracktion_engine::MidiNote *n);
    int getVelocity(int y);
    void clearNotesFlags();
    te::MidiNote *getHoveredNote();
    int m_cachedVelocity;
    juce::String m_timeLineID;
};

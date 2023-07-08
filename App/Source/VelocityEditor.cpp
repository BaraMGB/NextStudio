
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


#include "VelocityEditor.h"
void VelocityEditor::paint(juce::Graphics& g)
{
    drawBarsAndBeatLines(g, juce::Colour(0x77ffffff));
    g.setColour(juce::Colour(0x77ffffff));

    for (auto & midiClip : getMidiClipsOfTrack())
    {
        auto& seq = midiClip->getSequence();
        for (auto n : seq.getNotes())
        {
            drawVelocityRuler(g, midiClip, n);
        }
    }
}

void VelocityEditor::mouseDown(const juce::MouseEvent&)
{
    if (auto n = getHoveredNote())
    {
        m_cachedVelocity = n->getVelocity();
    }

}

void VelocityEditor::mouseDrag(const juce::MouseEvent&e)
{
    if (auto n = getHoveredNote())
    {
        auto v = juce::jlimit(0, 127
                              , m_cachedVelocity - e.getDistanceFromDragStartY());
        n->setVelocity(v, &m_editViewState.m_edit.getUndoManager());
        m_editViewState.m_lastVelocity = v;
        repaint();
    }
}

void VelocityEditor::mouseMove(const juce::MouseEvent& e)
{
    clearNotesFlags();
    if (auto note = getNote(e.position.toInt()))
    {
        note->state.setProperty(
            IDs::isHovered, true, &m_editViewState.m_edit.getUndoManager());
    }
    repaint();
}

void VelocityEditor::mouseExit(const juce::MouseEvent&)
{
    clearNotesFlags();
    repaint();
}

void VelocityEditor::mouseUp(const juce::MouseEvent&)
{

}

void VelocityEditor::mouseWheelMove(const juce::MouseEvent& event,
                                    const juce::MouseWheelDetails& wheel)
{

}

void VelocityEditor::drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour)
{
    g.setColour(colour);
    double x1 = m_editViewState.m_pianoX1;
    double x2 = m_editViewState.m_pianoX2;
    GUIHelpers::drawBarsAndBeatLines (g, m_editViewState, x1, x2, getBounds ());
}

double VelocityEditor::getNoteStartBeat(te::MidiClip* const& midiClip,
                                        const te::MidiNote* n) const
{
    auto sBeat= n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat.inBeats();
}

double VelocityEditor::getNoteEndBeat(te::MidiClip* const& midiClip,
                                      const te::MidiNote* n) const
{
    auto eBeat= n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat.inBeats();
}
std::vector<tracktion_engine::MidiClip*> VelocityEditor::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        for (auto c : at->getClips ())
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
            {
                midiClips.push_back (mc);
            }
        }
    }
    return midiClips;
}
void VelocityEditor::drawVelocityRuler(juce::Graphics& g,
                                       tracktion_engine::MidiClip*& midiClip,
                                       tracktion_engine::MidiNote* n)
{
    auto noteRangeX = getXLineRange(midiClip, n);
    auto velocityY = getVelocityPixel(n);

    g.setColour(juce::Colour(0xff666666));

    g.fillRect(juce::Rectangle<int>(
        noteRangeX.getStart() - 1,velocityY, 2,getHeight() - velocityY));
    g.setColour(juce::Colour(0xff181818));
    g.fillEllipse(noteRangeX.getStart() - 3, velocityY - 3, 6, 6);
    if (n->state.getPropertyAsValue(IDs::isHovered, &m_editViewState.m_edit.getUndoManager(), false) == true)
    {
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(m_track->getColour());
    }
    g.drawEllipse(noteRangeX.getStart() - 3, velocityY - 3, 6, 6, 2);
}

juce::Range<int> VelocityEditor::getXLineRange(te::MidiClip* const& midiClip,
                                                       const te::MidiNote* n) const
{
    double sBeat = getNoteStartBeat(midiClip, n);
    double eBeat = getNoteEndBeat(midiClip, n);

    auto x1 = m_editViewState.beatsToX(sBeat + midiClip->getStartBeat().inBeats(),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2);
    auto x2 = m_editViewState.beatsToX(eBeat + midiClip->getStartBeat().inBeats(),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2) + 1;

    return {x1, x2};

}

int VelocityEditor::getVelocity(int y)
{
    return juce::jmap((getHeight() - 4)  - y,0 , getHeight() - 8, 0, 127);
}

int VelocityEditor::getVelocityPixel(const te::MidiNote* n) const
{
    return (getHeight() - 4) - juce::jmap(n->getVelocity(), 0, 127, 0, getHeight() - 8 );
}

tracktion_engine::MidiNote* VelocityEditor::getNote(juce::Point<int> p)
{
    for (auto& mc : getMidiClipsOfTrack ())
    {
        for (auto note: mc->getSequence().getNotes())
        {
            auto y = getVelocityPixel(note);
            auto x = beatsToX(getNoteStartBeat(mc, note) + mc->getStartBeat().inBeats());

            if (GUIHelpers::getSensibleArea(p, 10).contains(x,y))
            {
                return note;
            }
        }
    }
    return nullptr;
}

int VelocityEditor::beatsToX(double beats)
{
    return m_editViewState.beatsToX(
        beats, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
void VelocityEditor::clearNotesFlags()
{
    for (auto mc : getMidiClipsOfTrack())
    {
        for (auto n : mc->getSequence().getNotes())
        {
            n->state.setProperty(
                IDs::isHovered, false, &m_editViewState.m_edit.getUndoManager());
        }
    }
}
te::MidiNote* VelocityEditor::getHoveredNote()
{
    for (auto mc : getMidiClipsOfTrack())
    {
        for (auto n : mc->getSequence().getNotes())
        {
            if (n->state.getProperty(IDs::isHovered, false))
                return n;
        }
    }
    return nullptr;
}

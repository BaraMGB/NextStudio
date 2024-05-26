
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

#include "LowerRangeComponent.h"
#include "Utilities.h"


SplitterComponent::SplitterComponent(EditViewState &evs) : m_editViewState(evs)
{

}

void SplitterComponent::mouseMove(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    m_isHovering = true;
    repaint ();
}

void SplitterComponent::mouseEnter(const juce::MouseEvent &)
{
}

void SplitterComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    m_isHovering = false;
    repaint ();
}

void SplitterComponent::mouseDown(const juce::MouseEvent &)
{
    m_pianorollHeightAtMousedown = m_editViewState.m_midiEditorHeight;
    m_cachedPianoNoteNum = (double) m_editViewState.m_pianoStartKey;
}

void SplitterComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_editViewState.m_isPianoRollVisible)
    {
        auto newHeight = static_cast<int> (m_pianorollHeightAtMousedown
                                        - event.getDistanceFromDragStartY());
        auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
        auto noteDist = event.getDistanceFromDragStartY () / noteHeight;

        m_editViewState.m_pianoStartKey =
                juce::jlimit(0.0
                           , 127.0 - (getHeight ()
                                / m_editViewState.m_pianoKeyWidth)
                           , m_cachedPianoNoteNum + noteDist);
        m_editViewState.m_midiEditorHeight = std::max(20, newHeight);
    }
}

void SplitterComponent::mouseUp(const juce::MouseEvent &)
{
}

void SplitterComponent::paint(juce::Graphics &g)
{
    if (m_isHovering)
    {
        g.setColour(juce::Colours::navy);
        g.fillRect (getLocalBounds ());
    }
}



//------------------------------------------------------------------------------

LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : m_evs(evs)
    , m_rackView(evs)
    , m_pianoRollEditor (evs)
    , m_splitter (evs)
{
    m_evs.m_isPianoRollVisible = false;
    addAndMakeVisible (m_splitter);
    addChildComponent (m_pianoRollEditor);
    addAndMakeVisible (m_rackView);
    m_evs.m_edit.state.addListener (this);
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_evs.m_edit.state.removeListener (this);
}

void LowerRangeComponent::paint(juce::Graphics &g)
{
    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(rect.removeFromBottom(getHeight() - (int) m_splitterHeight).toFloat());
}

void LowerRangeComponent::paintOverChildren(juce::Graphics &g)
{
    
    auto area = getLocalBounds ();
    area.removeFromTop(m_splitterHeight);

    GUIHelpers::drawFakeRoundCorners(g, area.toFloat(), m_evs.m_applicationState.getMainFrameColour(), m_evs.m_applicationState.getBorderColour());
}

void LowerRangeComponent::resized()
{
        auto area = getLocalBounds();
        auto splitter = area.removeFromTop ((int) m_splitterHeight);
        splitter.reduce(10, 1); 

        m_splitter.setBounds (splitter);

        m_rackView.setBounds(area);

        if (m_pianoRollEditor.isVisible ())
        {
            m_pianoRollEditor.setBounds (area);
        }
}

void LowerRangeComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
        if (v.hasType(te::IDs::TRACK) || v.hasType(te::IDs::FOLDERTRACK))
        {
            GUIHelpers::log("LowerRangeComponent valueTreePropertyChanged --------------- ");
            if (auto track = te::findTrackForState (m_evs.m_edit, v))
            {
                GUIHelpers::log("LowerRangeComponent valueTreePropertyChanged ", track->getName());
                if ( (bool) v.getProperty(IDs::showLowerRange) == true)
                {
                    if (m_evs.m_isPianoRollVisible)
                    {
                        m_rackView.setVisible(false);
                        m_pianoRollEditor.setTrack(track);
                        m_pianoRollEditor.setVisible(true);
                    }
                    else
                    {
                        m_pianoRollEditor.setVisible(false);
                        m_rackView.setTrack(track);
                        m_rackView.setVisible(true);
                        m_rackView.repaint();
                    }
                }
            }
            
        }
        if (v.hasType (tracktion_engine::IDs::MIDICLIP))
        {
            resized ();
            repaint ();
        }
        if (v.hasType (IDs::EDITVIEWSTATE))
        {
            if (i == IDs::pianoX1
                || i == IDs::pianoX2
                || i == IDs::pianoY1
                || i == IDs::pianorollNoteWidth)
            {
                resized ();
                repaint ();
            }
        }
}

void LowerRangeComponent::valueTreeChildAdded(juce::ValueTree &v, juce::ValueTree &i)
{
    if (!m_evs.m_isPianoRollVisible && v.hasType(te::IDs::TRACK))
    {
        auto track = te::findTrackForState (m_evs.m_edit, v);
        if (track != nullptr)
        {
            m_rackView.setTrack(track);
            m_rackView.setVisible(true);
        }
    }
    resized ();
    repaint ();
}

void LowerRangeComponent::valueTreeChildRemoved(juce::ValueTree &v, juce::ValueTree &i, int)
{
    if (i.getProperty(te::IDs::id).toString() == m_rackView.getCurrentTrackID())
        m_rackView.clearTrack();

    resized ();
    repaint ();
}

void LowerRangeComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int)
{
    resized ();
    repaint ();
}

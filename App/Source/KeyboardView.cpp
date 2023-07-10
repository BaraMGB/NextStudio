
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

#include "KeyboardView.h"
#include "Utilities.h"

void KeyboardView::mouseDown(const juce::MouseEvent& e)
{
    if (m_keyboard.getBounds().contains(e.getPosition()))
    {
        EngineHelpers::updateMidiInputs(m_editViewState, m_track);
        EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, getKey(e.y), .8f));
    }

    m_keyWidthCached = m_editViewState.m_pianoKeyWidth;
    m_clickedKey = getKey(e.y);
}
void KeyboardView::mouseDrag(const juce::MouseEvent& e)
{

    if (m_keyboard.getBounds().contains(e.getPosition()))
    {
        auto keyLenght = m_keyboard.getRectangleForKey((int)getKey(e.y)).getWidth();
        auto KeyboardX = m_keyboard.getX();
        if (((int) getKey(e.y) != (int) m_clickedKey) && (e.x < KeyboardX + keyLenght))
        {
            EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit).handleIncomingMidiMessage(juce::MidiMessage::noteOff(0, m_clickedKey));
            m_clickedKey = getKey(e.y);
            EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, m_clickedKey, .8f));
        }
    }
    else
    {
        auto visibleKeys = (float) (getHeight() / m_keyWidthCached);

        auto scaleFactor = GUIHelpers::getZoomScaleFactor(e.getDistanceFromDragStartX(), m_editViewState.getTimeLineZoomUnit());
        auto scaledVisibleKeys = juce::jlimit(12.f , 64.f, visibleKeys * scaleFactor);

        auto currentYtoKeys = juce::jmap((float) getHeight() - e.position.y, 0.f , (float) getHeight(), 0.f , scaledVisibleKeys);

        auto newStartKey = juce::jmax(0.f, m_clickedKey - currentYtoKeys);
        auto maxKeyHeight = (float) getHeight() / (float) (127.f - newStartKey);
        auto newKeyWidth = juce::jmax((float) getHeight() / scaledVisibleKeys, maxKeyHeight);

        m_editViewState.m_pianoStartKey = newStartKey;
        m_editViewState.m_pianoKeyWidth = newKeyWidth;
    }

}
void KeyboardView::mouseUp (const juce::MouseEvent& e) 
{
    EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit).handleIncomingMidiMessage(juce::MidiMessage::noteOff(0, m_clickedKey));
}

void KeyboardView::resized() 
{
    double firstVisibleNote = m_editViewState.m_pianoStartKey;
    double pianoRollNoteWidth = m_editViewState.m_pianoKeyWidth;

    m_keyboard.setKeyWidth (juce::jmax(0.1f, (float) pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (getWidth() - 50
                         , (getHeight () - (int) m_keyboard.getTotalKeyboardWidth ()
                          + (int) (firstVisibleNote * pianoRollNoteWidth)) + 2
                         , 50
                         , (int) m_keyboard.getTotalKeyboardWidth ());
}
float KeyboardView::getKey(int y)
{
    auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto noteNumb =
        static_cast<float>(m_editViewState.m_pianoStartKey
                           + ((getHeight() - y) / noteHeight));
    return noteNumb;
}

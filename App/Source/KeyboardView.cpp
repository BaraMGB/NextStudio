
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
        if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, getKey(e.y), .8f), 0);
    }

    m_keyWidthCached = m_editViewState.getViewYScale(m_timeLineID);
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
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit))
            {
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, m_clickedKey),0);
                m_clickedKey = getKey(e.y);
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, m_clickedKey, .8f),0);
            }
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

        m_editViewState.setYScroll(m_timeLineID, newStartKey);
        m_editViewState.setViewYScale(m_timeLineID,  newKeyWidth);
    }

}
void KeyboardView::mouseUp (const juce::MouseEvent& e) 
{
    if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit))
        virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, m_clickedKey),0);
}

void KeyboardView::resized() 
{
    double firstVisibleNote = m_editViewState.getViewYScroll(m_timeLineID);
    double pianoRollNoteWidth = m_editViewState.getViewYScale(m_timeLineID);
    double totalHeight = m_keyboard.getTotalKeyboardWidth();
    auto w = 50;
    auto x = getWidth() - w;
    auto y = (getHeight () - totalHeight + (firstVisibleNote * pianoRollNoteWidth)) + 2;

    m_keyboard.setKeyWidth (juce::jmax(0.1, pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (x, y, w, totalHeight);
}
float KeyboardView::getKey(int y)
{
    auto noteHeight = (double) m_editViewState.getViewYScale(m_timeLineID);
    auto noteNumb =
        static_cast<float>(m_editViewState.getViewYScroll(m_timeLineID)
                           + ((getHeight() - y) / noteHeight));
    return noteNumb;
}

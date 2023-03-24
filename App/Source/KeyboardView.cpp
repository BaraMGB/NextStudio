#include "KeyboardView.h"
#include "Utilities.h"

void KeyboardView::mouseDown(const juce::MouseEvent& e)
{
    if (m_keyboard.getBounds().contains(e.getPosition()))
        EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, getKey(e.y), .8f));
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
            EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine).handleIncomingMidiMessage(juce::MidiMessage::noteOff(0, m_clickedKey));
            m_clickedKey = getKey(e.y);
            EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, m_clickedKey, .8f));
        }
    }
    else
    {
        auto visibleKeys = (float) (getHeight() / m_keyWidthCached);

        auto scaleFactor = GUIHelpers::getZoomScaleFactor(
            e.getDistanceFromDragStartX(), m_editViewState.getTimeLineZoomUnit());
        auto scaledVisibleKeys = juce::jlimit(12.f , 64.f
                                              , visibleKeys * scaleFactor);

        auto currentYtoKeys = juce::jmap(
            (float) getHeight() - e.position.y
            , 0.f , (float) getHeight()
                , 0.f , scaledVisibleKeys);

        auto newStartKey = juce::jmax(0.f, m_clickedKey - currentYtoKeys);
        auto maxKeyHeight = (float) getHeight() / (float) (127.f - newStartKey);
        auto newKeyWidth = juce::jmax((float) getHeight() / scaledVisibleKeys, maxKeyHeight);

        m_editViewState.m_pianoStartKey = newStartKey;
        m_editViewState.m_pianoKeyWidth = newKeyWidth;
    }

}
void KeyboardView::mouseUp (const juce::MouseEvent& e) 
{
    EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine).handleIncomingMidiMessage(juce::MidiMessage::noteOff(0, m_clickedKey));
}

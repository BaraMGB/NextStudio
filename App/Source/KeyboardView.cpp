#include "KeyboardView.h"

void KeyboardView::mouseDown(const juce::MouseEvent& e)
{
    m_keyWidthCached = m_editViewState.m_pianoKeyWidth;
    m_clickedKey = getKey(e.y);
}
void KeyboardView::mouseDrag(const juce::MouseEvent& e)
{
    auto visibleKeys = (float) (getHeight() / m_keyWidthCached);

    auto scaleFactor = GUIHelpers::getScaleFactor(e.getDistanceFromDragStartX()
                                                      , m_editViewState.getTimeLineZoomUnit());
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

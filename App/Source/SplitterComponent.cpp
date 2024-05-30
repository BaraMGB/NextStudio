#include "SplitterComponent.h"
SplitterComponent::SplitterComponent(bool horizontal)
    :  m_isHorizontal(horizontal)
{
}

void SplitterComponent::mouseMove(const juce::MouseEvent &)
{
    setMouseCursor(m_isHorizontal ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
    m_isHovering = true;
    repaint();
}

void SplitterComponent::mouseEnter(const juce::MouseEvent &)
{
}

void SplitterComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    m_isHovering = false;
    repaint();
}

void SplitterComponent::mouseDown(const juce::MouseEvent &)
{
    if (onMouseDown)
    {
        onMouseDown();
    }
}

void SplitterComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (onDrag)
    {
        if (m_isHorizontal)
        {
            onDrag(event.getDistanceFromDragStartY());
        }
        else
        {
            onDrag(event.getDistanceFromDragStartX());
        }
    }
}

void SplitterComponent::mouseUp(const juce::MouseEvent &)
{
}

void SplitterComponent::paint(juce::Graphics &g)
{
    if (m_isHovering)
    {
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.7f));
        g.fillRect(getLocalBounds());
    }
}


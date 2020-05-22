/*
  ==============================================================================

    SpinBox.cpp
    Created: 26 Jan 2020 8:05:27pm
    Author:  BaraMGB

  ==============================================================================
*/

#include "SpinBox.h"

SpinBox::SpinBox(int digits, int init, int min, int max, juce::Font font, Colour fontColour, String separator, int step)
    : m_digits(digits)
    , m_value(init)
    , m_min(min)
    , m_max(max)
    , m_step(step)
    , m_neededWidth(10)
    , m_separator(separator)
    , m_overrun(false)
    , m_underrun(false)
    , m_dragging(false)
    , m_textColour(fontColour)
    , m_font(font)
{
}

SpinBox::~SpinBox()
{
}

void SpinBox::mouseEnter(const MouseEvent &/*event*/)
{
    setMouseCursor(MouseCursor::UpDownResizeCursor);
}

void SpinBox::mouseDown(const MouseEvent &event)
{
    m_mouseDown = event.getScreenPosition();
}

void SpinBox::mouseDrag(const MouseEvent &event)
{
    m_dragging = true;
    setMouseCursor(MouseCursor::NoCursor);
    int dy = event.getDistanceFromDragStartY();
    if (event.mods.isShiftDown())
    {
    }
    auto value = m_value - dy/2  * m_step;
    setValue(value);
    Desktop::setMousePosition(m_mouseDown);
}

void SpinBox::mouseUp(const MouseEvent &/*event*/)
{
    m_dragging = false;
    setMouseCursor(MouseCursor::UpDownResizeCursor);
}

void SpinBox::mouseExit(const MouseEvent &/*event*/)
{
    setMouseCursor(MouseCursor::NormalCursor);
}

void SpinBox::paint(Graphics &g)
{
    auto area = getLocalBounds();
    g.setColour(m_textColour);
    auto textArea = area.reduced(0, area.getHeight() / 4);

    String value_withBeginningZeros = "";
    int missingDigits = m_digits - String(m_value).length();
    for (auto i = 0; i < missingDigits; i++)
    {
        value_withBeginningZeros = value_withBeginningZeros + "0";
    }
    value_withBeginningZeros = value_withBeginningZeros + String(m_value);

    g.setFont(m_font);
    String valueStr = value_withBeginningZeros + m_separator;
    g.drawText(valueStr, textArea.getX(), textArea.getY(), textArea.getWidth(),
        textArea.getHeight(), Justification::centred, false);
}

void SpinBox::resized()
{
}

void SpinBox::setValue(int value)
{
    if (value < m_min)
    {
        m_overrun = false;
        m_underrun = true;
        sendChangeMessage();
    }
    else if (value > m_max)
    {
        m_overrun = true;
        m_underrun = false;
        sendChangeMessage();
    }
    else
    {
        m_value = value;
    }
    repaint();
}

int SpinBox::getValue()
{
    return m_value;
}

bool SpinBox::overrun() const
{
    return m_overrun;
}

bool SpinBox::underrun() const
{
    return m_underrun;
}

int SpinBox::getMin() const
{
    return m_min;
}

int SpinBox::getMax() const
{
    return m_max;
}

int SpinBox::getStep() const
{
    return m_step;
}

void SpinBox::setTextColour(const Colour &textColour)
{
    m_textColour = textColour;
}

bool SpinBox::isDragging() const
{
    return m_dragging;
}

int SpinBox::getNeededWidth() const
{
    return m_font.getStringWidth("0") * m_digits + m_font.getStringWidth(m_separator);
}






#include "InlineColourEditor.h"

#include <cmath>

juce::String InlineColourEditor::formatHexColour(juce::Colour colour)
{
    return "#" + juce::String::toHexString((int)colour.getRed()).toUpperCase().paddedLeft('0', 2)
           + juce::String::toHexString((int)colour.getGreen()).toUpperCase().paddedLeft('0', 2)
           + juce::String::toHexString((int)colour.getBlue()).toUpperCase().paddedLeft('0', 2);
}

bool InlineColourEditor::parseHexColour(const juce::String &text, juce::Colour &result)
{
    auto value = text.trim();
    if (value.startsWithChar('#'))
        value = value.substring(1);

    if (value.length() != 6 || value.containsOnly("0123456789abcdefABCDEF") == false)
        return false;

    const auto rgb = (juce::uint32)value.getHexValue32();
    result = juce::Colour((juce::uint8)((rgb >> 16) & 0xff),
                          (juce::uint8)((rgb >> 8) & 0xff),
                          (juce::uint8)(rgb & 0xff));
    return true;
}

InlineColourEditor::InlineColourEditor()
{
    setWantsKeyboardFocus(true);

    m_hexLabel.setText("Hex", juce::dontSendNotification);
    m_hexLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_hexLabel);

    m_hexEditor.setMultiLine(false);
    m_hexEditor.setJustification(juce::Justification::centredLeft);
    m_hexEditor.setInputRestrictions(7, "#0123456789abcdefABCDEF");
    m_hexEditor.onReturnKey = [this]
    {
        commitHexText();
        m_hexEditor.giveAwayKeyboardFocus();
    };
    m_hexEditor.onEscapeKey = [this]
    {
        m_hexIsValid = true;
        updateControls();
        m_hexEditor.giveAwayKeyboardFocus();
    };
    m_hexEditor.onFocusLost = [this] { commitHexText(); };
    addAndMakeVisible(m_hexEditor);

    m_previewLabel.setText("Preset / Current", juce::dontSendNotification);
    m_previewLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_previewLabel);

    m_resetButton.onClick = [this]
    {
        if (onReset)
            onReset();
    };
    addAndMakeVisible(m_resetButton);

    updateControls();
}

void InlineColourEditor::setCurrentColour(juce::Colour colour, juce::NotificationType notification)
{
    if (m_colour == colour)
        return;

    m_colour = colour.withAlpha(1.0f);
    const auto saturation = m_colour.getSaturation();
    if (saturation > 0.0001f)
        m_hue = colour.getHue();
    m_saturation = saturation;
    m_value = colour.getBrightness();
    updateControls();
    repaint();

    if (notification != juce::dontSendNotification && onColourChanged)
        onColourChanged(m_colour);
}

void InlineColourEditor::setReferenceColour(juce::Colour colour)
{
    m_referenceColour = colour.withAlpha(1.0f);
    repaint();
}

void InlineColourEditor::setThemeColours(juce::Colour background, juce::Colour text, juce::Colour border, juce::Colour accent)
{
    m_background = background;
    m_text = text;
    m_border = border;
    m_accent = accent;

    m_hexLabel.setColour(juce::Label::textColourId, text);
    m_previewLabel.setColour(juce::Label::textColourId, text);
    m_hexEditor.setColour(juce::TextEditor::backgroundColourId, background.darker(0.15f));
    m_hexEditor.setColour(juce::TextEditor::textColourId, text);
    m_hexEditor.setColour(juce::TextEditor::highlightColourId, accent);
    m_hexEditor.setColour(juce::TextEditor::focusedOutlineColourId, accent);
    repaint();
}

void InlineColourEditor::paint(juce::Graphics &g)
{
    g.setColour(m_background.brighter(0.04f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(hasKeyboardFocus(false) ? m_accent : m_border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, hasKeyboardFocus(false) ? 1.5f : 1.0f);

    rebuildSaturationValueImage();
    if (m_svImage.isValid())
        g.drawImage(m_svImage, m_svBounds);

    g.setColour(m_border);
    g.drawRect(m_svBounds, 1.0f);

    juce::ColourGradient hueGradient(juce::Colours::red, m_hueBounds.getX(), m_hueBounds.getY(), juce::Colours::red, m_hueBounds.getX(), m_hueBounds.getBottom(), false);
    for (int i = 1; i < 6; ++i)
        hueGradient.addColour((double)i / 6.0, juce::Colour::fromHSV((float)i / 6.0f, 1.0f, 1.0f, 1.0f));
    g.setGradientFill(hueGradient);
    g.fillRect(m_hueBounds);
    g.setColour(m_border);
    g.drawRect(m_hueBounds, 1.0f);

    const auto svMarker = juce::Point<float>(m_svBounds.getX() + m_saturation * m_svBounds.getWidth(),
                                             m_svBounds.getBottom() - m_value * m_svBounds.getHeight());
    g.setColour(m_colour.contrasting());
    g.drawEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre(svMarker), 2.0f);
    g.setColour(m_colour);
    g.drawEllipse(juce::Rectangle<float>(8.0f, 8.0f).withCentre(svMarker), 2.0f);

    const auto hueY = m_hueBounds.getY() + m_hue * m_hueBounds.getHeight();
    g.setColour(m_text);
    g.drawRect(juce::Rectangle<float>(m_hueBounds.getX() - 2.0f, hueY - 2.0f, m_hueBounds.getWidth() + 4.0f, 4.0f), 1.5f);

    const auto drawPreview = [&g, this](juce::Rectangle<float> bounds, juce::Colour colour)
    {
        g.setColour(colour);
        g.fillRect(bounds);
        g.setColour(m_border);
        g.drawRect(bounds, 1.0f);
    };

    drawPreview(m_referencePreviewBounds, m_referenceColour);
    drawPreview(m_currentPreviewBounds, m_colour);
}

void InlineColourEditor::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    auto picker = bounds.removeFromTop(126);
    m_hueBounds = picker.removeFromRight(18).toFloat();
    picker.removeFromRight(6);
    m_svBounds = picker.toFloat();

    bounds.removeFromTop(7);
    auto hexRow = bounds.removeFromTop(26);
    m_hexLabel.setBounds(hexRow.removeFromLeft(42));
    m_hexEditor.setBounds(hexRow);

    bounds.removeFromTop(5);
    auto previewRow = bounds.removeFromTop(26);
    m_previewLabel.setBounds(previewRow.removeFromLeft(96));
    m_resetButton.setBounds(previewRow.removeFromRight(62).reduced(1));
    m_currentPreviewBounds = previewRow.removeFromRight(26).reduced(2).toFloat();
    m_referencePreviewBounds = previewRow.removeFromRight(26).reduced(2).toFloat();

    m_svImage = {};
}

void InlineColourEditor::mouseDown(const juce::MouseEvent &event)
{
    if (m_svBounds.contains(event.position))
        m_keyboardEditsHue = false;
    else if (m_hueBounds.expanded(3.0f, 0.0f).contains(event.position))
        m_keyboardEditsHue = true;

    grabKeyboardFocus();
    updateFromPoint(event.position);
}

void InlineColourEditor::mouseDrag(const juce::MouseEvent &event)
{
    updateFromPoint(event.position);
}

bool InlineColourEditor::keyPressed(const juce::KeyPress &key)
{
    constexpr float step = 0.01f;

    if (m_keyboardEditsHue && (key == juce::KeyPress::upKey || key == juce::KeyPress::leftKey || key == juce::KeyPress::downKey || key == juce::KeyPress::rightKey))
    {
        const auto direction = (key == juce::KeyPress::upKey || key == juce::KeyPress::leftKey) ? -step : step;
        m_hue = std::fmod(m_hue + direction + 1.0f, 1.0f);
        m_svImage = {};
    }
    else if (!m_keyboardEditsHue && (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey || key == juce::KeyPress::upKey || key == juce::KeyPress::downKey))
    {
        if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
            m_saturation = juce::jlimit(0.0f, 1.0f, m_saturation + (key == juce::KeyPress::leftKey ? -step : step));
        else
            m_value = juce::jlimit(0.0f, 1.0f, m_value + (key == juce::KeyPress::downKey ? -step : step));
    }
    else if (key == juce::KeyPress::returnKey)
    {
        focusHexEditor();
        return true;
    }
    else
    {
        return false;
    }

    m_colour = colourForCurrentHsv();
    notifyColourChanged();
    return true;
}

void InlineColourEditor::focusHexEditor()
{
    m_hexEditor.grabKeyboardFocus();
    m_hexEditor.selectAll();
}

void InlineColourEditor::updateFromPoint(juce::Point<float> point)
{
    if (m_svBounds.contains(point))
    {
        m_saturation = juce::jlimit(0.0f, 1.0f, (point.x - m_svBounds.getX()) / m_svBounds.getWidth());
        m_value = juce::jlimit(0.0f, 1.0f, 1.0f - (point.y - m_svBounds.getY()) / m_svBounds.getHeight());
    }
    else if (m_hueBounds.expanded(3.0f, 0.0f).contains(point))
    {
        m_hue = juce::jlimit(0.0f, 1.0f, (point.y - m_hueBounds.getY()) / m_hueBounds.getHeight());
        m_svImage = {};
    }
    else
    {
        return;
    }

    m_colour = colourForCurrentHsv();
    updateControls();
    notifyColourChanged();
}

void InlineColourEditor::updateControls()
{
    if (!m_hexEditor.hasKeyboardFocus(true) || m_hexIsValid)
        m_hexEditor.setText(formatHexColour(m_colour), juce::dontSendNotification);
    m_hexEditor.setColour(juce::TextEditor::outlineColourId, m_hexIsValid ? m_border : juce::Colours::red);
}

void InlineColourEditor::commitHexText()
{
    juce::Colour parsed;
    m_hexIsValid = parseHexColour(m_hexEditor.getText(), parsed);

    if (m_hexIsValid)
    {
        setCurrentColour(parsed, juce::dontSendNotification);
        notifyColourChanged();
    }
    else
    {
        updateControls();
        m_hexEditor.repaint();
    }
}

void InlineColourEditor::notifyColourChanged()
{
    m_hexIsValid = true;
    updateControls();
    repaint();

    if (onColourChanged)
        onColourChanged(m_colour);
}

void InlineColourEditor::rebuildSaturationValueImage()
{
    const auto width = juce::jmax(1, (int)m_svBounds.getWidth());
    const auto height = juce::jmax(1, (int)m_svBounds.getHeight());
    if (m_svImage.isValid() && m_svImage.getWidth() == width && m_svImage.getHeight() == height && juce::approximatelyEqual(m_imageHue, m_hue))
        return;

    m_svImage = juce::Image(juce::Image::RGB, width, height, false);
    m_imageHue = m_hue;
    juce::Image::BitmapData pixels(m_svImage, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < height; ++y)
    {
        const auto value = 1.0f - (float)y / (float)juce::jmax(1, height - 1);
        for (int x = 0; x < width; ++x)
        {
            const auto saturation = (float)x / (float)juce::jmax(1, width - 1);
            pixels.setPixelColour(x, y, juce::Colour::fromHSV(m_hue, saturation, value, 1.0f));
        }
    }
}

juce::Colour InlineColourEditor::colourForCurrentHsv() const
{
    return juce::Colour::fromHSV(m_hue, m_saturation, m_value, 1.0f);
}

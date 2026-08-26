/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "RackPanelToggleButton.h"

#include "ApplicationViewState.h"
#include "Utilities.h"

RackPanelToggleButton::RackPanelToggleButton(juce::String title, ApplicationViewState &appState)
    : juce::Button("Toggle " + title),
      m_title(std::move(title)),
      m_appState(appState),
      m_collapsedIcon(GUIHelpers::getDrawableFromSvg(BinaryData::arrowdown18_svg, juce::Colours::black)),
      m_expandedIcon(GUIHelpers::getDrawableFromSvg(BinaryData::arrowright18_svg, juce::Colours::black))
{
}

void RackPanelToggleButton::setAppearance(bool collapsed, juce::Colour headerColour)
{
    if (m_collapsed == collapsed && m_headerColour == headerColour)
        return;

    m_collapsed = collapsed;
    m_headerColour = headerColour;
    setTooltip((m_collapsed ? "Expand " : "Collapse ") + m_title);
    repaint();
}

void RackPanelToggleButton::paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = getLocalBounds().toFloat();

    if (m_collapsed)
    {
        auto railBounds = bounds.reduced(1.0f);
        g.setColour(m_headerColour);
        g.fillRoundedRectangle(railBounds, 7.0f);
        g.setColour(m_appState.getBorderColour());
        g.drawRoundedRectangle(railBounds, 7.0f, 1.0f);

        auto titleArea = railBounds;
        titleArea.removeFromTop(22.0f);
        const auto titleColour = m_headerColour.getBrightness() > 0.8f ? juce::Colours::black : juce::Colours::white;
        g.setColour(titleColour);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));

        juce::Graphics::ScopedSaveState saveState(g);
        const auto centre = titleArea.getCentre();
        g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, centre.x, centre.y));
        const auto textRect = juce::Rectangle<float>(centre.x - titleArea.getHeight() * 0.5f, centre.y - titleArea.getWidth() * 0.5f, titleArea.getHeight(), titleArea.getWidth());
        g.drawFittedText(m_title, textRect.toNearestInt(), juce::Justification::centred, 1);
    }

    if (isMouseOverButton || isButtonDown)
    {
        g.setColour(juce::Colours::white.withAlpha(isButtonDown ? 0.22f : 0.12f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 4.0f);
    }

    const auto iconBounds = m_collapsed ? juce::Rectangle<float>(3.0f, 3.0f, 18.0f, 18.0f) : bounds;
    if (auto *icon = m_collapsed ? m_collapsedIcon.get() : m_expandedIcon.get())
        icon->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
}

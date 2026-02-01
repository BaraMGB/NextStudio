
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities/ApplicationViewState.h"
#include "UI/Controls/AutomatableSlider.h"
#include "SongEditor/TrackHeadComponent.h"
#include "Utilities/Utilities.h"

class NextLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NextLookAndFeel(ApplicationViewState &appState)
        : m_appState(appState)
    {
    }

    juce::Colour getPrimeColour() const { return m_appState.getPrimeColour(); }
    juce::Colour getBackgroundColour1() const { return m_appState.getBackgroundColour1(); }
    juce::Colour getBackgroundColour2() const { return m_appState.getBackgroundColour2(); }
    juce::Colour getBackgroundColour3() const { return m_appState.getBackgroundColour3(); }
    juce::Colour getBorderColour() const { return m_appState.getBorderColour(); }
    juce::Colour getMainFrameColour() const { return m_appState.getMainFrameColour(); }
    juce::Colour getTextColour() const { return m_appState.getTextColour(); }
    juce::Colour getButtonTextColour() const { return m_appState.getButtonTextColour(); }
    juce::Colour getButtonBackgroundColour() const { return m_appState.getButtonBackgroundColour(); }
    juce::Colour getTrackHeaderBackgroundColour() const { return m_appState.getTrackHeaderBackgroundColour(); }
    juce::Colour getTrackHeaderTextColour() const { return m_appState.getTrackHeaderTextColour(); }
    juce::Colour getTrackBackgroundColour() const { return m_appState.getTrackBackgroundColour(); }

    void drawGroupComponentOutline(juce::Graphics &g, int width, int height, const juce::String &text, const juce::Justification &position, juce::GroupComponent &group) override
    {
        const float textH = 15.0f;
        const float indent = 3.0f;
        const float textEdgeGap = 4.0f;
        auto cs = 5.0f;

        juce::Font f{juce::FontOptions(textH)};

        juce::Path p;
        auto x = indent;
        auto y = f.getAscent() - 3.0f;
        auto w = juce::jmax(0.0f, (float)width - x * 2.0f);
        auto h = juce::jmax(0.0f, (float)height - y - indent);
        cs = juce::jmin(cs, w * 0.5f, h * 0.5f);
        auto cs2 = 2.0f * cs;

        auto textW = text.isEmpty() ? 0 : juce::jlimit(0.0f, juce::jmax(0.0f, w - cs2 - textEdgeGap * 2), (float)f.getStringWidth(text) + textEdgeGap * 2.0f);
        auto textX = cs + textEdgeGap;

        if (position.testFlags(juce::Justification::horizontallyCentred))
            textX = cs + (w - cs2 - textW) * 0.5f;
        else if (position.testFlags(juce::Justification::right))
            textX = w - cs - textW - textEdgeGap;

        p.startNewSubPath(x + textX + textW, y);
        p.lineTo(x + w - cs, y);

        p.addArc(x + w - cs2, y, cs2, cs2, 0, juce::MathConstants<float>::halfPi);
        p.lineTo(x + w, y + h - cs);

        p.addArc(x + w - cs2, y + h - cs2, cs2, cs2, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);
        p.lineTo(x + cs, y + h);

        p.addArc(x, y + h - cs2, cs2, cs2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
        p.lineTo(x, y + cs);

        p.addArc(x, y, cs2, cs2, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
        p.lineTo(x + textX, y);

        auto alpha = group.isEnabled() ? 1.0f : 0.5f;

        g.setColour(group.findColour(juce::GroupComponent::outlineColourId).withMultipliedAlpha(alpha));
        g.strokePath(p, juce::PathStrokeType(1.0f));

        // g.fillPath(p);

        g.setColour(group.findColour(juce::GroupComponent::textColourId).withMultipliedAlpha(alpha));
        g.setFont(f);
        g.drawText(text, juce::roundToInt(x + textX), 0, juce::roundToInt(textW), juce::roundToInt(textH), juce::Justification::centred, true);
    }

    void drawTabButtonText(juce::TabBarButton &button, juce::Graphics &g, bool isMouseOver, bool isMouseDown) override
    {
        // Get the bounds of the button
        juce::Rectangle<int> bounds = button.getBounds();

        // Set the font and color for the text
        g.setColour(m_appState.getTextColour());
        g.setFont(juce::FontOptions(bounds.getHeight() * 0.6f));

        // Get the text and orientation
        const juce::String &text = button.getButtonText();
        juce::TabbedButtonBar::Orientation orientation = button.getTabbedButtonBar().getOrientation();

        // Check if the orientation is vertical (TabsAtRight or TabsAtLeft)
        if (orientation == juce::TabbedButtonBar::TabsAtRight || orientation == juce::TabbedButtonBar::TabsAtLeft)
        {
            // Calculate the position and size of the text rectangle
            int textX = (bounds.getWidth() - bounds.getHeight()) / 2;
            int textY = (bounds.getHeight() - bounds.getWidth()) / 2;
            int textWidth = bounds.getHeight();
            int textHeight = bounds.getWidth();

            // Draw the text horizontally within the calculated rectangle
            g.drawFittedText(text, textX, textY, textWidth, textHeight, juce::Justification::centred, 1);
        }
        else
        {
            // Draw the text normally for horizontal tabs
            g.drawText(text, bounds, juce::Justification::centred, true);
        }
    }
    void drawDrawableButton(juce::Graphics &g, juce::DrawableButton &button, bool isMouseOverButton, bool isButtonDown) override
    {
        drawButtonBackground(g, button, button.findColour(juce::DrawableButton::backgroundColourId), isMouseOverButton, isButtonDown);

        const int textH = (button.getStyle() == juce::DrawableButton::ImageAboveTextLabel) ? juce::jmin(16, button.proportionOfHeight(0.20f)) : 0;

        if (textH > 0)
        {
            g.setFont((float)textH);

            g.setColour(button.findColour(button.getToggleState() ? juce::DrawableButton::textColourOnId : juce::DrawableButton::textColourId).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f));

            g.drawFittedText(button.getButtonText(), 4, button.getHeight() - textH - 4, button.getWidth() - 8, textH, juce::Justification::centred, 1);
        }
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour, bool isMouseOverButton, bool isButtonDown) override
    {
        const juce::Rectangle<int> area = button.getLocalBounds().reduced(1);

        auto buttonColour = m_appState.getButtonBackgroundColour();
        if (button.isDown() || button.getToggleState())
        {
            buttonColour = buttonColour.darker(0.7f);
        }

        auto cornerSize = 7.0f;
        if (button.getWidth() < 35)
            cornerSize = 4.f;
        if (button.getWidth() < 25)
            cornerSize = 2.f;

        g.setColour(buttonColour);
        g.fillRoundedRectangle(area.toFloat(), cornerSize);
        auto stroke = 1.f;
        auto borderColour = m_appState.getBorderColour();
        if (button.isDown())
        {
            borderColour = borderColour.darker(0.4f);
        }
        if (button.getToggleState())
        {
            stroke = 2.f;
            borderColour = borderColour.brighter(0.8f);
        }
        g.setColour(borderColour);
        g.drawRoundedRectangle(area.toFloat(), cornerSize, stroke);
    }

    int getSliderThumbRadius(juce::Slider &slider) override { return 13; }

    void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider &slider) override
    {
        if (style == juce::Slider::SliderStyle::LinearBarVertical)
        {
            const auto area = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
            g.setColour(juce::Colour(0xff242424));
            g.fillRoundedRectangle(area, 3);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(area.toFloat(), 3, 1);

            g.setColour(m_appState.getPrimeColour());

            if (slider.getMinimum() < 0 && slider.getMaximum() > 0)
            {
                auto midY = (minSliderPos + maxSliderPos) * 0.5f;

                juce::Rectangle<float> fillRect;
                if (sliderPos < midY)
                    fillRect.setBounds(area.getX(), sliderPos, area.getWidth(), midY - sliderPos);
                else
                    fillRect.setBounds(area.getX(), midY, area.getWidth(), sliderPos - midY);

                g.fillRoundedRectangle(fillRect, 3);

                // Draw a thin line at center
                g.setColour(m_appState.getTextColour().withAlpha(0.5f));
                g.drawHorizontalLine(juce::roundToInt(midY), area.getX(), area.getRight());
            }
            else
            {
                const auto bar = area.withTop(sliderPos);
                g.fillRoundedRectangle(bar, 3);
            }

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(area.toFloat(), 3, 1);
        }
        else if (style == juce::Slider::SliderStyle::LinearBar)
        {
            const auto area = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
            g.setColour(m_appState.getBackgroundColour1());
            g.fillRect(area);

            // Draw center line if range is bipolar
            if (slider.getMinimum() < 0 && slider.getMaximum() > 0)
            {
                auto midX = (minSliderPos + maxSliderPos) * 0.5f;

                juce::Rectangle<float> fillRect;
                if (sliderPos > midX)
                    fillRect.setBounds(midX, area.getY(), sliderPos - midX, area.getHeight());
                else
                    fillRect.setBounds(sliderPos, area.getY(), midX - sliderPos, area.getHeight());

                juce::Colour baseColour = m_appState.getPrimeColour();
                juce::ColourGradient gradient(baseColour.withBrightness(1.3f), fillRect.getTopLeft(), baseColour.withBrightness(0.7f), fillRect.getBottomLeft(), false);
                g.setGradientFill(gradient);
                g.fillRect(fillRect);

                // Draw a thin line at center
                g.setColour(m_appState.getTextColour().withAlpha(0.5f));
                g.drawVerticalLine(juce::roundToInt(midX), area.getY(), area.getBottom());
            }
            else
            {
                auto fillRect = area.withRight(sliderPos);

                juce::Colour baseColour = m_appState.getPrimeColour();
                juce::ColourGradient gradient(baseColour.withBrightness(1.3f), fillRect.getTopLeft(), baseColour.withBrightness(0.7f), fillRect.getBottomLeft(), false);
                g.setGradientFill(gradient);
                g.fillRect(fillRect);
            }

            g.setColour(m_appState.getBorderColour());
            g.drawRect(area, 1.0f);
        }
        else if (style == juce::Slider::SliderStyle::LinearVertical)
        {
            auto trackWidth = 6.0f;
            // Use minSliderPos and maxSliderPos to define the track's vertical extent
            // Note: maxSliderPos is the pixel position for the maximum value (Top), minSliderPos is for Minimum (Bottom)
            float trackTop = (float)maxSliderPos;
            float trackBottom = (float)minSliderPos;
            float trackHeight = trackBottom - trackTop;

            juce::Rectangle<float> trackBounds((float)x + ((float)width - trackWidth) * 0.5f, trackTop, trackWidth, trackHeight);

            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(trackBounds, 3.0f);

            juce::Rectangle<float> fillRect;
            fillRect.setX(trackBounds.getX());
            fillRect.setWidth(trackBounds.getWidth());
            fillRect.setBottom(trackBounds.getBottom());
            fillRect.setTop(sliderPos);

            g.setColour(m_appState.getPrimeColour());
            g.fillRoundedRectangle(fillRect, 3.0f);

            auto thumbWidth = juce::jmin((float)width * 0.7f, 18.0f);
            auto thumbHeight = 26.0f; // Fixed height matching 2*radius

            auto thumbX = (float)x + ((float)width - thumbWidth) * 0.5f;
            auto thumbY = sliderPos - (thumbHeight * 0.5f);

            juce::Rectangle<float> thumbBounds(thumbX, thumbY, thumbWidth, thumbHeight);

            juce::Colour baseColour(0xff222222);
            juce::ColourGradient grad(baseColour.brighter(0.3f), thumbBounds.getTopLeft(), baseColour.darker(0.3f), thumbBounds.getBottomLeft(), false);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(thumbBounds, 2.0f);

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(thumbBounds, 2.0f, 1.0f);

            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.fillRect(thumbBounds.getX() + 3.0f, thumbBounds.getCentreY() - 1.0f, thumbBounds.getWidth() - 6.0f, 2.0f);
        }
        else if (style == juce::Slider::SliderStyle::LinearHorizontal)
        {
            auto trackHeight = 4.0f;
            auto cy = (float)y + (float)height * 0.5f;

            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle((float)x, cy - trackHeight * 0.5f, (float)width, trackHeight, 2.0f);

            auto centerPos = (minSliderPos + maxSliderPos) * 0.5f;

            juce::Rectangle<float> bar;
            if (sliderPos > centerPos)
                bar.setBounds(centerPos, cy - trackHeight * 0.5f, sliderPos - centerPos, trackHeight);
            else
                bar.setBounds(sliderPos, cy - trackHeight * 0.5f, centerPos - sliderPos, trackHeight);

            if (!bar.isEmpty())
            {
                g.setColour(m_appState.getPrimeColour());
                g.fillRoundedRectangle(bar, 2.0f);
            }

            auto thumbWidth = 10.0f;
            auto thumbHeight = 16.0f;
            thumbHeight = juce::jmin(thumbHeight, (float)height);

            juce::Rectangle<float> thumb(sliderPos - thumbWidth * 0.5f, cy - thumbHeight * 0.5f, thumbWidth, thumbHeight);

            juce::Colour baseColour(0xff222222);
            g.setGradientFill(juce::ColourGradient(baseColour.brighter(0.3f), thumb.getTopLeft(), baseColour.darker(0.3f), thumb.getBottomLeft(), false));
            g.fillRoundedRectangle(thumb, 2.0f);

            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(thumb, 2.0f, 1.0f);

            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.fillRect(thumb.getX() + (thumb.getWidth() - 2.0f) * 0.5f, thumb.getY() + 3.0f, 2.0f, thumb.getHeight() - 6.0f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override
    {
        auto isAutomationActive = false;
        auto isModulationActive = false;
        auto volumeColour = m_appState.getPrimeColour();
        auto primeColour = m_appState.getPrimeColour();
        auto backgroundArcColour = juce::Colour(0xff171717);
        float currentModValueNormalized = sliderPos;

        if (auto automatableSlider = dynamic_cast<AutomatableSliderComponent *>(&slider))
        {
            if (auto param = automatableSlider->getAutomatableParameter())
            {
                volumeColour = automatableSlider->getTrackColour();
                isAutomationActive = param->isAutomationActive();
                isModulationActive = !param->getAssignments().isEmpty();

                auto currentVal = param->getCurrentValue();
                currentModValueNormalized = (float)slider.valueToProportionOfLength(currentVal);
            }
        }

        currentModValueNormalized = juce::jlimit(0.0f, 1.0f, currentModValueNormalized);

        const auto radius = (float)juce::jmin(width / 2, height / 2) - 7.0f;
        const auto centreX = (float)x + width * 0.5f;
        const auto centreY = (float)y + height * 0.5f + 3.f;
        const auto rx = centreX - radius;
        const auto ry = centreY - radius;
        const auto rw = radius * 2.0f;

        auto lineW = 7.0f;
        auto arcRadius = radius + 3.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // --- Background Arc ---
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);

        juce::Point<float> startPoint(rx, ry);
        juce::Point<float> endPoint(rx, ry + rw);
        juce::ColourGradient bgGrad(m_appState.getMainFrameColour().darker(.8f), startPoint, m_appState.getMainFrameColour(), endPoint, false);

        g.setGradientFill(bgGrad);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

        // --- Value Arc ---
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

            juce::ColourGradient valGrad(volumeColour.withBrightness(1.3f), startPoint, volumeColour.withBrightness(.7f), endPoint, false);
            g.setGradientFill(valGrad);
            g.strokePath(valueArc, juce::PathStrokeType(lineW - 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }

        // --- Modulation Arc (Outer Ring) ---
        if (isModulationActive)
        {
            auto outerRadius = arcRadius + lineW / 2.0f + 1.0f;
            auto angleBase = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
            auto angleMod = rotaryStartAngle + currentModValueNormalized * (rotaryEndAngle - rotaryStartAngle);

            if (std::abs(angleMod - angleBase) > 0.001f)
            {
                juce::Path outerModPath;
                outerModPath.addCentredArc(centreX, centreY, outerRadius, outerRadius, 0.0f, angleBase, angleMod, true);

                juce::ColourGradient modGrad(primeColour.withBrightness(1.3f), startPoint, primeColour.withBrightness(0.7f), endPoint, false);
                g.setGradientFill(modGrad);
                g.strokePath(outerModPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            }
        }

        juce::Point<float> knobstartPoint(rx, ry);
        juce::Point<float> konbendPoint(rx + rw, ry + rw);
        juce::ColourGradient knobbgGrad(m_appState.getButtonBackgroundColour().brighter(.3), startPoint, m_appState.getButtonBackgroundColour().darker(.3), endPoint, false);
        g.setGradientFill(knobbgGrad);
        g.fillEllipse(rx, ry, rw, rw);
        // --- Thumb (Pointer) ---
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto pointerLength = radius;
        auto pointerThickness = radius < 10 ? 2.f : 4.f;
        juce::Path p;
        p.addEllipse(-pointerThickness * 0.5f, -radius + (radius / 4.0f), pointerThickness, pointerThickness);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        juce::Colour thumbCol = (isAutomationActive || isModulationActive) ? juce::Colours::red : volumeColour.withLightness(.5);
        g.setColour(thumbCol);
        g.fillPath(p);

        g.setColour(thumbCol.withAlpha(.4f));
        // g.setColour(thumbCol.darker(0.7));
        g.strokePath(p, juce::PathStrokeType(1.0f));
        // start cap
        juce::Path sc;
        sc.addRectangle(-0.5f, -radius - lineW + 1.0f, 1.0f, lineW - 1.0f);
        sc.applyTransform(juce::AffineTransform::rotation(rotaryStartAngle).translated(centreX, centreY));
        g.setColour(backgroundArcColour);
        g.fillPath(sc);

        // end cap
        juce::Path ec;
        ec.addRectangle(-0.5f, -radius - lineW, 1.0f, lineW);
        ec.applyTransform(juce::AffineTransform::rotation(rotaryEndAngle).translated(centreX, centreY));
        g.setColour(backgroundArcColour);
        g.fillPath(ec);
    }

    juce::Font getTextButtonFont(juce::TextButton &, int buttonHeight) override { return juce::Font(juce::FontOptions(juce::jmin(10.0f, buttonHeight * 0.6f))); }

    void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        // drawButtonBackground (g, button, juce::Colour(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto buttonArea = button.getLocalBounds();
        // buttonArea.reduce(2, 2);
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(buttonArea.toFloat(), 2);

        auto textColour = juce::Colour(0xff7b7b7b);
        if (button.getToggleState())
        {
            textColour = juce::Colour(0xff000000);
            if (button.getComponentID() == "solo")
            {
                g.setColour(juce::Colours::lightgreen);
            }
            else if (button.getComponentID() == "mute")
            {
                g.setColour(juce::Colours::darkorange);
            }
            else if (button.getComponentID() == "arm")
            {
                g.setColour(juce::Colours::firebrick);
            }
            else
            {
                g.setColour(m_appState.getButtonBackgroundColour().greyLevel(0.7));
            }
        }
        else
        {
            g.setColour(m_appState.getButtonBackgroundColour());
        }
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 1);
        // Draw checkmark for normal toggle buttons when toggled on
        if (button.getToggleState() && button.getComponentID().isEmpty())
        {
            auto fullArea = button.getLocalBounds();
            auto center = fullArea.getCentre().toFloat();

            auto size = (float)fullArea.getHeight() * 0.4f;

            auto p1 = juce::Point<float>(center.x - size * 0.5f, center.y + size * 0.1f);

            auto p2 = juce::Point<float>(center.x - size * 0.1f, center.y + size * 0.5f);

            auto p3 = juce::Point<float>(center.x + size * 0.5f, center.y - size * 0.4f);

            g.setColour(m_appState.getButtonTextColour());
            g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.f);
            g.drawLine(p2.x, p2.y, p3.x, p3.y, 2.f);
        }

        g.setColour(textColour);
        g.drawFittedText(button.getName(), button.getLocalBounds(), juce::Justification::centred, 1);
    }

    void drawFileBrowserRow(juce::Graphics &g, int width, int height, const juce::File &file, const juce::String &filename, juce::Image *icon, const juce::String &fileSizeDescription, const juce::String &fileTimeDescription, bool isDirectory, bool isItemSelected, int itemIndex, juce::DirectoryContentsDisplayComponent &dcc) override
    {

        auto w = 24;
        auto h = 24;
        juce::Image iconImage(juce::Image::RGB, w, h, true);
        juce::Graphics graph(iconImage);
        juce::Rectangle<float> rect = {0.0, 0.0, (float)w, (float)h};
        if (isDirectory)
        {

            GUIHelpers::drawFromSvg(graph, BinaryData::folder_svg, juce::Colour(0xffbbbbbb), rect);
        }
        else if (filename.endsWith(".wav"))
        {
            GUIHelpers::drawFromSvg(graph, BinaryData::filemusic_svg, juce::Colour(0xffbbbbbb), rect);
        }

        else
        {
            GUIHelpers::drawFromSvg(graph, BinaryData::file_svg, juce::Colour(0xffbbbbbb), rect);
        }
        icon = &iconImage;
        auto fileListComp = dynamic_cast<juce::Component *>(&dcc);

        if (isItemSelected)
            g.fillAll(fileListComp != nullptr ? fileListComp->findColour(juce::DirectoryContentsDisplayComponent::highlightColourId) : findColour(juce::DirectoryContentsDisplayComponent::highlightColourId));

        const int x = 32;
        g.setColour(juce::Colours::black);

        if (icon != nullptr && icon->isValid())
        {
            g.drawImageWithin(*icon, 2, 2, x - 4, height - 4, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, false);
        }
        else
        {
            if (auto *d = isDirectory ? getDefaultFolderImage() : getDefaultDocumentFileImage())
                d->drawWithin(g, juce::Rectangle<float>(2.0f, 2.0f, x - 4.0f, height - 4.0f), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
        }

        if (isItemSelected)
            g.setColour(fileListComp != nullptr ? fileListComp->findColour(juce::DirectoryContentsDisplayComponent::highlightedTextColourId) : findColour(juce::DirectoryContentsDisplayComponent::highlightedTextColourId));
        else
            g.setColour(fileListComp != nullptr ? fileListComp->findColour(juce::DirectoryContentsDisplayComponent::textColourId) : findColour(juce::DirectoryContentsDisplayComponent::textColourId));

        g.setFont(height * 0.7f);

        if (width > 450 && !isDirectory)
        {
            auto sizeX = juce::roundToInt(width * 0.7f);
            auto dateX = juce::roundToInt(width * 0.8f);

            g.drawFittedText(filename, x, 0, sizeX - x, height, juce::Justification::centredLeft, 1, 1.0f);

            g.setFont(height * 0.5f);
            g.setColour(juce::Colours::darkgrey);

            if (!isDirectory)
            {
                g.drawFittedText(fileSizeDescription, sizeX, 0, dateX - sizeX - 8, height, juce::Justification::centredRight, 1, 1.0f);

                g.drawFittedText(fileTimeDescription, dateX, 0, width - 8 - dateX, height, juce::Justification::centredRight, 1, 1.0f);
            }
        }
        else
        {
            g.drawFittedText(filename, x, 0, width - x, height, juce::Justification::centredLeft, 1, 1.0f);
        }
    }

    void drawScrollbar(juce::Graphics &g, juce::ScrollBar &, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        if (isScrollbarVertical)
        {
            if (isMouseDown)
            {
                g.setColour(juce::Colour(0x66ffffff));
                g.fillRect(juce::Rectangle<int>(0, thumbStartPosition, width, thumbSize));
            }
            else if (isMouseOver)
            {
                g.setColour(juce::Colour(0x33ffffff));
                g.fillRect(juce::Rectangle<int>(0, thumbStartPosition, width, thumbSize));
            }
        }
        else
        {
            if (isMouseDown)
            {
                g.setColour(juce::Colour(0x66ffffff));
                g.fillRect(juce::Rectangle<int>(thumbStartPosition, 0, thumbSize, height));
            }
            else if (isMouseOver)
            {
                g.setColour(juce::Colour(0x33ffffff));
                g.fillRect(juce::Rectangle<int>(thumbStartPosition, 0, thumbSize, height));
            }
        }
    }
    // void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
    // {
    //     auto rect = juce::Rectangle<float>(0.f, 0.f, w, h);
    //     g.setColour(m_appState.getBackgroundColour());
    //     g.fillRect(rect);
    //     g.setColour(m_appState.getBorderColour());
    //     g.drawRect(rect, 1);
    // }
    // void drawPopupMenuItem(
    //     juce::Graphics &g
    //     , const juce::Rectangle<int> &area
    //     , bool isSeparator
    //     , bool isActive
    //     , bool isHighlighted
    //     , bool isTicked
    //     , bool hasSubMenu
    //     , const juce::String &text
    //     , const juce::String &shortcutKeyText
    //     , const juce::Drawable *icon
    //     , const juce::Colour *textColour) override
    // {
    //     g.setFont(juce::Font(12, juce::Font::plain));
    //
    //     if (isHighlighted)
    //     {
    //         g.setFont(juce::Font(12, juce::Font::bold));
    //         auto r = area.toFloat();
    //         r.reduce(2,2);
    //         g.setColour(m_appState.getMenuBackgroundColour());
    //         g.fillRoundedRectangle(r, 2);
    //     }
    //
    //     g.setColour(m_appState.getTextColour());
    //     auto rect = area;
    //     rect.removeFromLeft(10);
    //     g.drawFittedText(text, rect, juce::Justification::left, 1);
    // }
    //
    void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox &box) override
    {
        using namespace juce;
        // g.fillAll(m_appState.getBackgroundColour());
        const auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 1.5f;
        Rectangle<int> boxBounds(0, 0, width, height);
        boxBounds.reduce(4, 4);

        auto isChoiceCompChild = (box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr);

        if (isChoiceCompChild)
        {

            g.setColour(m_appState.getBackgroundColour1());
            g.fillRect(boxBounds);

            auto arrowZone = boxBounds.removeFromRight(boxBounds.getHeight()).reduced(0, 2).toFloat();
            GUIHelpers::drawFromSvg(g, BinaryData::arrowdown18_svg, m_appState.getTextColour(), arrowZone);
        }
        else
        {
            g.setColour(m_appState.getBackgroundColour1());
            g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
            g.setColour(m_appState.getBorderColour());
            g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

            auto arrowZone = boxBounds.removeFromRight(boxBounds.getHeight()).toFloat();
            GUIHelpers::drawFromSvg(g, BinaryData::arrowdown18_svg, m_appState.getTextColour(), arrowZone);
        }
    }

    juce::Label *createComboBoxTextBox(juce::ComboBox &) override
    {
        auto *l = new juce::Label("txt", juce::String());
        l->setJustificationType(juce::Justification::centred);
        return l;
    }

private:
    ApplicationViewState &m_appState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NextLookAndFeel)
};

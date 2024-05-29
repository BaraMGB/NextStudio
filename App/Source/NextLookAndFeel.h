
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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeadComponent.h"
#include "AutomatableSliderComponent.h"
#include "ApplicationViewState.h"
#include "Utilities.h"

class NextLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NextLookAndFeel(ApplicationViewState& appState)
        : m_appState(appState)
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xffff00ff));
        setColour(juce::TextButton::buttonColourId , m_appState.getButtonBackgroundColour());
        setColour(juce::TextButton::textColourOnId, m_appState.getTextColour());
        setColour(juce::DrawableButton::backgroundColourId, m_appState.getButtonBackgroundColour());
        setColour(juce::PopupMenu::backgroundColourId, m_appState.getMenuBackgroundColour());
        setColour(juce::TabbedComponent::backgroundColourId, m_appState.getMenuBackgroundColour());
        setColour(juce::TabbedButtonBar::tabTextColourId, m_appState.getTextColour());
        setColour(juce::ListBox::backgroundColourId, m_appState.getMenuBackgroundColour());
        setColour(juce::Slider::thumbColourId, m_appState.getTextColour());
        setColour(juce::Slider::trackColourId, m_appState.getTextColour());
        setColour(juce::Slider::backgroundColourId, m_appState.getTextColour().withAlpha(0.3f));

        setColour (juce::TableListBox::ColourIds::backgroundColourId, m_appState.getMenuBackgroundColour());
        setColour (juce::Label::ColourIds::textColourId, m_appState.getTextColour());
        setColour (juce::TextEditor::ColourIds::backgroundColourId, m_appState.getMainFrameColour());

        setColour(juce::TooltipWindow::outlineColourId, m_appState.getBorderColour());
        setColour(juce::TooltipWindow::backgroundColourId, m_appState.getMainFrameColour());
        setColour(juce::TooltipWindow::textColourId, m_appState.getTextColour());

        setColour(juce::TableHeaderComponent::ColourIds::backgroundColourId, m_appState.getMenuBackgroundColour());
        setColour(juce::TableHeaderComponent::ColourIds::textColourId, m_appState.getTextColour());
        setColour(juce::TableHeaderComponent::ColourIds::outlineColourId, m_appState.getBorderColour());
        setColour(juce::TableHeaderComponent::ColourIds::highlightColourId, m_appState.getPrimeColour());
        setColour(juce::GroupComponent::ColourIds::outlineColourId, m_appState.getBorderColour());
        setColour(juce::GroupComponent::ColourIds::textColourId, m_appState.getTextColour());

    }
    
    void drawGroupComponentOutline (juce::Graphics& g, int width, int height,
                                                const juce::String& text, const juce::Justification& position,
                                                juce::GroupComponent& group) override
    {
        const float textH = 15.0f;
        const float indent = 3.0f;
        const float textEdgeGap = 4.0f;
        auto cs = 5.0f;

        juce::Font f (textH);

        juce::Path p;
        auto x = indent;
        auto y = f.getAscent() - 3.0f;
        auto w = juce::jmax (0.0f, (float) width - x * 2.0f);
        auto h = juce::jmax (0.0f, (float) height - y  - indent);
        cs = juce::jmin (cs, w * 0.5f, h * 0.5f);
        auto cs2 = 2.0f * cs;

        auto textW = text.isEmpty() ? 0
            : juce::jlimit (0.0f,
                            juce::jmax (0.0f, w - cs2 - textEdgeGap * 2),
                            (float) f.getStringWidth (text) + textEdgeGap * 2.0f);
        auto textX = cs + textEdgeGap;

        if (position.testFlags (juce::Justification::horizontallyCentred))
            textX = cs + (w - cs2 - textW) * 0.5f;
        else if (position.testFlags (juce::Justification::right))
            textX = w - cs - textW - textEdgeGap;

        p.startNewSubPath (x + textX + textW, y);
        p.lineTo (x + w - cs, y);

        p.addArc (x + w - cs2, y, cs2, cs2, 0, juce::MathConstants<float>::halfPi);
        p.lineTo (x + w, y + h - cs);

        p.addArc (x + w - cs2, y + h - cs2, cs2, cs2, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);
        p.lineTo (x + cs, y + h);

        p.addArc (x, y + h - cs2, cs2, cs2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
        p.lineTo (x, y + cs);

        p.addArc (x, y, cs2, cs2, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
        p.lineTo (x + textX, y);

        auto alpha = group.isEnabled() ? 1.0f : 0.5f;

        g.setColour (group.findColour (juce::GroupComponent::outlineColourId)
                     .withMultipliedAlpha (alpha));
        g.strokePath (p, juce::PathStrokeType (1.0f));

        // g.fillPath(p);

        g.setColour (group.findColour (juce::GroupComponent::textColourId)
                     .withMultipliedAlpha (alpha));
        g.setFont (f);
        g.drawText (text,
                    juce::roundToInt (x + textX), 0,
                    juce::roundToInt (textW),
                    juce::roundToInt (textH),
                    juce::Justification::centred, true);
    }
    void drawTabButton (juce::TabBarButton& tbb, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        if (tbb.getToggleState() == false)
        {
            g.setColour (m_appState.getBorderColour());
            g.drawLine(tbb.getLocalBounds().getWidth(), 0, tbb.getLocalBounds().getWidth(), tbb.getLocalBounds().getHeight());
            g.setColour (m_appState.getTextColour().withAlpha(0.4f));
            g.drawFittedText (tbb.getButtonText(), 0, 0, tbb.getWidth(), tbb.getHeight(), juce::Justification::centred, 1);
        }
        else
        {
            g.fillAll(m_appState.getMenuBackgroundColour());
            g.setColour (m_appState.getBorderColour());
            g.drawLine(tbb.getLocalBounds().getWidth(), 0, tbb.getLocalBounds().getWidth(), tbb.getLocalBounds().getHeight());
            g.setColour (m_appState.getTextColour());
            auto font = g.getCurrentFont();
            font.setUnderline(true);
            g.setFont(font);
            g.drawFittedText (tbb.getButtonText(), 0, 0, tbb.getWidth(), tbb.getHeight(), juce::Justification::centred, 1);
        }
    }

    void drawDrawableButton(juce::Graphics& g,
                              juce::DrawableButton& button,
                              bool isMouseOverButton,
                              bool isButtonDown) override
    {
        drawButtonBackground(g, button, button.findColour(juce::DrawableButton::backgroundColourId) , isMouseOverButton,  isButtonDown);

        const int textH = (button.getStyle() == juce::DrawableButton::ImageAboveTextLabel)
                        ? juce::jmin (16, button.proportionOfHeight (0.20f))
                        : 0;

        if (textH > 0)
        {
            g.setFont ((float) textH);

            g.setColour (button.findColour (button.getToggleState() ? juce::DrawableButton::textColourOnId
                                                        : juce::DrawableButton::textColourId)
                            .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));

            g.drawFittedText (button.getButtonText(),
                              4, button.getHeight() - textH - 4,
                              button.getWidth() - 8, textH,
                              juce::Justification::centred, 1);
        }

    }
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override
    {
        const juce::Rectangle<int> area = button.getLocalBounds ().reduced(1);

        auto buttonColour = backgroundColour;
        if (button.isDown ())
        {
            buttonColour = buttonColour.darker (0.4f);
        }

        auto cornerSize = 7.0f;
        if (button.getWidth() < 35)
            cornerSize = 4.f;
        if (button.getWidth() < 25)
            cornerSize = 2.f;

        g.setColour(buttonColour);
        g.fillRoundedRectangle (area.toFloat (), cornerSize);

        auto borderColour = juce::Colour(0xff888888);
        if (button.isDown ())
        {
            borderColour = borderColour.darker (0.4f);
        }
        g.setColour(borderColour);
        g.drawRoundedRectangle (area.toFloat (), cornerSize, 1.f);
    }


    void drawLinearSlider (juce::Graphics&g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::SliderStyle::LinearBarVertical)
        {
            const auto area = juce::Rectangle<float>(x, y, width, height);
            const auto slider = area.withTop(sliderPos);
            g.setColour(juce::Colour(0xff242424));
            g.fillRoundedRectangle(area, 3);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(area.toFloat(), 3,1);

            g.setColour(m_appState.getPrimeColour());
            g.fillRoundedRectangle(slider, 3);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(slider.toFloat(), 3,1);
        }
        else {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawRotarySlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto isAutomationActive = false;
        auto volumeColour = juce::Colour(0x88e9e949);
        auto backgroundArcColour = juce::Colour(0xff171717);
        if (auto automatableSlider = dynamic_cast<AutomatableSliderComponent*>(&slider))
        {
            volumeColour = automatableSlider->getTrackColour();
            if (automatableSlider->getAutomatableParameter()->isAutomationActive())
                isAutomationActive = true;
        }
        auto radius = juce::jmin(width / 2, height / 2) - 10.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto thumbColour = isAutomationActive ? juce::Colours::red : volumeColour;
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();//.reduced(10);
        auto lineW = 7;
        auto arcRadius = radius + 3;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path contureArc;
        contureArc.addCentredArc(bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    rotaryEndAngle,
                                    true);
        g.setColour(backgroundArcColour);
        g.strokePath(contureArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    rotaryEndAngle,
                                    true);
        g.setColour(volumeColour);
        juce::Point<float> startPoint(rx, ry); 
        juce::Point<float> endPoint(rx, ry + rw); 

        juce::ColourGradient gradient(m_appState.getMainFrameColour().darker(.8f), startPoint, m_appState.getMainFrameColour(), endPoint, false);
        g.setGradientFill(gradient);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW-2, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   arcRadius,
                                   arcRadius,
                                   0.0f,
                                   rotaryStartAngle,
                                   toAngle,
                                   true);

            g.setColour(volumeColour);
            juce::Point<float> startPoint(rx, ry); 
            juce::Point<float> endPoint(rx, ry + rw); 

            juce::ColourGradient gradient(volumeColour.withBrightness(1.3f), startPoint, volumeColour.withBrightness(.7f), endPoint, false);
            g.setGradientFill(gradient);
            g.strokePath(valueArc, juce::PathStrokeType(lineW - 2, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }

        // pointer
        juce::Path p;
        auto pointerLength = radius ;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius + 1, pointerThickness, pointerLength * 0.5 );
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour (thumbColour);
        g.fillPath (p);

        //start cap
        juce::Path sc;
        sc.addRectangle(- .5,- radius - lineW +1,  1, lineW - 1);
        sc.applyTransform(juce::AffineTransform::rotation(rotaryStartAngle).translated(centreX, centreY));
        g.setColour (backgroundArcColour);
        g.fillPath (sc);

        //end cap
        juce::Path ec;
        ec.addRectangle(- .5,- radius - lineW, 1, lineW);
        ec.applyTransform(juce::AffineTransform::rotation(rotaryEndAngle).translated(centreX, centreY));
        g.setColour (backgroundArcColour);
        g.fillPath (ec);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return {juce::jmin(10.0f, buttonHeight * 0.6f)};
    }

//    void drawImageButton ( 	Graphics & g,
//                            Image * image,
//                            int  	imageX,
//                            int  	imageY,
//                            int  	imageW,
//                            int  	imageH,
//                            const Colour & overlayColour,
//                            float  	imageOpacity,
//                            ImageButton & button
//                            ) override
//    {
//        drawButtonBackground (g, button,Colour(),button.m_isOver (), button.isDown ());
//        //LookAndFeel_V4::drawImageButton ( g, image,imageX, imageY, imageW, imageH,overlayColour, imageOpacity, button);
//    }

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        //drawButtonBackground (g, button, juce::Colour(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto buttonArea = button.getLocalBounds();
        //buttonArea.reduce(2, 2);
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
                g.setColour(juce::Colours::gold);
            }
        }
        else
        {
            g.setColour(juce::Colour(0xff343434));
        }
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 1);
        g.setColour(textColour);
        //        auto fontSize = jmin (15.0f, button.getHeight() * 0.75f);
        //        auto tickWidth = fontSize * 1.1f;
        g.drawFittedText(
            button.getName(), button.getLocalBounds(), juce::Justification::centred, 1);
    }




    void drawFileBrowserRow(juce::Graphics &g,
                            int width,
                            int height,
                            const juce::File &file,
                            const juce::String &filename,
                            juce::Image *icon,
                            const juce::String &fileSizeDescription,
                            const juce::String &fileTimeDescription,
                            bool isDirectory, bool isItemSelected,
                            int itemIndex,
                            juce::DirectoryContentsDisplayComponent &dcc) override
    {

        auto w = 24;
        auto h = 24;
        juce::Image iconImage (juce::Image::RGB, w, h, true);
        juce::Graphics graph (iconImage);
        juce::Rectangle <float> rect = {0.0, 0.0,(float) w,(float) h};
        if (isDirectory)
        {

           GUIHelpers::drawFromSvg (graph,
                                    BinaryData::folder_svg,
                                    juce::Colour(0xffbbbbbb),
                                    rect);
        }
        else if (filename.endsWith (".wav"))
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::filemusic_svg,
                                     juce::Colour(0xffbbbbbb),
                                     rect);
        }

        else
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::file_svg,
                                     juce::Colour(0xffbbbbbb),
                                     rect);
        }
        icon = &iconImage;
//        juce::LookAndFeel_V4::drawFileBrowserRow (g, width, height, file, filename, icon,
//                                                                  fileSizeDescription, fileTimeDescription,
//                                                                  isDirectory, isItemSelected, itemIndex, dcc);
        auto fileListComp = dynamic_cast<juce::Component*> (&dcc);

        if (isItemSelected)
            g.fillAll (fileListComp != nullptr ? fileListComp->findColour (juce::DirectoryContentsDisplayComponent::highlightColourId)
                                               : findColour (juce::DirectoryContentsDisplayComponent::highlightColourId));

        const int x = 32;
        g.setColour (juce::Colours::black);

        if (icon != nullptr && icon->isValid())
        {
            g.drawImageWithin (*icon, 2, 2, x - 4, height - 4,
                               juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                               false);
        }
        else
        {
            if (auto* d = isDirectory ? getDefaultFolderImage()
                                      : getDefaultDocumentFileImage())
                d->drawWithin (g, juce::Rectangle<float> (2.0f, 2.0f, x - 4.0f, height - 4.0f),
                               juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
        }

            if (isItemSelected)
            g.setColour (fileListComp != nullptr ? fileListComp->findColour (juce::DirectoryContentsDisplayComponent::highlightedTextColourId)
                                                 : findColour (juce::DirectoryContentsDisplayComponent::highlightedTextColourId));
        else
            g.setColour (fileListComp != nullptr ? fileListComp->findColour (juce::DirectoryContentsDisplayComponent::textColourId)
                                                 : findColour (juce::DirectoryContentsDisplayComponent::textColourId));

        g.setFont (height * 0.7f);

        if (width > 450 && ! isDirectory)
        {
            auto sizeX = juce::roundToInt (width * 0.7f);
            auto dateX = juce::roundToInt (width * 0.8f);

            g.drawFittedText (filename,
                              x, 0, sizeX - x, height,
                              juce::Justification::centredLeft, 1 , 1.0f);

            g.setFont (height * 0.5f);
            g.setColour (juce::Colours::darkgrey);

            if (! isDirectory)
            {
                g.drawFittedText (fileSizeDescription,
                                  sizeX, 0, dateX - sizeX - 8, height,
                                  juce::Justification::centredRight, 1, 1.0f);

                g.drawFittedText (fileTimeDescription,
                                  dateX, 0, width - 8 - dateX, height,
                                  juce::Justification::centredRight, 1, 1.0f);
            }
        }
        else
        {
            g.drawFittedText (filename,
                              x, 0, width - x, height,
                              juce::Justification::centredLeft, 1, 1.0f);

        }
    }




    void drawScrollbar (juce::Graphics &g,
                       juce::ScrollBar &,
                       int x, int y,
                       int width, int height,
                       bool isScrollbarVertical,
                       int thumbStartPosition,
                       int thumbSize,
                       bool isMouseOver,
                       bool isMouseDown) override
    {
        if (isScrollbarVertical)
        {
            if (isMouseDown)
            {
                g.setColour (juce::Colour(0x66ffffff));
                g.fillRect (juce::Rectangle<int> (0, thumbStartPosition, width, thumbSize));
            }
            else if (isMouseOver)
            {
                g.setColour (juce::Colour(0x33ffffff));
                g.fillRect (juce::Rectangle<int> (0, thumbStartPosition, width, thumbSize));
            }
        }
        else
        {
            if (isMouseDown)
            {
                g.setColour (juce::Colour(0x66ffffff));
                g.fillRect (juce::Rectangle<int> (thumbStartPosition, 0, thumbSize, height));
            }
            else if (isMouseOver)
            {
                g.setColour (juce::Colour(0x33ffffff));
                g.fillRect (juce::Rectangle<int> (thumbStartPosition, 0, thumbSize, height));
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
    void drawComboBox (juce::Graphics &g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox &box) override
    {
        using namespace juce;
        // g.fillAll(m_appState.getBackgroundColour());
        const auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 1.5f;
        Rectangle<int> boxBounds (0, 0, width, height);
        boxBounds.reduce(4,4);

        auto isChoiceCompChild = (box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr);

        if (isChoiceCompChild)
        {

            g.setColour (m_appState.getMenuBackgroundColour());
            g.fillRect (boxBounds);

            auto arrowZone = boxBounds.removeFromRight (boxBounds.getHeight()).reduced (0, 2).toFloat();
            GUIHelpers::drawFromSvg(g, BinaryData::arrowdown18_svg, m_appState.getTextColour(), arrowZone);
        }
        else
        {
            g.setColour (m_appState.getMenuBackgroundColour());
            g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
            g.setColour (m_appState.getBorderColour());
            g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

            auto arrowZone = boxBounds.removeFromRight (boxBounds.getHeight()).toFloat();
            GUIHelpers::drawFromSvg(g, BinaryData::arrowdown18_svg, m_appState.getTextColour(), arrowZone);

        }
    }

private:
    ApplicationViewState& m_appState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NextLookAndFeel)
};

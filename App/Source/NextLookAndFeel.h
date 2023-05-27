/*
  ==============================================================================

    NextLookAndFeel.h
    Created: 27 Dec 2019 11:29:28pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeadComponent.h"
#include "AutomatableSliderComponent.h"

class NextLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NextLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff000000));
    }
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override
    {
        const juce::Rectangle<int> area = button.getLocalBounds ();

        auto buttonColour = juce::Colour(0xff4b4b4b);
        if (button.isDown ())
        {
            buttonColour = buttonColour.darker (0.4f);
        }
        g.setColour(buttonColour);
        g.fillRoundedRectangle (area.toFloat (), 5.0f);

        auto borderColour = juce::Colour(0xffcccccc);
        if (button.isDown ())
        {
            borderColour = borderColour.darker (0.4f);
        }
        g.setColour(borderColour);
        g.drawRoundedRectangle (area.reduced (1).toFloat (), 5.0f, 0.5f);
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
        auto radius = juce::jmin(width / 2, height / 2) - 10.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle =
            rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto thumbColour = juce::Colour(0xff000000);
        auto thumbMouseColour = juce::Colour(0xff999999);
        auto volumeColour = juce::Colour(0x88e9e949);
        auto isAutomationActive = false;
        if (auto automatableSlider = dynamic_cast<AutomatableSliderComponent*>(&slider))
        {
            volumeColour = automatableSlider->getTrackColour();
            if (automatableSlider->getAutomatableParameter()->isAutomationActive())
            {
                isAutomationActive = true;
            }

        }
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto lineW = 5;
        auto arcRadius = radius + 3;
        auto toAngle =
            rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // fill

        g.setGradientFill ({juce::Colour(0xffbbbbbb),
                            0,
                            0,
                            juce::Colour(0xff2b2b2b),
                            0,
                            static_cast<float>(height),
                            false});
        g.fillEllipse(rx, ry, rw, rw);
        juce::Rectangle<int> r;

        g.setGradientFill ({juce::Colour(0xff1b1b1b),
                            static_cast<float>(width/2),
                            static_cast<float>(height/2),
                            juce::Colour(0xff3b3b3b),
                            static_cast<float>(width),
                            static_cast<float>(height),
                            true});
        g.fillEllipse(rx + 3, ry + 3, rw - 6, rw - 6);
        juce::Path backgroundArc;
                backgroundArc.addCentredArc(bounds.getCentreX(),
                                            bounds.getCentreY(),
                                            arcRadius,
                                            arcRadius,
                                            0.0f,
                                            rotaryStartAngle,
                                            rotaryEndAngle,
                                            true);

                g.strokePath(
                    backgroundArc,
                    juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

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
                    g.strokePath(valueArc,
                                 juce::PathStrokeType(lineW,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::butt));
                }

                juce::Path p;
                auto pointerLength = radius * 0.33f;
                auto pointerThickness = 2.0f;
                p.addRectangle(
                    -pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
                p.applyTransform(
                    juce::AffineTransform::rotation(angle).translated(centreX, centreY));
                // pointer
                g.setColour (thumbColour);
                g.fillPath (p);
                if (isAutomationActive)
                {
                    g.setColour (juce::Colour(0xffaa3300));
                    g.fillEllipse ({centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f});
                }

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
                                    "#bbbbbb",
                                    rect);
        }
        else if (filename.endsWith (".wav"))
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::filemusic_svg,
                                     "#bbbbbb",
                                     rect);
        }

        else
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::file_svg,
                                     "#bbbbbb",
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
    void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
    {
        g.fillAll (juce::Colours::red);
    }
    void drawPopupMenuItem(
            juce::Graphics &g
          , const juce::Rectangle<int> &area
          , bool isSeparator
          , bool isActive
          , bool isHighlighted
          , bool isTicked
          , bool hasSubMenu
          , const juce::String &text
          , const juce::String &shortcutKeyText
          , const juce::Drawable *icon
          , const juce::Colour *textColour) override
    {
        g.setFont(juce::Font( juce::Typeface::createSystemTypefaceFor(
                                  BinaryData::IBMPlexSansRegular_ttf
                                , BinaryData::IBMPlexSansRegular_ttfSize)->getName(), 12, juce::Font::FontStyleFlags::plain ));
        g.setColour(*textColour);
        g.drawFittedText(text, area, juce::Justification::left, 1);
    }


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NextLookAndFeel)
};

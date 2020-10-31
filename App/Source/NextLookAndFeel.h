/*
  ==============================================================================

    NextLookAndFeel.h
    Created: 27 Dec 2019 11:29:28pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackComponent.h"
//#include "BinaryData.h"

class NextLookAndFeel : public LookAndFeel_V4
{
public:
    NextLookAndFeel()
    {
        setColour(ResizableWindow::backgroundColourId, Colour(0xff000000));
    }
    void drawButtonBackground(Graphics& g,
                              Button& button,
                              const Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override
    {
        juce::Rectangle<int> area = button.getLocalBounds ();



        g.setColour(Colour(0xff000000));
        g.drawRect(area, 1);
        area.reduce(1, 1);

        auto buttonColour = Colour(0xff4b4b4b);


        if (isButtonDown)
        {
            buttonColour = buttonColour.darker (0.4f);
        }


        juce::ColourGradient border = {buttonColour.brighter (0.1f),
                                       0,
                                       0,
                                       buttonColour.brighter (0.1f),
                                       0,
                                       static_cast<float>(button.getHeight()),
                                       false};
        border.addColour (0.5, buttonColour.brighter (0.8f));
        g.setGradientFill (border);
        g.fillRect (area);
        area.reduce (1,1);
        juce::ColourGradient gradient = {buttonColour,
                                         0,
                                         0,
                                         buttonColour,
                                         0,
                                         static_cast<float>(button.getHeight()),
                                         false};
        gradient.addColour (0.5, buttonColour.brighter (0.05f));
        g.setGradientFill(gradient);
        g.fillRect(area);
    }


    void drawRotarySlider(Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          Slider& slider) override
    {
        auto radius = jmin(width / 2, height / 2) - 10.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle =
            rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto thumbColour = Colour(0xff000000);
        auto thumbMouseColour = Colour(0xff999999);
        auto volumeColour = Colour(0x88e9e949);
        auto thc = dynamic_cast<TrackHeaderComponent*>(slider.getParentComponent ());
        if (thc != nullptr)
        {
            volumeColour = thc->getTrackColour ();
        }
        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto lineW = 5;
        auto arcRadius = radius + 3;
        auto toAngle =
            rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // fill

        g.setGradientFill ({Colour(0xffbbbbbb),
                            0,
                            0,
                            Colour(0xff2b2b2b),
                            0,
                            static_cast<float>(height),
                            false});
        g.fillEllipse(rx, ry, rw, rw);
        Rectangle<int> r;

        g.setGradientFill ({Colour(0xff1b1b1b),
                            static_cast<float>(width/2),
                            static_cast<float>(height/2),
                            Colour(0xff3b3b3b),
                            static_cast<float>(width),
                            static_cast<float>(height),
                            true});
        g.fillEllipse(rx + 3, ry + 3, rw - 6, rw - 6);
        Path backgroundArc;
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
                    PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::butt));

                if (slider.isEnabled())
                {
                    Path valueArc;
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
                                 PathStrokeType(lineW,
                                                PathStrokeType::curved,
                                                PathStrokeType::butt));
                }

                Path p;
                auto pointerLength = radius * 0.33f;
                auto pointerThickness = 2.0f;
                p.addRectangle(
                    -pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
                p.applyTransform(
                    AffineTransform::rotation(angle).translated(centreX, centreY));
                // pointer
                g.setColour(thumbColour);
                g.fillPath(p);

    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return {jmin(10.0f, buttonHeight * 0.6f)};
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
//        drawButtonBackground (g, button,Colour(),button.isOver (), button.isDown ());
//        //LookAndFeel_V4::drawImageButton ( g, image,imageX, imageY, imageW, imageH,overlayColour, imageOpacity, button);
//    }

    void drawToggleButton(Graphics& g,
                          ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        //drawButtonBackground (g, button, juce::Colour(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto buttonArea = button.getLocalBounds();
        buttonArea.reduce(2, 2);
        g.setColour(Colour(0xff000000));
        g.fillRoundedRectangle(buttonArea.toFloat(), 7);
        g.setColour(Colour(0xff343434));
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 5);
        auto textColour = juce::Colour(0xff7b7b7b);
        if (button.getToggleState())
        {
            textColour = Colour(0xff000000);
            if (button.getComponentID() == "solo")
            {
                g.setColour(Colours::lightgreen);
            }
            else if (button.getComponentID() == "mute")
            {
                g.setColour(Colours::indigo);
            }
            else
            {
                g.setColour(Colours::gold);
            }
        }
        else
        {
            g.setColour(Colour(14, 14, 14));
        }
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 5);
        g.setColour(textColour);
        //        auto fontSize = jmin (15.0f, button.getHeight() * 0.75f);
        //        auto tickWidth = fontSize * 1.1f;
        g.drawFittedText(
            button.getName(), button.getLocalBounds(), Justification::centred, 1);
    }




    void drawFileBrowserRow(Graphics &g,
                            int width,
                            int height,
                            const File &file,
                            const String &filename,
                            Image *icon,
                            const String &fileSizeDescription,
                            const String &fileTimeDescription,
                            bool isDirectory, bool isItemSelected,
                            int itemIndex,
                            DirectoryContentsDisplayComponent &dcc) override
    {

        auto w = 24;
        auto h = 24;
        juce::Image iconImage (Image::RGB, w, h, true);
        juce::Graphics graph (iconImage);

        if (isDirectory)
        {

           GUIHelpers::drawFromSvg (graph,
                                    BinaryData::folder_svg,
                                    "#bbbbbb",
                                    w,
                                    h);
        }
        else if (filename.endsWith (".wav"))
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::filemusic_svg,
                                     "#bbbbbb",
                                     w,
                                     h);
        }

        else
        {
            GUIHelpers::drawFromSvg (graph,
                                     BinaryData::file_svg,
                                     "#bbbbbb",
                                     w,
                                     h);
        }
        icon = &iconImage;
//        juce::LookAndFeel_V4::drawFileBrowserRow (g, width, height, file, filename, icon,
//                                                                  fileSizeDescription, fileTimeDescription,
//                                                                  isDirectory, isItemSelected, itemIndex, dcc);
        auto fileListComp = dynamic_cast<Component*> (&dcc);

        if (isItemSelected)
            g.fillAll (fileListComp != nullptr ? fileListComp->findColour (DirectoryContentsDisplayComponent::highlightColourId)
                                               : findColour (DirectoryContentsDisplayComponent::highlightColourId));

        const int x = 32;
        g.setColour (Colours::black);

        if (icon != nullptr && icon->isValid())
        {
            g.drawImageWithin (*icon, 2, 2, x - 4, height - 4,
                               RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
                               false);
        }
        else
        {
            if (auto* d = isDirectory ? getDefaultFolderImage()
                                      : getDefaultDocumentFileImage())
                d->drawWithin (g, Rectangle<float> (2.0f, 2.0f, x - 4.0f, height - 4.0f),
                               RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
        }

        if (isItemSelected)
            g.setColour (fileListComp != nullptr ? fileListComp->findColour (DirectoryContentsDisplayComponent::highlightedTextColourId)
                                                 : findColour (DirectoryContentsDisplayComponent::highlightedTextColourId));
        else
            g.setColour (fileListComp != nullptr ? fileListComp->findColour (DirectoryContentsDisplayComponent::textColourId)
                                                 : findColour (DirectoryContentsDisplayComponent::textColourId));

        g.setFont (height * 0.7f);

        if (width > 450 && ! isDirectory)
        {
            auto sizeX = roundToInt (width * 0.7f);
            auto dateX = roundToInt (width * 0.8f);

            g.drawFittedText (filename,
                              x, 0, sizeX - x, height,
                              Justification::centredLeft, 1 , 1.0f);

            g.setFont (height * 0.5f);
            g.setColour (Colours::darkgrey);

            if (! isDirectory)
            {
                g.drawFittedText (fileSizeDescription,
                                  sizeX, 0, dateX - sizeX - 8, height,
                                  Justification::centredRight, 1, 1.0f);

                g.drawFittedText (fileTimeDescription,
                                  dateX, 0, width - 8 - dateX, height,
                                  Justification::centredRight, 1, 1.0f);
            }
        }
        else
        {
            g.drawFittedText (filename,
                              x, 0, width - x, height,
                              Justification::centredLeft, 1, 1.0f);

        }
    }




    void drawScrollbar (Graphics &g,
                       ScrollBar &,
                       int x, int y,
                       int width, int height,
                       bool isScrollbarVertical,
                       int thumbStartPosition,
                       int thumbSize,
                       bool isMouseOver,
                       bool isMouseDown) override
    {
        if (isMouseDown)
        {
            g.setColour (juce::Colour(0x66ffffff));
            g.fillRect (Rectangle<int> (0, thumbStartPosition, width, thumbSize));
        }
        else if (isMouseOver)
        {
            g.setColour (juce::Colour(0x33ffffff));
            g.fillRect (Rectangle<int> (0, thumbStartPosition, width, thumbSize));
        }
    }
    Colour m_BGcolour = Colour(14, 14, 14);
    Colour m_DarkArea = Colour(10, 10, 10);
    Colour m_BrightButton1 = Colour(90, 90, 90);
    Colour m_BrightButton2 = Colour(66, 66, 66);
};

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

    void drawToggleButton(Graphics& g,
                          ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
    {
        auto buttonArea = button.getLocalBounds();
        buttonArea.reduce(2, 2);
        g.setColour(Colour(0xff000000));
        g.fillRoundedRectangle(buttonArea.toFloat(), 7);
        g.setColour(Colour(0xff343434));
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 5);
        if (button.getToggleState())
        {
            if (button.getComponentID() == "solo")
            {
                g.setColour(Colours::green);
            }
            else if (button.getComponentID() == "mute")
            {
                g.setColour(Colours::gold);
            }
            else
            {
                g.setColour(Colours::aqua);
            }
        }
        else
        {
            g.setColour(Colour(14, 14, 14));
        }
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 5);
        g.setColour(Colours::white);
        //        auto fontSize = jmin (15.0f, button.getHeight() * 0.75f);
        //        auto tickWidth = fontSize * 1.1f;
        g.drawFittedText(
            button.getName(), button.getLocalBounds(), Justification::centred, 1);
    }
//    void drawFileBrowserRow(Graphics &,
//                            int width,
//                            int height,
//                            const File &file,
//                            const String &filename,
//                            Image *icon,
//                            const String &fileSizeDescription,
//                            const String &fileTimeDescription,
//                            bool isDirectory, bool isItemSelected,
//                            int itemIndex,
//                            DirectoryContentsDisplayComponent &) override
//    {

//    }
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

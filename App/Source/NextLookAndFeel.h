/*
  ==============================================================================

    NextLookAndFeel.h
    Created: 27 Dec 2019 11:29:28pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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
        auto buttonArea = button.getLocalBounds();

        g.setColour(Colour(0xff000000));
        g.fillRoundedRectangle(buttonArea.toFloat(), 4);
        g.setGradientFill({Colour(0xff9b9b9b),
                           0,
                           0,
                           Colour(0xff4b4b4b),
                           0,
                           static_cast<float>(buttonArea.getHeight()),
                           false});
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 3);
        g.setColour(Colour(0xff5b5b5b));
        g.setGradientFill({Colour(0xff7b7b7b),
                           0,
                           0,
                           Colour(0xff4b4b4b),
                           0,
                           static_cast<float>(buttonArea.getHeight()),
                           false});
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(), 2);
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

        auto sliderColour = Colour(0xff2c2c2c);
        auto thumbColour = Colour(0xff9b9b9b);
        auto volumeColour = Colour(0xff04e9e9);
        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto lineW = 2;
        auto arcRadius = radius + 3;
        auto toAngle =
            rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        // fill
        g.setColour(sliderColour);
        g.fillEllipse(rx, ry, rw, rw);

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
            PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

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
                                        PathStrokeType::rounded));
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
    Colour m_BGcolour = Colour(14, 14, 14);
    Colour m_DarkArea = Colour(10, 10, 10);
    Colour m_BrightButton1 = Colour(90, 90, 90);
    Colour m_BrightButton2 = Colour(66, 66, 66);
};

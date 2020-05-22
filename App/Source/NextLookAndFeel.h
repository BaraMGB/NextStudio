/*
  ==============================================================================

    NextLookAndFeel.h
    Created: 27 Dec 2019 11:29:28pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"
class NextLookAndFeel : public LookAndFeel_V4
{
public:
    NextLookAndFeel()
    {
        setColour(ResizableWindow::backgroundColourId, Colour(0xff242424));
        
    }
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto buttonArea = button.getLocalBounds();

        g.setColour(Colour(0xff000000));
        g.fillRoundedRectangle(buttonArea.toFloat(),7);
        g.setColour(Colour(0xff343434));
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(),5);
        g.setColour(Colour(0xff5b5b5b));
        buttonArea.reduce(1, 1);
        g.fillRoundedRectangle(buttonArea.toFloat(),5);


        

    }

 
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
    {
        auto radius = jmin(width / 2, height / 2) - 10.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto sliderColour = Colour(0xff2c2c2c);
        auto thumbColour = Colour(0xff9b9b9b);
        auto volumeColour = Colour(0xff04e9e9);
        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto lineW = 2;
        auto arcRadius = radius+3;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
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

        
        g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

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
            g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
        }

        Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
        // pointer
        g.setColour(thumbColour);
        g.fillPath(p);
    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return { jmin(10.0f, buttonHeight * 0.6f) };
    }

    Colour m_BGcolour = Colour(14, 14, 14);
    Colour m_DarkArea = Colour(10, 10, 10);
    Colour m_BrightButton1 = Colour(90, 90, 90);
    Colour m_BrightButton2 = Colour(66, 66, 66);
};

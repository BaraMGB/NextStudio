#include "LevelMeterComponent.h"


LevelMeterComponent::LevelMeterComponent(te::LevelMeasurer &lm) : levelMeasurer(lm)
{
    setOpaque(true);
    levelMeasurer.addClient(levelClient);
    startTimerHz(30);
}

LevelMeterComponent::~LevelMeterComponent()
{
    levelMeasurer.removeClient(levelClient);
    stopTimer();
}

void LevelMeterComponent::paint(Graphics &g)
{
    g.fillAll(Colour(0xff000000));// fill the background black

    const double meterHeight{ double(getHeight()) };
    const double meterWidth{ double(getWidth()) };
    const double offSet{ fabs(RANGEMINdB) };
    const double scaleFactor{ meterHeight / (RANGEMAXdB + offSet) };

    // now we calculate and draw our 0dB line
    g.setColour(Colours::lightgrey);  // set line color
    g.fillRect(0.0f, float(meterHeight - (offSet * scaleFactor)), float(meterWidth), 1.0f);

    // draw meter Gain bar
    g.setColour(Colours::green);
    auto displayBarHeight = ((currentLeveldB + offSet) * scaleFactor);
    if (displayBarHeight > 0)
    {
        g.fillRect(0.0f, float(meterHeight - displayBarHeight), float(meterWidth), float(displayBarHeight));
    }
}

void LevelMeterComponent::timerCallback()
{
    prevLeveldB = currentLeveldB;

    currentLeveldB = levelClient.getAndClearAudioLevel(0).dB;

    // Now we give the level bar fading charcteristics.
    // And, the below coversions, decibelsToGain and gainToDecibels,
    // take care of 0dB, which will never fade!...but a gain of 1.0 (0dB) will.

    const auto prevLevel{ Decibels::decibelsToGain(prevLeveldB) };

    if (prevLeveldB > currentLeveldB)
        currentLeveldB = Decibels::gainToDecibels(prevLevel * 0.94);

    // the test below may save some unnecessary paints
    if(currentLeveldB != prevLeveldB)
        repaint();
}

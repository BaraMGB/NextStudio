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


    // draw meter Gain bar
    auto lineAtNullDb = float(meterHeight - (offSet * scaleFactor));
    auto displayBarHeightLeft = ((currentLeveldBLeft + offSet) * scaleFactor);
    auto displayBarHeightRight = ((currentLeveldBRight + offSet) * scaleFactor);


    if (float(meterHeight - displayBarHeightLeft) <= lineAtNullDb
        || float(meterHeight - displayBarHeightRight <= lineAtNullDb) )
    {
        g.setColour (Colours::red);
    }
    else
    {
        g.setColour(Colour(0xff74bb00));
    }

    if (displayBarHeightLeft > 0 || displayBarHeightRight > 0)
    {
        g.fillRect(0.0f, float(meterHeight - displayBarHeightLeft), float(meterWidth * 0.45), float(displayBarHeightLeft));
        g.fillRect(float(meterWidth * 0.55), float(meterHeight - displayBarHeightRight), float(meterWidth), float(displayBarHeightRight));
    }
    // now we calculate and draw our 0dB line
    g.setColour(Colours::lightgrey);  // set line color
    g.fillRect(0.0f, lineAtNullDb , float(meterWidth), 1.0f);
}

void LevelMeterComponent::timerCallback()
{
    prevLeveldBLeft = currentLeveldBLeft;
    prevLeveldBRight = currentLeveldBRight;
    currentLeveldBLeft = levelClient.getAndClearAudioLevel(0).dB;
    currentLeveldBRight = levelClient.getAndClearAudioLevel(1).dB;

    // Now we give the level bar fading charcteristics.
    // And, the below coversions, decibelsToGain and gainToDecibels,
    // take care of 0dB, which will never fade!...but a gain of 1.0 (0dB) will.

    const auto prevLevelLeft{ Decibels::decibelsToGain(prevLeveldBLeft) };
    const auto prevLevelRight{ Decibels::decibelsToGain(prevLeveldBRight) };

    if (prevLeveldBLeft > currentLeveldBLeft)
    {
        currentLeveldBLeft = Decibels::gainToDecibels(prevLevelLeft * 0.94);
    }

    if (prevLeveldBRight > currentLeveldBRight)
    {
        currentLeveldBRight = Decibels::gainToDecibels(prevLevelRight * 0.94);
    }

    // the test below may save some unnecessary paints
    if((currentLeveldBLeft != prevLeveldBLeft) || currentLeveldBRight != prevLevelRight)
    {
        repaint();
    }
}

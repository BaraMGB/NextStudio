#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class LevelMeterComponent : public juce::Component, public juce::Timer
{
public:
        LevelMeterComponent(te::LevelMeasurer& lm);

        ~LevelMeterComponent();

        void paint(juce::Graphics& g) override;

private:
        void timerCallback();

        // set the range of the meter in dB
        const double RANGEMAXdB{ 3.0 };//+3dB
        const double RANGEMINdB{ -30.0 };//-30dB

        double currentLeveldBLeft{ 0.0 };
        double prevLeveldBLeft{ 0.0 };
        double currentLeveldBRight{ 0.0 };
        double prevLeveldBRight{ 0.0 };

        te::LevelMeasurer& levelMeasurer;
        te::LevelMeasurer::Client levelClient;
};

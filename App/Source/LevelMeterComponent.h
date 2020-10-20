#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
namespace te = tracktion_engine;
class LevelMeterComponent : public Component, public Timer
{
public:
        LevelMeterComponent(te::LevelMeasurer& lm);

        ~LevelMeterComponent();

        void paint(Graphics& g) override;

private:
        void timerCallback();

        // set the range of the meter in dB
        const double RANGEMAXdB{ 3.0 };//+3dB
        const double RANGEMINdB{ -30.0 };//-30dB

        double currentLeveldB{ 0.0 };
        double prevLeveldB{ 0.0 };

        te::LevelMeasurer& levelMeasurer;
        te::LevelMeasurer::Client levelClient;
};

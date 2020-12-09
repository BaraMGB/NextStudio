#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class LevelMeterComponent : public juce::Component
                          , public juce::Timer
{
public:
        LevelMeterComponent(te::LevelMeasurer& lm);
        ~LevelMeterComponent();

        void paint(juce::Graphics& g) override;

private:
        void timerCallback() override;

        // set the range of the meter in dB
        const double RANGEMAXdB{ 3.0 };//+3dB
        const double RANGEMINdB{ -30.0 };//-30dB

        double m_currentLeveldBLeft{ 0.0 };
        double m_prevLeveldBLeft{ 0.0 };
        double m_currentLeveldBRight{ 0.0 };
        double m_prevLeveldBRight{ 0.0 };

        te::LevelMeasurer& m_levelMeasurer;
        te::LevelMeasurer::Client m_levelClient;
};

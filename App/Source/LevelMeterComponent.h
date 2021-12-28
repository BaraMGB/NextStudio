#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class LevelMeterComponent : public juce::Component
                          , public juce::Timer
{
public:
        explicit LevelMeterComponent(te::LevelMeasurer& lm);
        ~LevelMeterComponent() override;

        void paint(juce::Graphics& g) override;

private:
        void timerCallback() override;

        const double RANGEMAXdB{ 3.0 };
        const double RANGEMINdB{ -30.0 };

        double m_currentLeveldBLeft{ 0.0 };
        double m_prevLeveldBLeft{ 0.0 };
        double m_currentLeveldBRight{ 0.0 };
        double m_prevLeveldBRight{ 0.0 };

        te::LevelMeasurer& m_levelMeasurer;
        te::LevelMeasurer::Client m_levelClient;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterComponent)
};

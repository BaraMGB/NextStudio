
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableSliderComponent : public juce::Slider
                                 , public te::AutomatableParameter::Listener
                                 , private FlaggedAsyncUpdater
{
public:

    explicit AutomatableSliderComponent(const te::AutomatableParameter::Ptr& ap);
    ~AutomatableSliderComponent() override;
    void mouseDown (const juce::MouseEvent& e) override;

    void valueChanged() override;

    te::AutomatableParameter::Ptr getAutomatableParameter();

    void curveHasChanged(tracktion_engine::AutomatableParameter &) override;
    void currentValueChanged(tracktion_engine::AutomatableParameter &, float) override;
    void parameterChanged(tracktion_engine::AutomatableParameter &, float) override;

    void handleAsyncUpdate() override;

    [[nodiscard]] juce::Colour getTrackColour() const;

private:

    te::AutomatableParameter::Ptr m_automatableParameter;
    juce::Colour m_trackColour;
    bool m_updateSlider {false};
    double m_cachedValue {0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableSliderComponent)
};

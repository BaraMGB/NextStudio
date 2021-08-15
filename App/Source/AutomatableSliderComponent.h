#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableSliderComponent : public juce::Slider
                                 , public te::AutomatableParameter::Listener
                                 , private FlaggedAsyncUpdater
{
public:
    AutomatableSliderComponent(te::AutomatableParameter::Ptr ap);
    ~AutomatableSliderComponent();
    void mouseDown (const juce::MouseEvent& e) override;

    void valueChanged();
    te::AutomatableParameter::Ptr getAutomatableParameter()
    {
        return m_automatableParameter;
    }
private:
    te::AutomatableParameter::Ptr m_automatableParameter;
    juce::Colour m_trackColour;
    bool m_updateSlider {false};
    double m_cachedValue {0.0};

    // Listener interface
public:
    void curveHasChanged(tracktion_engine::AutomatableParameter &);
    void currentValueChanged(tracktion_engine::AutomatableParameter &, float);
    void parameterChanged(tracktion_engine::AutomatableParameter &, float);

    // AsyncUpdater interface
public:
    void handleAsyncUpdate();
    const juce::Colour trackColour() const;
};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;
/**
     Wraps a te::AutomatableParameter as a juce::ValueSource so it can be used as
     a Value for example in a Slider.
 */
 class ParameterValueSource  : public juce::Value::ValueSource,
                               private te::AutomatableParameter::Listener
 {
 public:
     ParameterValueSource (te::AutomatableParameter::Ptr p)
         : param (p)
     {
         param->addListener (this);
     }
     
     ~ParameterValueSource() override
     {
         param->removeListener (this);
     }
     
     juce::var getValue() const override
     {
         return param->getCurrentValue();
     }
 
     void setValue (const juce::var& newValue) override
     {
         param->setParameter (static_cast<float> (newValue), juce::sendNotification);
     }
 
 private:
     te::AutomatableParameter::Ptr param;
     
     void curveHasChanged (te::AutomatableParameter&) override
     {
         sendChangeMessage (false);
     }
     
     void currentValueChanged (te::AutomatableParameter&, float /*newValue*/) override
     {
         sendChangeMessage (false);
     }
 };


class AutomatableSliderComponent : public juce::Slider
{
public:

    explicit AutomatableSliderComponent(const te::AutomatableParameter::Ptr& ap);
    ~AutomatableSliderComponent() override;
    void mouseDown (const juce::MouseEvent& e) override;


    te::AutomatableParameter::Ptr getAutomatableParameter();


    void bindSliderToParameter ();

    [[nodiscard]] juce::Colour getTrackColour() const;

private:

    te::AutomatableParameter::Ptr m_automatableParameter;
    juce::Colour m_trackColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomatableSliderComponent)
};

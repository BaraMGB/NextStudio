#include "AutomatableSliderComponent.h"




AutomatableSliderComponent::AutomatableSliderComponent(const tracktion_engine::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    bindSliderToParameter();
    if (auto t = m_automatableParameter->getTrack())
        m_trackColour = t->getColour();
}

AutomatableSliderComponent::~AutomatableSliderComponent()
{
}

void AutomatableSliderComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        if (m_automatableParameter->getCurve().getNumPoints() == 0)
        {
            m.addItem(2000, "Add automation lane");
        }
        const int result = m.show();
        if (result == 2000)
        {
            m_automatableParameter->getCurve().addPoint(0.0, (float) getValue(), 0.0);
            m_automatableParameter->getTrack()->state.setProperty(
                        IDs::isTrackMinimized
                        , false
                        , nullptr);

        }
    }
    else
        juce::Slider::mouseDown(e);
}

juce::Colour AutomatableSliderComponent::getTrackColour() const
{
    return m_trackColour;
}

te::AutomatableParameter::Ptr AutomatableSliderComponent::getAutomatableParameter()
{
    return m_automatableParameter;
}

void AutomatableSliderComponent::bindSliderToParameter ()
{
    const auto v = m_automatableParameter->valueRange;
    const auto range = juce::NormalisableRange<double> (static_cast<double> (v.start),
                                                  static_cast<double> (v.end),
                                                  static_cast<double> (v.interval),
                                                  static_cast<double> (v.skew),
                                                  v.symmetricSkew);
 
    setNormalisableRange (range);
    getValueObject().referTo (juce::Value (new ParameterValueSource (m_automatableParameter)));
}


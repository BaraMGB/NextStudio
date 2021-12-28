#include "AutomatableSliderComponent.h"




AutomatableSliderComponent::AutomatableSliderComponent(const tracktion_engine::AutomatableParameter::Ptr& ap)
    : m_automatableParameter(ap)
    , m_trackColour(ap->getTrack()->getColour())
{
    m_automatableParameter->addListener(this);
    setValue(m_automatableParameter->getCurrentValue());
}

AutomatableSliderComponent::~AutomatableSliderComponent()
{
    m_automatableParameter->removeListener(this);
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

void AutomatableSliderComponent::valueChanged()
{
    m_automatableParameter->setParameter((float)getValue()
                                             , juce::dontSendNotification);
}

juce::Colour AutomatableSliderComponent::getTrackColour() const
{
    return m_trackColour;
}

void AutomatableSliderComponent::curveHasChanged(tracktion_engine::AutomatableParameter &)
{
}

void AutomatableSliderComponent::currentValueChanged(tracktion_engine::AutomatableParameter &, float v)
{
    m_cachedValue = v;
    markAndUpdate(m_updateSlider);
}

void AutomatableSliderComponent::parameterChanged(tracktion_engine::AutomatableParameter &, float)
{
}

void AutomatableSliderComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_updateSlider))
    {
        if (m_automatableParameter->isAutomationActive())
        {
            setValue(m_cachedValue, juce::dontSendNotification);
        }
        else
        {
            setValue(m_cachedValue);
        }
    }
}

te::AutomatableParameter::Ptr AutomatableSliderComponent::getAutomatableParameter()
{
    return m_automatableParameter;
}

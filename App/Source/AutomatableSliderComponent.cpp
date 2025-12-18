
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/


#include "AutomatableSliderComponent.h"




AutomatableSliderComponent::AutomatableSliderComponent(const tracktion_engine::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
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
            auto start = tracktion::core::TimePosition::fromSeconds(0.0);
            m_automatableParameter->getCurve().addPoint(start, (float) getValue(), 0.0);
            m_automatableParameter->getTrack()->state.setProperty(IDs::isTrackMinimized, false, nullptr);

        }
    }
    else
        juce::Slider::mouseDown(e);
}

void AutomatableSliderComponent::setTrackColour(juce::Colour colour)
{
    m_trackColour = colour;
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

bool AutomatableSliderComponent::hasAnAutomatableParameter()
{
    return m_automatableParameter != nullptr;
}

void AutomatableSliderComponent::chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam,
                                                             std::function<void()> /*startLearnMode*/)
{
    if (handleChosenParam && m_automatableParameter)
        handleChosenParam(m_automatableParameter);
}


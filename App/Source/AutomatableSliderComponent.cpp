
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
#include "NextLookAndFeel.h"



AutomatableSliderComponent::AutomatableSliderComponent(const tracktion_engine::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
    bindSliderToParameter();
    m_automatableParameter->addListener(this);
    if (auto t = m_automatableParameter->getTrack())
        m_trackColour = t->getColour();

    m_modDepthSlider.setSliderStyle(juce::Slider::LinearBar);
    m_modDepthSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    m_modDepthSlider.setRange(-1.0, 1.0);
    m_modDepthSlider.setValue(1.0, juce::dontSendNotification);
    m_modDepthSlider.setVisible(false);

    // Theme colors
    auto thumbColour = juce::Colours::orange;
    auto trackColour = juce::Colours::darkgrey;
    auto bgColour = juce::Colours::black.withAlpha(0.5f);

    if (auto* lnf = dynamic_cast<NextLookAndFeel*>(&getLookAndFeel()))
    {
        thumbColour = lnf->getPrimeColour();
        trackColour = lnf->getBackgroundColour2();
        bgColour = lnf->getBackgroundColour1().withAlpha(0.5f);
    }

    m_modDepthSlider.setColour(juce::Slider::thumbColourId, thumbColour);
    m_modDepthSlider.setColour(juce::Slider::trackColourId, trackColour);
    m_modDepthSlider.setColour(juce::Slider::backgroundColourId, bgColour);

    m_modDepthSlider.setAlpha(0.7f);
    addChildComponent(m_modDepthSlider);
    m_modDepthSlider.addMouseListener(this, false);

    m_modDepthSlider.onValueChange = [this]
    {
        auto assignments = m_automatableParameter->getAssignments();
        if (!assignments.isEmpty())
        {
             auto& ass = *assignments[0];
             ass.value = (float)m_modDepthSlider.getValue();
             repaint();
        }
    };

    updateModDepthVisibility();
}

AutomatableSliderComponent::~AutomatableSliderComponent()
{
    m_modDepthSlider.removeMouseListener(this);
    m_automatableParameter->removeListener(this);
}

void AutomatableSliderComponent::mouseEnter (const juce::MouseEvent&)
{
    updateModDepthVisibility();
}

void AutomatableSliderComponent::mouseExit (const juce::MouseEvent&)
{
    updateModDepthVisibility();
}

void AutomatableSliderComponent::resized()
{
    juce::Slider::resized();
    m_modDepthSlider.setBounds(getLocalBounds().removeFromBottom(10));
}

void AutomatableSliderComponent::updateModDepthVisibility()
{
    auto assignments = m_automatableParameter->getAssignments();

    // Check if mouse is over this component or the depth slider itself
    bool mouseIsOver = isMouseOver(true) || m_modDepthSlider.isMouseOverOrDragging();
    bool shouldBeVisible = !assignments.isEmpty() && mouseIsOver;

    if (shouldBeVisible)
    {
        auto& ass = *assignments[0];
        if (!m_modDepthSlider.isMouseButtonDown())
             m_modDepthSlider.setValue(ass.value.get(), juce::dontSendNotification);

        m_modDepthSlider.setVisible(true);
        m_modDepthSlider.toFront(false);
    }
    else
    {
        // Don't hide if we are currently dragging the depth slider
        if (!m_modDepthSlider.isMouseButtonDown())
            m_modDepthSlider.setVisible(false);
    }
}

void AutomatableSliderComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;

        auto assignments = m_automatableParameter->getAssignments();
        if (!assignments.isEmpty())
        {
            juce::PopupMenu modifierMenu;
            int itemId = 1;

            auto* track = m_automatableParameter->getTrack();
            if (auto* modifierList = track != nullptr ? track->getModifierList() : nullptr)
            {
                for (auto* modifier : modifierList->getModifiers())
                {
                    for (auto& assignment : assignments)
                    {
                        if (assignment->isForModifierSource(*modifier))
                        {
                            juce::String modifierName = modifier->getName();
                            if (modifierName.isEmpty())
                                modifierName = "Modifier";

                            modifierMenu.addItem(itemId++, modifierName);
                            break;
                        }
                    }
                }
            }

            m.addSubMenu("Remove Modifier", modifierMenu);
        }

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
        else if (result >= 1)
        {
            int index = result - 1;
            if (index >= 0 && index < assignments.size())
            {
                m_automatableParameter->removeModifier(*assignments[index]);
            }
        }
    }
    else if (e.originalComponent != &m_modDepthSlider)
    {
        juce::Slider::mouseDown(e);
    }
}

void AutomatableSliderComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.originalComponent != &m_modDepthSlider)
        juce::Slider::mouseDrag(e);
}

void AutomatableSliderComponent::mouseUp(const juce::MouseEvent& e)
{
    if (e.originalComponent != &m_modDepthSlider)
        juce::Slider::mouseUp(e);
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
    setValue(m_automatableParameter->getCurrentBaseValue(), juce::dontSendNotification);
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

void AutomatableSliderComponent::curveHasChanged(te::AutomatableParameter&)
{
}

void AutomatableSliderComponent::currentValueChanged(te::AutomatableParameter&)
{
    if (m_automatableParameter)
    {
         double newVal = m_automatableParameter->getCurrentBaseValue();

         if (!isMouseButtonDown() && std::abs(getValue() - newVal) > 0.0001)
             setValue(newVal, juce::dontSendNotification);

         repaint();
    }
}

void AutomatableSliderComponent::startedDragging()
{
    if (m_automatableParameter)
        m_automatableParameter->beginParameterChangeGesture();
}

void AutomatableSliderComponent::stoppedDragging()
{
    if (m_automatableParameter)
        m_automatableParameter->endParameterChangeGesture();
}

void AutomatableSliderComponent::valueChanged()
{
    if (m_automatableParameter)
    {
        float val = static_cast<float>(getValue());
        m_automatableParameter->setParameter(val, juce::sendNotification);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------------

AutomatableParameterComponent::AutomatableParameterComponent(const te::AutomatableParameter::Ptr ap, juce::String name)
    : m_automatableParameter (ap)
{
    m_knob = std::make_unique<AutomatableSliderComponent>(ap);
    m_knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_knob->setDoubleClickReturnValue(true, ap->getCurrentValue());
    m_knob->onValueChange = [this] {
        updateLabel();
    };

    addAndMakeVisible(*m_knob);

    m_valueLabel.setJustificationType(juce::Justification::centredTop);
    m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    updateLabel();
    addAndMakeVisible(m_valueLabel);

    m_titleLabel.setJustificationType(juce::Justification::centredBottom);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    m_titleLabel.setText(name, juce::dontSendNotification);

    addAndMakeVisible(m_titleLabel);

    m_automatableParameter->addListener(this);
}

AutomatableParameterComponent::~AutomatableParameterComponent()
{
    m_automatableParameter->removeListener(this);
}
void AutomatableParameterComponent::resized() 
{
    auto area = getLocalBounds();
    auto h = area.getHeight() / 4;

    m_titleLabel.setBounds(area.removeFromTop(h));
    m_knob->setBounds(area.removeFromTop(h * 2));
    m_valueLabel.setBounds(area.removeFromTop(h));
}

void AutomatableParameterComponent::setKnobColour (juce::Colour colour)
{
    m_knob->setTrackColour(colour);
    repaint();
}

void AutomatableParameterComponent::updateLabel()
{
    m_valueLabel.setText(m_automatableParameter->getCurrentValueAsString(), juce::NotificationType::dontSendNotification);
}

// ---------------------------------------------------------------------------------------------------------------------------------
NonAutomatableParameterComponent::NonAutomatableParameterComponent(juce::Value v, juce::String name, int rangeStart, int rangeEnd)
{
    m_knob.setRange(rangeStart, rangeEnd, 1); 
    m_knob.getValueObject().referTo(v);
    m_titleLabel.setText(name, juce::dontSendNotification);
    m_knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);

    m_knob.onValueChange = [this] { updateLabel(); };
    m_valueLabel.setJustificationType(juce::Justification::centredTop);
    m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    updateLabel();

    m_titleLabel.setJustificationType(juce::Justification::centredBottom);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));

    Helpers::addAndMakeVisible(*this,{&m_titleLabel, &m_knob, &m_valueLabel});
}

void NonAutomatableParameterComponent::resized()
{
    auto area = getLocalBounds();
    auto h = area.getHeight() / 4;

    m_titleLabel.setBounds(area.removeFromTop(h));
    m_knob.setBounds(area.removeFromTop(h * 2));
    m_valueLabel.setBounds(area.removeFromTop(h));
}

void NonAutomatableParameterComponent::updateLabel()
{
    m_valueLabel.setText(juce::String(m_knob.getValue()), juce::dontSendNotification);
}

void NonAutomatableParameterComponent::setSliderRange(double start, double end, double interval)
{
    m_knob.setRange(start, end, interval);
}

// ---------------------------------------------------------------------------------------------------------------------------------

AutomatableComboBoxComponent::AutomatableComboBoxComponent(te::AutomatableParameter::Ptr ap)
: m_automatableParameter(ap)
{
    ap->addListener(this);
    auto range = ap->valueRange;
    int index = 1;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    for (float v = range.start; v <= range.end + 0.001f; v += step)
    {
        juce::String text;
        if (ap->hasLabels())
            text = ap->getLabelForValue(v);

        if (text.isEmpty())
            text = ap->valueToString(v);

        addItem(text, index++);
    }

    // Initial update
    currentValueChanged(*ap);

    onChange = [this]
        {
            int index = getSelectedId();
            if (index > 0)
            {
                auto range = m_automatableParameter->valueRange;
                float step = range.interval > 0.0f ? range.interval : 1.0f;
                float newVal = range.start + ((float)(index - 1) * step);

                if (m_automatableParameter->getCurrentValue() != newVal)
                    m_automatableParameter->setParameter(newVal, juce::sendNotification);
            }
        };
}
AutomatableComboBoxComponent::~AutomatableComboBoxComponent()
{
    m_automatableParameter->removeListener(this);
}

void AutomatableComboBoxComponent::currentValueChanged(te::AutomatableParameter& p)
{
    auto range = p.valueRange;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    int id = (int)std::round((p.getCurrentValue() - range.start) / step) + 1;

    if (getSelectedId() != id)
        setSelectedId(id, juce::dontSendNotification);
}

void AutomatableComboBoxComponent::chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/)
{
    handleChosenParam(m_automatableParameter);
}


// ---------------------------------------------------------------------------------------------------------------------------------

AutomatableChoiceComponent::AutomatableChoiceComponent(te::AutomatableParameter::Ptr  ap, juce::String name)
{
    m_combo = std::make_unique<AutomatableComboBoxComponent>(ap);
    addAndMakeVisible(*m_combo);

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    m_titleLabel.setText(name, juce::dontSendNotification);
    addAndMakeVisible(m_titleLabel);
}

void AutomatableChoiceComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(area.getWidth() / 5, 0);
    auto labelHeight = 15;
    auto h = 30;
    m_titleLabel.setBounds(area.removeFromTop(labelHeight));
    m_combo->setBounds(area.removeFromTop(h));
}

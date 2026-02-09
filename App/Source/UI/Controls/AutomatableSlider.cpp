/*
  ==============================================================================

    AutomatableSlider.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "UI/Controls/AutomatableSlider.h"
#include "Utilities/NextLookAndFeel.h"

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

    if (auto *lnf = dynamic_cast<NextLookAndFeel *>(&getLookAndFeel()))
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

    if (auto def = m_automatableParameter->getDefaultValue())
        setDoubleClickReturnValue(true, *def);

    m_modDepthSlider.onValueChange = [this]
    {
        auto assignments = m_automatableParameter->getAssignments();
        if (!assignments.isEmpty())
        {
            auto &ass = *assignments[0];
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

void AutomatableSliderComponent::mouseEnter(const juce::MouseEvent &) { updateModDepthVisibility(); }

void AutomatableSliderComponent::mouseExit(const juce::MouseEvent &) { updateModDepthVisibility(); }

void AutomatableSliderComponent::enablementChanged() { setAlpha(isEnabled() ? 1.0f : 0.5f); }

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
        auto &ass = *assignments[0];
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

            auto *track = m_automatableParameter->getTrack();
            if (auto *modifierList = track != nullptr ? track->getModifierList() : nullptr)
            {
                for (auto *modifier : modifierList->getModifiers())
                {
                    for (auto &assignment : assignments)
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
        else
        {
            m.addItem(2001, "Clear automation");
        }

        const int result = m.show();

        if (result == 2000)
        {
            auto start = tracktion::core::TimePosition::fromSeconds(0.0);
            auto &um = m_automatableParameter->getEdit().getUndoManager();
            m_automatableParameter->getCurve().addPoint(start, (float)getValue(), 0.0);

            if (auto *track = m_automatableParameter->getTrack())
                track->state.setProperty(IDs::isTrackMinimized, false, &um);
        }
        else if (result == 2001)
        {
            m_automatableParameter->getCurve().clear();
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

void AutomatableSliderComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (e.originalComponent != &m_modDepthSlider)
        juce::Slider::mouseDrag(e);
}

void AutomatableSliderComponent::mouseUp(const juce::MouseEvent &e)
{
    if (e.originalComponent != &m_modDepthSlider)
        juce::Slider::mouseUp(e);
}

void AutomatableSliderComponent::setTrackColour(juce::Colour colour) { m_trackColour = colour; }
juce::Colour AutomatableSliderComponent::getTrackColour() const { return m_trackColour; }

te::AutomatableParameter::Ptr AutomatableSliderComponent::getAutomatableParameter() { return m_automatableParameter; }

void AutomatableSliderComponent::setParameter(te::AutomatableParameter::Ptr newParam)
{
    if (m_automatableParameter == newParam)
        return;

    if (m_automatableParameter)
        m_automatableParameter->removeListener(this);

    m_automatableParameter = newParam;

    if (m_automatableParameter)
    {
        m_automatableParameter->addListener(this);
        bindSliderToParameter();

        if (auto t = m_automatableParameter->getTrack())
            m_trackColour = t->getColour();

        // Reset double click value if available
        if (auto def = m_automatableParameter->getDefaultValue())
            setDoubleClickReturnValue(true, *def);
        else
            setDoubleClickReturnValue(false, 0.0);

        updateModDepthVisibility();
    }
    else
    {
        setEnabled(false);
    }

    repaint();
}

void AutomatableSliderComponent::bindSliderToParameter()
{
    const auto v = m_automatableParameter->valueRange;
    const auto range = juce::NormalisableRange<double>(static_cast<double>(v.start), static_cast<double>(v.end), static_cast<double>(v.interval), static_cast<double>(v.skew), v.symmetricSkew);

    setNormalisableRange(range);
    setValue(m_automatableParameter->getCurrentBaseValue(), juce::dontSendNotification);
}

bool AutomatableSliderComponent::hasAnAutomatableParameter() { return m_automatableParameter != nullptr; }

void AutomatableSliderComponent::chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/)
{
    if (handleChosenParam && m_automatableParameter)
        handleChosenParam(m_automatableParameter);
}

void AutomatableSliderComponent::curveHasChanged(te::AutomatableParameter &) {}

void AutomatableSliderComponent::currentValueChanged(te::AutomatableParameter &)
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

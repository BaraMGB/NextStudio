/*
  ==============================================================================

    AutomatableToggle.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableToggle.h"

AutomatableToggleButton::AutomatableToggleButton(te::AutomatableParameter::Ptr ap)
: m_automatableParameter(ap)
{
    ap->addListener(this);
    setButtonText(ap->getParameterName());
    
    // Initial update
    currentValueChanged(*ap);

    onClick = [this]
        {
            float newVal = getToggleState() ? 1.0f : 0.0f;
            if (m_automatableParameter->getCurrentValue() != newVal)
                m_automatableParameter->setParameter(newVal, juce::sendNotification);
        };
}

AutomatableToggleButton::~AutomatableToggleButton()
{
    m_automatableParameter->removeListener(this);
}

void AutomatableToggleButton::currentValueChanged(te::AutomatableParameter& p)
{
    bool shouldBeOn = p.getCurrentValue() > 0.5f;
    if (getToggleState() != shouldBeOn)
        setToggleState(shouldBeOn, juce::dontSendNotification);
}

void AutomatableToggleButton::chooseAutomatableParameter (std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/)
{
    handleChosenParam(m_automatableParameter);
}

void AutomatableToggleButton::mouseDown(const juce::MouseEvent &e)
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
        else
        {
            m.addItem(2001, "Clear automation");
        }

        const int result = m.show();

        if (result == 2000)
        {
            auto start = tracktion::core::TimePosition::fromSeconds(0.0);
            auto& um = m_automatableParameter->getTrack()->edit.getUndoManager();
            m_automatableParameter->getCurve().addPoint(start, (float) getToggleState(), 0.0);
            m_automatableParameter->getTrack()->state.setProperty(IDs::isTrackMinimized, false, &um);
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
    else
    {
        juce::ToggleButton::mouseDown(e);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------

AutomatableToggleComponent::AutomatableToggleComponent(te::AutomatableParameter::Ptr ap, juce::String name)
{
    m_button = std::make_unique<AutomatableToggleButton>(ap);
    m_button->setButtonText(""); // We use the label for the name
    addAndMakeVisible(*m_button);

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    m_titleLabel.setText(name, juce::dontSendNotification);
    addAndMakeVisible(m_titleLabel);
}

void AutomatableToggleComponent::resized()
{
    auto area = getLocalBounds();
    auto labelHeight = 15;
    m_titleLabel.setBounds(area.removeFromTop(labelHeight));
    
    // Center the toggle button
    auto buttonSize = 20;
    m_button->setBounds(area.withSizeKeepingCentre(buttonSize, buttonSize));
}

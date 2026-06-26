/*
  ==============================================================================

    AutomatableToggle.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableToggle.h"
#include "AutomationWriteGuard.h"

namespace
{
constexpr int removeModifierMenuBaseId = 1;
constexpr int addAutomationLaneMenuId = 2000;
constexpr int clearAutomationMenuId = 2001;
constexpr int midiLearnMenuId = 2100;
constexpr int removeMidiMappingMenuId = 2101;
}

AutomatableToggleButton::AutomatableToggleButton(te::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    setWantsKeyboardFocus(true);
    m_midiLearn = std::make_unique<AutomatableMidiLearnSupport>(*this, m_automatableParameter);
    ap->addListener(this);
    setButtonText(ap->getParameterName());

    currentValueChanged(*ap);

    onClick = [this]
    {
        float newVal = getToggleState() ? 1.0f : 0.0f;
        if (m_automatableParameter->getCurrentValue() != newVal)
        {
            AutomationWriteGuard::markTouched(m_automatableParameter);
            m_automatableParameter->setParameter(newVal, juce::sendNotification);
        }
    };
}

AutomatableToggleButton::~AutomatableToggleButton()
{
    m_midiLearn.reset();
    m_automatableParameter->removeListener(this);
}

void AutomatableToggleButton::currentValueChanged(te::AutomatableParameter &p)
{
    if (AutomationWriteGuard::isTouched(&p))
        return;

    bool shouldBeOn = p.getCurrentValue() > 0.5f;
    if (getToggleState() != shouldBeOn)
        setToggleState(shouldBeOn, juce::dontSendNotification);
}

void AutomatableToggleButton::parameterChanged(te::AutomatableParameter &p, float newValue)
{
    AutomationWriteGuard::markTouched(&p);

    bool shouldBeOn = newValue > 0.5f;
    if (getToggleState() != shouldBeOn)
        setToggleState(shouldBeOn, juce::dontSendNotification);
}

void AutomatableToggleButton::chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/) { handleChosenParam(m_automatableParameter); }

void AutomatableToggleButton::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;

        auto assignments = m_automatableParameter->getAssignments();
        if (!assignments.isEmpty())
        {
            juce::PopupMenu modifierMenu;
            int itemId = removeModifierMenuBaseId;

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

        m_midiLearn->addContextMenuItems(m, midiLearnMenuId, removeMidiMappingMenuId);
        m.addSeparator();

        if (m_automatableParameter->getCurve().getNumPoints() == 0)
        {
            m.addItem(addAutomationLaneMenuId, "Add automation lane");
        }
        else
        {
            m.addItem(clearAutomationMenuId, "Clear automation");
        }

        const int result = m.show();

        if (m_midiLearn->handleContextMenuResult(result, midiLearnMenuId, removeMidiMappingMenuId))
        {
        }
        else if (result == addAutomationLaneMenuId)
        {
            auto start = tracktion::core::TimePosition::fromSeconds(0.0);
            m_automatableParameter->getCurve().addPoint(start, (float)getToggleState(), 0.0);

            if (auto *track = m_automatableParameter->getTrack())
                track->state.setProperty(IDs::isTrackMinimized, false, nullptr);
        }
        else if (result == clearAutomationMenuId)
        {
            m_automatableParameter->getCurve().clear();
        }
        else if (result >= removeModifierMenuBaseId && result < midiLearnMenuId)
        {
            int index = result - removeModifierMenuBaseId;
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

void AutomatableToggleButton::mouseEnter(const juce::MouseEvent &e)
{
    m_midiLearn->refreshMappingState();
    repaint();
    juce::ToggleButton::mouseEnter(e);
}

void AutomatableToggleButton::mouseExit(const juce::MouseEvent &e)
{
    repaint();
    juce::ToggleButton::mouseExit(e);
}

void AutomatableToggleButton::paintOverChildren(juce::Graphics &g)
{
    juce::ToggleButton::paintOverChildren(g);
    m_midiLearn->paintOverChildren(g, getLocalBounds(), isMouseOver(true));
}

bool AutomatableToggleButton::keyPressed(const juce::KeyPress &key)
{
    if (m_midiLearn->keyPressed(key))
        return true;

    return juce::ToggleButton::keyPressed(key);
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
    m_titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(8); // Intermediate padding

    // Position Button
    auto buttonArea = area.removeFromTop(30);
    auto buttonSize = 20;
    m_button->setBounds(buttonArea.withSizeKeepingCentre(buttonSize, buttonSize));
}

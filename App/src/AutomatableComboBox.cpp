/*
  ==============================================================================

    AutomatableComboBox.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableComboBox.h"
#include "AutomationWriteGuard.h"

namespace
{
constexpr int midiLearnMenuId = 2100;
constexpr int removeMidiMappingMenuId = 2101;
}

AutomatableComboBoxComponent::AutomatableComboBoxComponent(te::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    setWantsKeyboardFocus(true);
    m_midiLearn = std::make_unique<AutomatableMidiLearnSupport>(*this, m_automatableParameter);
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
            {
                AutomationWriteGuard::markTouched(m_automatableParameter);
                m_automatableParameter->setParameter(newVal, juce::sendNotification);
            }
        }
    };
}
AutomatableComboBoxComponent::~AutomatableComboBoxComponent()
{
    m_midiLearn.reset();
    m_automatableParameter->removeListener(this);
}

void AutomatableComboBoxComponent::currentValueChanged(te::AutomatableParameter &p)
{
    if (AutomationWriteGuard::isTouched(&p))
        return;

    auto range = p.valueRange;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    int id = (int)std::round((p.getCurrentValue() - range.start) / step) + 1;

    if (getSelectedId() != id)
        setSelectedId(id, juce::dontSendNotification);
}

void AutomatableComboBoxComponent::parameterChanged(te::AutomatableParameter &p, float newValue)
{
    AutomationWriteGuard::markTouched(&p);

    auto range = p.valueRange;
    float step = range.interval > 0.0f ? range.interval : 1.0f;
    int id = (int)std::round((newValue - range.start) / step) + 1;

    if (getSelectedId() != id)
        setSelectedId(id, juce::dontSendNotification);
}

void AutomatableComboBoxComponent::chooseAutomatableParameter(std::function<void(te::AutomatableParameter::Ptr)> handleChosenParam, std::function<void()> /*startLearnMode*/) { handleChosenParam(m_automatableParameter); }

void AutomatableComboBoxComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        m_midiLearn->addContextMenuItems(menu, midiLearnMenuId, removeMidiMappingMenuId);
        m_midiLearn->handleContextMenuResult(menu.show(), midiLearnMenuId, removeMidiMappingMenuId);
        return;
    }

    juce::ComboBox::mouseDown(e);
}

void AutomatableComboBoxComponent::mouseEnter(const juce::MouseEvent &e)
{
    m_midiLearn->refreshMappingState();
    repaint();
    juce::ComboBox::mouseEnter(e);
}

void AutomatableComboBoxComponent::mouseExit(const juce::MouseEvent &e)
{
    repaint();
    juce::ComboBox::mouseExit(e);
}

void AutomatableComboBoxComponent::paintOverChildren(juce::Graphics &g)
{
    juce::ComboBox::paintOverChildren(g);
    m_midiLearn->paintOverChildren(g, getLocalBounds(), isMouseOver(true));
}

bool AutomatableComboBoxComponent::keyPressed(const juce::KeyPress &key)
{
    if (m_midiLearn->keyPressed(key))
        return true;

    return juce::ComboBox::keyPressed(key);
}

// ---------------------------------------------------------------------------------------------------------------------------------

AutomatableChoiceComponent::AutomatableChoiceComponent(te::AutomatableParameter::Ptr ap, juce::String name)
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
    m_titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(8); // Intermediate padding

    // Position ComboBox
    auto comboArea = area.removeFromTop(30);
    m_combo->setBounds(comboArea.withSizeKeepingCentre(area.getWidth() - 10, 30));
}

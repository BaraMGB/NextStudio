/*
  ==============================================================================

    NonAutomatableParameter.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "NonAutomatableParameter.h"
#include "DismissibleAlertWindow.h"

#include <cmath>

NonAutomatableParameterComponent::NonAutomatableParameterComponent(juce::Value v, juce::String name, int rangeStart, int rangeEnd)
{
    m_knob.setRange(rangeStart, rangeEnd, 1);
    m_knob.getValueObject().referTo(v);
    m_titleLabel.setText(name, juce::dontSendNotification);
    m_knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_knob.addMouseListener(this, false);

    m_knob.onValueChange = [this] { updateLabel(); };
    m_valueLabel.setJustificationType(juce::Justification::centred);
    m_valueLabel.setFont(juce::Font(juce::FontOptions{11.0f}));
    updateLabel();

    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::Font(juce::FontOptions{11.0f}));

    Helpers::addAndMakeVisible(*this, {&m_titleLabel, &m_knob, &m_valueLabel});
}

void NonAutomatableParameterComponent::resized()
{
    auto area = getLocalBounds();

    // Top: Title (Name)
    m_titleLabel.setBounds(area.removeFromTop(20));

    // Bottom: Value
    m_valueLabel.setBounds(area.removeFromBottom(15));

    // Rest: Knob
    m_knob.setBounds(area);
}

void NonAutomatableParameterComponent::mouseDown(const juce::MouseEvent &event)
{
    if (!event.mods.isRightButtonDown())
        return;

    juce::PopupMenu menu;
    menu.addItem(1, "Enter value...");

    if (menu.show() == 1)
        showValueEntryDialog();
}

void NonAutomatableParameterComponent::showValueEntryDialog()
{
    DismissibleAlertWindow dialog("Enter Parameter Value", "Enter a value for " + m_titleLabel.getText() + ":");
    dialog.addTextEditor("value", juce::String(m_knob.getValue()));
    dialog.addButton("Set", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // Enter modal state without focusing the AlertWindow itself, then explicitly
    // focus the editor before starting the nested event loop.
    dialog.enterModalState(false);
    dialog.toFront(true);
    if (auto *editor = dialog.getTextEditor("value"))
    {
        editor->setWantsKeyboardFocus(true);
        editor->grabKeyboardFocus();
        editor->selectAll();

        // Some window managers assign focus to the newly-created top-level
        // window after the immediate request. Retry once that activation has
        // settled, but never disturb an editor that already has focus.
        juce::Component::SafePointer<juce::TextEditor> safeEditor(editor);
        const auto focusEditor = [safeEditor]
        {
            if (safeEditor != nullptr && !safeEditor->hasKeyboardFocus(false))
            {
                safeEditor->grabKeyboardFocus();
                safeEditor->selectAll();
            }
        };
        juce::Timer::callAfterDelay(50, focusEditor);
        juce::Timer::callAfterDelay(200, focusEditor);
    }

    if (dialog.runModalLoop() != 1)
        return;

    const auto text = dialog.getTextEditorContents("value").trim().replaceCharacter(',', '.');
    if (!text.containsAnyOf("0123456789"))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Value", "Please enter a numeric value.");
        return;
    }

    const auto value = text.getDoubleValue();
    if (!std::isfinite(value))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Value", "The entered value is not valid for this parameter.");
        return;
    }

    m_knob.setValue(juce::jlimit(m_knob.getMinimum(), m_knob.getMaximum(), value), juce::sendNotificationSync);
}

void NonAutomatableParameterComponent::updateLabel() { m_valueLabel.setText(juce::String(m_knob.getValue()), juce::dontSendNotification); }

void NonAutomatableParameterComponent::setSliderRange(double start, double end, double interval) { m_knob.setRange(start, end, interval); }

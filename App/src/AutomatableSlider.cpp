/*
  ==============================================================================

    AutomatableSlider.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableSlider.h"
#include "DismissibleAlertWindow.h"
#include "NextLookAndFeel.h"
#include "AutomationWriteGuard.h"

#include <cmath>

namespace
{
constexpr int removeModifierMenuBaseId = 1;
constexpr int addAutomationLaneMenuId = 2000;
constexpr int clearAutomationMenuId = 2001;
constexpr int midiLearnMenuId = 2100;
constexpr int removeMidiMappingMenuId = 2101;
constexpr int enterValueMenuId = 2200;
}

AutomatableSliderComponent::AutomatableSliderComponent(const tracktion_engine::AutomatableParameter::Ptr ap)
    : m_automatableParameter(ap)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
    setWantsKeyboardFocus(true);

    if (m_automatableParameter != nullptr)
    {
        m_midiLearn = std::make_unique<AutomatableMidiLearnSupport>(*this, m_automatableParameter);
        bindSliderToParameter();
        m_automatableParameter->addListener(this);

        if (auto t = m_automatableParameter->getTrack())
            m_trackColour = t->getColour();
    }
    else
    {
        setEnabled(false);
    }

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

    if (m_automatableParameter != nullptr)
        if (auto def = m_automatableParameter->getDefaultValue())
            setDoubleClickReturnValue(true, *def);

    m_modDepthSlider.onValueChange = [this]
    {
        if (m_automatableParameter == nullptr)
            return;

        auto assignments = m_automatableParameter->getAssignments();
        if (!assignments.isEmpty())
        {
            auto &ass = *assignments[0];
            ass.value = (float)m_modDepthSlider.getValue();
            repaint();
        }
    };

    updateModDepthVisibility();
    if (m_midiLearn != nullptr)
        m_midiLearn->refreshMappingState();
}

AutomatableSliderComponent::~AutomatableSliderComponent()
{
    m_midiLearn.reset();
    m_modDepthSlider.removeMouseListener(this);

    if (m_automatableParameter != nullptr)
        m_automatableParameter->removeListener(this);
}

void AutomatableSliderComponent::mouseEnter(const juce::MouseEvent &)
{
    if (m_midiLearn != nullptr)
        m_midiLearn->refreshMappingState();
    updateModDepthVisibility();
    repaint();
}

void AutomatableSliderComponent::mouseExit(const juce::MouseEvent &)
{
    updateModDepthVisibility();
    repaint();
}

void AutomatableSliderComponent::enablementChanged() { setAlpha(isEnabled() ? 1.0f : 0.5f); }

void AutomatableSliderComponent::paintOverChildren(juce::Graphics &g)
{
    juce::Slider::paintOverChildren(g);

    if (m_automatableParameter == nullptr)
        return;

    if (m_midiLearn != nullptr)
        m_midiLearn->paintOverChildren(g, getLocalBounds(), isMouseOver(true));
}

bool AutomatableSliderComponent::keyPressed(const juce::KeyPress &key)
{
    if (m_midiLearn != nullptr && m_midiLearn->keyPressed(key))
        return true;

    return juce::Slider::keyPressed(key);
}

void AutomatableSliderComponent::resized()
{
    juce::Slider::resized();
    m_modDepthSlider.setBounds(getLocalBounds().removeFromBottom(10));
}

void AutomatableSliderComponent::updateModDepthVisibility()
{
    if (m_automatableParameter == nullptr)
    {
        m_modDepthSlider.setVisible(false);
        return;
    }

    auto assignments = m_automatableParameter->getAssignments();

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
    if (m_automatableParameter == nullptr)
        return;

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;

        m.addItem(enterValueMenuId, "Enter value...");
        m.addSeparator();

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

        if (m_midiLearn != nullptr)
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

        if (result == enterValueMenuId)
        {
            showValueEntryDialog();
        }
        else if (m_midiLearn != nullptr && m_midiLearn->handleContextMenuResult(result, midiLearnMenuId, removeMidiMappingMenuId))
        {
        }
        else if (result == addAutomationLaneMenuId)
        {
            auto start = tracktion::core::TimePosition::fromSeconds(0.0);
            m_automatableParameter->getCurve().addPoint(start, (float)getValue(), 0.0);

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
    else if (e.originalComponent != &m_modDepthSlider)
    {
        juce::Slider::mouseDown(e);
    }
}

void AutomatableSliderComponent::showValueEntryDialog()
{
    if (m_automatableParameter == nullptr)
        return;

    DismissibleAlertWindow dialog("Enter Parameter Value", "Enter a value for " + m_automatableParameter->getParameterName() + ":");
    dialog.addTextEditor("value", m_automatableParameter->valueToString(m_automatableParameter->getCurrentBaseValue()));
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

    auto text = dialog.getTextEditorContents("value").trim().replaceCharacter(',', '.');
    const auto lowerText = text.toLowerCase();
    const bool hasNumericValue = text.containsAnyOf("0123456789") || lowerText.contains("inf") || lowerText == "centre" || lowerText == "center";

    if (!hasNumericValue)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Value", "Please enter a numeric value.");
        return;
    }

    auto value = m_automatableParameter->stringToValue(text);
    if (!std::isfinite(value))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid Value", "The entered value is not valid for this parameter.");
        return;
    }

    const auto &range = m_automatableParameter->valueRange;
    value = range.snapToLegalValue(juce::jlimit(range.start, range.end, value));

    m_automatableParameter->beginParameterChangeGesture();
    m_automatableParameter->setParameter(value, juce::sendNotification);
    m_automatableParameter->endParameterChangeGesture();
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

    if (m_midiLearn != nullptr)
        m_midiLearn->cancelMidiLearn();

    if (m_automatableParameter)
        m_automatableParameter->removeListener(this);

    m_midiLearn.reset();

    m_automatableParameter = newParam;

    if (m_automatableParameter)
    {
        m_midiLearn = std::make_unique<AutomatableMidiLearnSupport>(*this, m_automatableParameter);
        m_automatableParameter->addListener(this);
        bindSliderToParameter();

        if (auto t = m_automatableParameter->getTrack())
            m_trackColour = t->getColour();

        if (auto def = m_automatableParameter->getDefaultValue())
            setDoubleClickReturnValue(true, *def);
        else
            setDoubleClickReturnValue(false, 0.0);

        updateModDepthVisibility();
        m_midiLearn->refreshMappingState();
    }
    else
    {
        setEnabled(false);
    }

    repaint();
}

void AutomatableSliderComponent::bindSliderToParameter()
{
    if (m_automatableParameter == nullptr)
    {
        setEnabled(false);
        return;
    }

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
    if (m_automatableParameter == nullptr)
        return;

    if (AutomationWriteGuard::isTouched(m_automatableParameter))
        return;

    const double newVal = m_automatableParameter->getCurrentBaseValue();

    if (!isMouseButtonDown() && std::abs(getValue() - newVal) > 0.0001)
        setValue(newVal, juce::dontSendNotification);

    repaint();
}

void AutomatableSliderComponent::parameterChanged(te::AutomatableParameter &p, float newValue)
{
    AutomationWriteGuard::markTouched(&p);

    if (&p == m_automatableParameter.get() && std::abs(getValue() - static_cast<double>(newValue)) > 0.0001)
        setValue(newValue, juce::dontSendNotification);

    repaint();
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
        AutomationWriteGuard::markTouched(m_automatableParameter);
        m_automatableParameter->setParameter(val, juce::sendNotification);
    }
}


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

#include "PluginWindow.h"
#include "PluginWindowPinning.h"
#include "PluginWindowTitleBarLayout.h"

namespace
{
class PluginWindowPinButton final : public juce::Button
{
public:
    PluginWindowPinButton()
        : juce::Button(TRANS("Pin plugin window"))
    {
        setClickingTogglesState(true);
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
    }

    void paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown) override
    {
        auto area = getLocalBounds().toFloat().reduced(1.0f);
        const auto pinned = getToggleState();
        const auto activeColour = findColour(juce::TextEditor::highlightColourId);
        const auto idleColour = findColour(juce::DocumentWindow::textColourId);

        if (pinned || isMouseOverButton || isButtonDown)
        {
            auto background = pinned ? activeColour : findColour(juce::TextButton::buttonColourId);
            if (isButtonDown)
                background = background.darker(0.18f);
            else if (isMouseOverButton && !pinned)
                background = background.brighter(0.08f);

            g.setColour(background.withMultipliedAlpha(isEnabled() ? 0.95f : 0.45f));
            g.fillRoundedRectangle(area, 3.0f);
        }

        juce::Path pin;
        pin.startNewSubPath(2.0f, 1.0f);
        pin.lineTo(10.0f, 1.0f);
        pin.lineTo(9.0f, 4.0f);
        pin.lineTo(8.0f, 6.0f);
        pin.lineTo(7.0f, 6.0f);
        pin.lineTo(7.0f, 10.0f);
        pin.lineTo(6.0f, 13.0f);
        pin.lineTo(5.0f, 10.0f);
        pin.lineTo(5.0f, 6.0f);
        pin.lineTo(4.0f, 6.0f);
        pin.lineTo(3.0f, 4.0f);
        pin.closeSubPath();

        const auto iconArea = area.reduced(4.0f, 3.0f);
        pin.applyTransform(juce::AffineTransform::scale(iconArea.getWidth() / 12.0f, iconArea.getHeight() / 14.0f)
                               .translated(iconArea.getX(), iconArea.getY()));
        if (!pinned)
            pin.applyTransform(juce::AffineTransform::rotation(-0.55f, area.getCentreX(), area.getCentreY()));

        const auto iconColour = pinned ? activeColour.contrasting() : idleColour;
        g.setColour(iconColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.45f));
        g.fillPath(pin);
    }
};
} // namespace

#if JUCE_LINUX
constexpr bool shouldAddPluginWindowToDesktop = false;
#else
constexpr bool shouldAddPluginWindowToDesktop = true;
#endif

juce::Array<PluginWindow *> PluginWindow::s_openWindows;
bool PluginWindow::s_interactionEnabled = true;

PluginWindow::PluginWindow(te::Plugin &plug, ComputerMidiKeyboardController *computerMidiKeyboard)
    : DocumentWindow(plug.getName(), juce::Colours::black, DocumentWindow::closeButton, shouldAddPluginWindowToDesktop),
      plugin(plug),
      windowState(*plug.windowState),
      m_computerMidiKeyboard(computerMidiKeyboard)
{
    s_openWindows.add(this);

    m_pinButton = std::make_unique<PluginWindowPinButton>();
    addAndMakeVisible(*m_pinButton);
    m_pinButton->onClick = [this]
    {
        setPinned(m_pinButton->getToggleState());
    };
    updatePinButtonState();
    setWorkflowInteractionEnabled(s_interactionEnabled);

    getConstrainer()->setMinimumOnscreenAmounts(0x10000, 50, 30, 50);
    setResizeLimits(100, 50, 4000, 4000);

    recreateEditor();

    setBoundsConstrained(getLocalBounds() + plugin.windowState->choosePositionForPluginWindow());

#if JUCE_LINUX
    addToDesktop();
#endif

    updateStoredBounds = true;
}

PluginWindow::~PluginWindow()
{
    s_openWindows.removeFirstMatchingValue(this);
    if (m_computerMidiKeyboard != nullptr && m_midiKeyboardAttached)
        m_computerMidiKeyboard->detachFrom(*this);

    updateStoredBounds = false;
    plugin.edit.flushPluginStateIfNeeded(plugin);
    setEditor(nullptr);
}

void PluginWindow::setAllInteractionEnabled(bool enabled)
{
    s_interactionEnabled = enabled;
    for (auto *window : s_openWindows)
        if (window != nullptr)
            window->setWorkflowInteractionEnabled(enabled);
}

void PluginWindow::setWorkflowInteractionEnabled(bool enabled)
{
    setEnabled(enabled);
    if (m_computerMidiKeyboard == nullptr || enabled == m_midiKeyboardAttached)
        return;

    if (enabled)
        m_computerMidiKeyboard->attachTo(*this);
    else
        m_computerMidiKeyboard->detachFrom(*this);
    m_midiKeyboardAttached = enabled;
}

void PluginWindow::show()
{
    setVisible(true);
    toFront(false);
    setBoundsConstrained(getBounds());

#if JUCE_LINUX
    if (m_isPinned && !PluginWindowPinning::setLinuxAlwaysOnTop(*this, true))
        m_isPinned = false;
#endif

    updatePinButtonState();
}

void PluginWindow::setPinned(bool pinned)
{
    if (m_isPinned != pinned)
    {
#if JUCE_LINUX
        if (!PluginWindowPinning::setLinuxAlwaysOnTop(*this, pinned))
        {
            updatePinButtonState();
            return;
        }
        m_isPinned = pinned;
#else
        juce::ScopedValueSetter<bool> suppressBoundsPersistence(updateStoredBounds, false);
        setAlwaysOnTop(pinned);
        m_isPinned = isAlwaysOnTop();
#endif
    }

    updatePinButtonState();
    if (m_isPinned)
        toFront(false);
}

void PluginWindow::updatePinButtonState()
{
    if (m_pinButton == nullptr)
        return;

    m_pinButton->setToggleState(m_isPinned, juce::dontSendNotification);
    m_pinButton->setName(m_isPinned ? TRANS("Unpin plugin window") : TRANS("Pin plugin window"));
    m_pinButton->setTooltip(m_isPinned ? TRANS("Allow plugin window behind other windows")
                                       : TRANS("Keep plugin window on top"));
    m_pinButton->repaint();
}

void PluginWindow::paint(juce::Graphics &g)
{
    juce::ResizableWindow::paint(g);

    const auto titleBarArea = getTitleBarArea();
    if (titleBarArea.isEmpty())
        return;

    const auto closeBounds = getCloseButton() != nullptr ? getCloseButton()->getBounds() : juce::Rectangle<int>{};
    const auto layout = PluginWindowTitleBarLayout::calculate(titleBarArea, closeBounds);
    const juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(titleBarArea);
    g.setOrigin(titleBarArea.getPosition());

    getLookAndFeel().drawDocumentWindowTitleBar(*this,
                                                g,
                                                titleBarArea.getWidth(),
                                                titleBarArea.getHeight(),
                                                layout.titleSpace.getX() - titleBarArea.getX(),
                                                juce::jmax(1, layout.titleSpace.getWidth()),
                                                nullptr,
                                                false);
}

void PluginWindow::resized()
{
    juce::DocumentWindow::resized();

    if (m_pinButton == nullptr)
        return;

    const auto titleBarArea = getTitleBarArea();
    const auto closeBounds = getCloseButton() != nullptr ? getCloseButton()->getBounds() : juce::Rectangle<int>{};
    const auto layout = PluginWindowTitleBarLayout::calculate(titleBarArea, closeBounds);
    m_pinButton->setBounds(layout.pinButton);
    m_pinButton->setVisible(!layout.pinButton.isEmpty());
}

void PluginWindow::setEditor(std::unique_ptr<te::Plugin::EditorComponent> newEditor)
{
    JUCE_AUTORELEASEPOOL
    {
        setConstrainer(nullptr);
        editor.reset();

        if (newEditor != nullptr)
        {
            editor = std::move(newEditor);
            setContentNonOwned(editor.get(), true);
        }

        setResizable(editor == nullptr || editor->allowWindowResizing(), false);

        if (editor != nullptr && editor->allowWindowResizing())
            setConstrainer(editor->getBoundsConstrainer());
    }
}

std::unique_ptr<juce::Component> PluginWindow::create(te::Plugin &plugin, ComputerMidiKeyboardController *computerMidiKeyboard)
{
    if (auto externalPlugin = dynamic_cast<te::ExternalPlugin *>(&plugin))
        if (externalPlugin->getAudioPluginInstance() == nullptr)
            return nullptr;

    std::unique_ptr<PluginWindow> w;

#if JUCE_WINDOWS && JUCE_WIN_PER_MONITOR_DPI_AWARE
    if (!isDPIAware(plugin))
    {
        juce::ScopedDPIAwarenessDisabler disableDPIAwareness;
        w = std::make_unique<PluginWindow>(plugin, computerMidiKeyboard);
    }
    else
#endif
    {
        w = std::make_unique<PluginWindow>(plugin, computerMidiKeyboard);
    }

    if (w == nullptr || w->getEditor() == nullptr)
        return {};

    w->show();

    return w;
}

void PluginWindow::userTriedToCloseWindow()
{
    // Keep the editor alive and only hide its host window. Destroying some native
    // VST editors while several desktop windows are active can block the message
    // thread. The retained window is reused by showWindowExplicitly().
    windowState.wasExplicitlyClosed = true;
    setVisible(false);
}

void PluginWindow::recreateEditor()
{
    setEditor(nullptr);
    setEditor(plugin.createEditor());
}

void PluginWindow::recreateEditorAsync()
{
    setEditor(nullptr);

    juce::Timer::callAfterDelay(50,
                                [this, sp = SafePointer<Component>(this)]
                                {
                                    if (sp != nullptr)
                                        recreateEditor();
                                });
}

void PluginWindow::moved()
{
    if (updateStoredBounds)
    {
        plugin.windowState->lastWindowBounds = getBounds();
        plugin.edit.pluginChanged(plugin);
    }
}

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ComputerMidiKeyboardController.h"

#include <algorithm>

ComputerMidiKeyboardController::ComputerMidiKeyboardController()
{
    juce::Desktop::getInstance().addFocusChangeListener(this);
}

ComputerMidiKeyboardController::~ComputerMidiKeyboardController()
{
    releaseAllNotes();
    juce::Desktop::getInstance().removeFocusChangeListener(this);

    for (auto &root : m_attachedRoots)
        if (root != nullptr)
            root->removeKeyListener(this);
}

void ComputerMidiKeyboardController::attachTo(juce::Component &root)
{
    const auto alreadyAttached = std::any_of(m_attachedRoots.begin(), m_attachedRoots.end(),
                                             [&root](const auto &candidate) { return candidate.getComponent() == &root; });
    if (alreadyAttached)
        return;

    root.addKeyListener(this);
    m_attachedRoots.emplace_back(&root);
}

void ComputerMidiKeyboardController::detachFrom(juce::Component &root)
{
    root.removeKeyListener(this);
    m_attachedRoots.erase(std::remove_if(m_attachedRoots.begin(), m_attachedRoots.end(),
                                         [&root](const auto &candidate) { return candidate.getComponent() == &root; }),
                          m_attachedRoots.end());

    if (!focusBelongsToAttachedRoot(juce::Component::getCurrentlyFocusedComponent()))
        releaseAllNotes();
}

void ComputerMidiKeyboardController::setKeyboardState(juce::MidiKeyboardState *keyboardState)
{
    if (m_keyboardState == keyboardState)
        return;

    releaseAllNotes();
    m_keyboard.reset();
    m_keyboardState = keyboardState;

    if (m_keyboardState != nullptr)
    {
        m_keyboard = std::make_unique<juce::MidiKeyboardComponent>(*m_keyboardState,
                                                                   juce::MidiKeyboardComponent::horizontalKeyboard);
        ComputerMidiKeyboardLayout::applyTo(*m_keyboard);
    }
}

void ComputerMidiKeyboardController::releaseAllNotes()
{
    if (m_keyboard != nullptr)
        m_keyboard->focusLost(juce::Component::focusChangedDirectly);

    if (m_upperCDown && m_keyboardState != nullptr)
        m_keyboardState->noteOff(ComputerMidiKeyboardLayout::midiChannel,
                                 ComputerMidiKeyboardLayout::upperCMidiNote,
                                 0.0f);

    m_upperCDown = false;
}

bool ComputerMidiKeyboardController::keyPressed(const juce::KeyPress &key, juce::Component *)
{
    if (m_keyboard == nullptr)
        return false;

    return ComputerMidiKeyboardLayout::isMappedPerformanceKey(key) || m_keyboard->keyPressed(key);
}

bool ComputerMidiKeyboardController::keyStateChanged(bool isKeyDown, juce::Component *)
{
    juce::ignoreUnused(isKeyDown);

    if (m_keyboard == nullptr || m_keyboardState == nullptr)
        return false;

    bool used = m_keyboard->keyStateChanged(isKeyDown);
    const bool upperCIsDown = m_upperCKey.isCurrentlyDown() || m_upperCAlternateKey.isCurrentlyDown();

    if (upperCIsDown != m_upperCDown)
    {
        m_upperCDown = upperCIsDown;

        if (m_upperCDown)
            m_keyboardState->noteOn(ComputerMidiKeyboardLayout::midiChannel,
                                    ComputerMidiKeyboardLayout::upperCMidiNote,
                                    ComputerMidiKeyboardLayout::velocity);
        else
            m_keyboardState->noteOff(ComputerMidiKeyboardLayout::midiChannel,
                                     ComputerMidiKeyboardLayout::upperCMidiNote,
                                     0.0f);

        used = true;
    }

    return used;
}

void ComputerMidiKeyboardController::globalFocusChanged(juce::Component *focusedComponent)
{
    if (!focusBelongsToAttachedRoot(focusedComponent))
        releaseAllNotes();
}

bool ComputerMidiKeyboardController::focusBelongsToAttachedRoot(const juce::Component *focusedComponent) const
{
    if (focusedComponent == nullptr)
        return false;

    return std::any_of(m_attachedRoots.begin(), m_attachedRoots.end(),
                       [focusedComponent](const auto &root)
                       {
                           return root != nullptr
                                  && (root.getComponent() == focusedComponent || root->isParentOf(focusedComponent));
                       });
}

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ComputerMidiKeyboardLayout.h"

/** Routes computer-keyboard notes through Tracktion's virtual MIDI input.

    JUCE's MidiKeyboardComponent owns the key transition state, suppresses OS
    key-repeat note-ons, and releases its notes on focus loss. This adapter lets
    that behaviour participate in the key-event hierarchy of NextStudio's main
    and plug-in windows without making a visible keyboard own global input.
*/
class ComputerMidiKeyboardController final
    : public juce::KeyListener
    , private juce::FocusChangeListener
{
public:
    ComputerMidiKeyboardController();
    ~ComputerMidiKeyboardController() override;

    void attachTo(juce::Component &root);
    void detachFrom(juce::Component &root);

    void setKeyboardState(juce::MidiKeyboardState *keyboardState);
    void setLayout(const ComputerMidiKeyboardLayout::State &layout);
    void releaseAllNotes();

    bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) override;

private:
    void globalFocusChanged(juce::Component *focusedComponent) override;
    bool focusBelongsToAttachedRoot(const juce::Component *focusedComponent) const;

    std::unique_ptr<juce::MidiKeyboardComponent> m_keyboard;
    juce::MidiKeyboardState *m_keyboardState{};
    ComputerMidiKeyboardLayout::State m_layout{ComputerMidiKeyboardLayout::createDefaultState()};
    std::vector<juce::Component::SafePointer<juce::Component>> m_attachedRoots;
    bool m_upperCAliasDown{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComputerMidiKeyboardController)
};

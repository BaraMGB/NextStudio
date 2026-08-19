
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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include <functional>

class VirtualKeyboardComponent : public juce::KeyboardComponentBase
{
public:
    explicit VirtualKeyboardComponent(ApplicationViewState &applicationState)
        : juce::KeyboardComponentBase(juce::MidiKeyboardComponent::Orientation::verticalKeyboardFacingRight),
          m_applicationState(applicationState)
    {
        setBlackNoteWidthProportion(0.5f);
        setBlackNoteLengthProportion(0.6f);
        setScrollButtonsVisible(false);

        setInterceptsMouseClicks(false, false);
    }

    ~VirtualKeyboardComponent() = default;

    void setNoteDown(int midiNoteNumber, bool isDown)
    {
        if (juce::isPositiveAndBelow(midiNoteNumber, 128) && m_notesDown[midiNoteNumber] != isDown)
        {
            m_notesDown.setBit(midiNoteNumber, isDown);
            repaint(getRectangleForKey(midiNoteNumber).getSmallestIntegerContainer());
        }
    }

    void drawKeyboardBackground(juce::Graphics &g, juce::Rectangle<float> area) override {}
    void drawWhiteKey(int midiNoteNumberm, juce::Graphics &g, juce::Rectangle<float> area) override
    {
        g.setColour(m_notesDown[midiNoteNumberm] ? m_applicationState.getPrimeColour() : juce::Colour(0xffdddddd));
        g.fillRect(area);
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(0, area.getX(), area.getRight());
        if (juce::MidiMessage::getMidiNoteName(midiNoteNumberm, true, false, 3) == "C")
        {
            auto name = juce::MidiMessage::getMidiNoteName(midiNoteNumberm, true, true, 3);

            g.drawText(name, area, juce::Justification::right);
        }
    }
    void drawBlackKey(int midiNoteNumberm, juce::Graphics &g, juce::Rectangle<float> area) override
    {
        g.setColour(m_notesDown[midiNoteNumberm] ? m_applicationState.getPrimeColour().darker() : juce::Colours::black);
        g.fillRect(area);
    }

private:
    ApplicationViewState &m_applicationState;
    juce::BigInteger m_notesDown;
};

class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView(EditViewState &evs, juce::String timeLineID)
        : m_editViewState(evs),
          m_keyboard(evs.m_applicationState),
          m_timeLineID(timeLineID)
    {
        addAndMakeVisible(&m_keyboard);
    }
    ~KeyboardView() = default;

    void setOnKeyClicked(std::function<void(int midiNoteNumber, bool addToSelection)> callback) { m_onKeyClicked = std::move(callback); }
    void setMidiNotesDown(const juce::Array<int> &notesOn, const juce::Array<int> &notesOff);

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void resized() override;

private:
    float getKey(int y);

    EditViewState &m_editViewState;
    VirtualKeyboardComponent m_keyboard;
    std::function<void(int midiNoteNumber, bool addToSelection)> m_onKeyClicked;

    float m_clickedKey;
    double m_keyWidthCached;
    juce::String m_timeLineID;
};

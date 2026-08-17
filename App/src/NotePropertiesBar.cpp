/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "NotePropertiesBar.h"
#include "PositionDisplayHelpers.h"
#include "Utilities.h"
#include <cmath>

namespace
{
constexpr double valueEpsilon = 1.0e-7;

std::optional<int> parseStrictInt(const juce::String &text)
{
    const auto value = text.trim();
    if (value.isEmpty())
        return {};

    int start = (value[0] == '-' || value[0] == '+') ? 1 : 0;
    if (start == value.length())
        return {};

    for (int i = start; i < value.length(); ++i)
        if (!juce::CharacterFunctions::isDigit(value[i]))
            return {};

    return value.getIntValue();
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < valueEpsilon;
}

float measureTextWidth(const juce::Font &font, const juce::String &text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

constexpr int panelPadding = 6;
constexpr int fieldPadding = 6;
constexpr int labelGap = 4;
constexpr int fieldGap = 3;
constexpr float titleFontHeight = 9.0f;
constexpr float valueFontHeight = 14.0f;

constexpr int snapOffItem = 1;
constexpr int snapWholeItem = 2;
constexpr int snapHalfItem = 3;
constexpr int snapQuarterItem = 4;
constexpr int snapEighthItem = 5;
constexpr int snapSixteenthItem = 6;
constexpr int snapThirtySecondItem = 7;
constexpr int snapSixtyFourthItem = 8;
constexpr int snapOneTwentyEighthItem = 9;
constexpr int snapAdaptiveItem = 10;
} // namespace

bool NotePropertiesBar::PropertyEditor::keyPressed(const juce::KeyPress &key)
{
    if (key.getKeyCode() == juce::KeyPress::tabKey)
    {
        if (tabPressed)
            tabPressed(key.getModifiers().isShiftDown());
        return true;
    }

    if (isReadOnly() && (key.getKeyCode() == juce::KeyPress::returnKey || key.getKeyCode() == juce::KeyPress::F2Key))
    {
        setReadOnly(false);
        setMouseCursor(juce::MouseCursor::IBeamCursor);
        if (textEditingStarted)
            textEditingStarted();
        grabKeyboardFocus();
        selectAll();
        return true;
    }

    return isReadOnly() ? false : juce::TextEditor::keyPressed(key);
}

void NotePropertiesBar::PropertyEditor::focusGained(FocusChangeType type)
{
    juce::TextEditor::focusGained(type);
    if (focusGainedCallback)
        focusGainedCallback();
}

void NotePropertiesBar::PropertyEditor::mouseDown(const juce::MouseEvent &event)
{
    m_dragActive = false;
    m_lastDragStep = 0;
    juce::TextEditor::mouseDown(event);
}

void NotePropertiesBar::PropertyEditor::mouseDoubleClick(const juce::MouseEvent &event)
{
    if (!isReadOnly())
    {
        juce::TextEditor::mouseDoubleClick(event);
        return;
    }

    setReadOnly(false);
    setMouseCursor(juce::MouseCursor::IBeamCursor);
    if (textEditingStarted)
        textEditingStarted();
    grabKeyboardFocus();
    selectAll();
}

void NotePropertiesBar::PropertyEditor::mouseDrag(const juce::MouseEvent &event)
{
    if (!isReadOnly())
    {
        juce::TextEditor::mouseDrag(event);
        return;
    }

    constexpr int pixelsPerStep = 4;
    const auto dragStep = -(event.getDistanceFromDragStartY() / pixelsPerStep);

    if (!m_dragActive && std::abs(event.getDistanceFromDragStartY()) >= pixelsPerStep)
    {
        m_dragActive = true;
        if (dragStarted)
            dragStarted();
    }

    if (!m_dragActive)
    {
        juce::TextEditor::mouseDrag(event);
        return;
    }

    event.source.enableUnboundedMouseMovement(true);
    const auto stepDelta = dragStep - m_lastDragStep;
    if (stepDelta != 0 && dragUpdated)
        dragUpdated(stepDelta);
    m_lastDragStep = dragStep;
}

void NotePropertiesBar::PropertyEditor::mouseUp(const juce::MouseEvent &event)
{
    if (!isReadOnly())
    {
        juce::TextEditor::mouseUp(event);
        return;
    }

    if (m_dragActive)
    {
        event.source.enableUnboundedMouseMovement(false);
        if (dragEnded)
            dragEnded();
    }
    else
    {
        juce::TextEditor::mouseUp(event);
    }

    m_dragActive = false;
    m_lastDragStep = 0;
}

void NotePropertiesBar::PropertyEditor::mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel)
{
    if (isReadOnly() && wheelMoved && std::abs(wheel.deltaY) > 0.0f)
        wheelMoved(wheel.deltaY > 0.0f ? 1 : -1);
}

NotePropertiesBar::NotePropertiesBar(EditViewState &evs)
    : m_evs(evs)
{
    const std::array<juce::String, 5> names{{"START", "END", "DURATION", "PITCH", "VELOCITY"}};

    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        field.label.setText(names[i], juce::dontSendNotification);
        field.label.setJustificationType(juce::Justification::centredLeft);
        field.label.setFont(juce::Font(juce::FontOptions(titleFontHeight)));
        field.label.setBorderSize(juce::BorderSize<int>());
        field.label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(field.label);
        addAndMakeVisible(field.editor);
        configureField(field, static_cast<int>(i) + 1);
    }

    m_snapLabel.setText("SNAP", juce::dontSendNotification);
    m_snapLabel.setJustificationType(juce::Justification::centredLeft);
    m_snapLabel.setFont(juce::Font(juce::FontOptions(titleFontHeight)));
    m_snapLabel.setBorderSize({});
    m_snapLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_snapLabel);

    m_snapBox.addItem("Off", snapOffItem);
    m_snapBox.addItem("1/1", snapWholeItem);
    m_snapBox.addItem("1/2", snapHalfItem);
    m_snapBox.addItem("1/4", snapQuarterItem);
    m_snapBox.addItem("1/8", snapEighthItem);
    m_snapBox.addItem("1/16", snapSixteenthItem);
    m_snapBox.addItem("1/32", snapThirtySecondItem);
    m_snapBox.addItem("1/64", snapSixtyFourthItem);
    m_snapBox.addItem("1/128", snapOneTwentyEighthItem);
    m_snapBox.addItem("Adaptive", snapAdaptiveItem);
    m_snapBox.setExplicitFocusOrder(6);
    addAndMakeVisible(m_snapBox);

    const auto snapMode = static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_pianoRollSnapMode));
    int selectedSnapItem = snapAdaptiveItem;
    if (snapMode == PianoRollSnapMode::off)
        selectedSnapItem = snapOffItem;
    else if (snapMode == PianoRollSnapMode::fixed)
    {
        switch (static_cast<int>(m_evs.m_pianoRollSnapDenominator))
        {
        case 1: selectedSnapItem = snapWholeItem; break;
        case 2: selectedSnapItem = snapHalfItem; break;
        case 4: selectedSnapItem = snapQuarterItem; break;
        case 8: selectedSnapItem = snapEighthItem; break;
        case 16: selectedSnapItem = snapSixteenthItem; break;
        case 32: selectedSnapItem = snapThirtySecondItem; break;
        case 64: selectedSnapItem = snapSixtyFourthItem; break;
        case 128: selectedSnapItem = snapOneTwentyEighthItem; break;
        default: break;
        }
    }
    m_snapBox.setSelectedId(selectedSnapItem, juce::dontSendNotification);
    m_snapBox.onChange = [this]
    {
        const auto selected = m_snapBox.getSelectedId();
        if (selected == snapOffItem)
        {
            m_evs.m_pianoRollSnapMode = static_cast<int>(PianoRollSnapMode::off);
            return;
        }
        if (selected == snapAdaptiveItem)
        {
            m_evs.m_pianoRollSnapMode = static_cast<int>(PianoRollSnapMode::adaptive);
            return;
        }

        const std::array<int, 8> denominators{{1, 2, 4, 8, 16, 32, 64, 128}};
        const auto index = selected - snapWholeItem;
        if (index >= 0 && index < static_cast<int>(denominators.size()))
        {
            m_evs.m_pianoRollSnapDenominator = denominators[static_cast<size_t>(index)];
            m_evs.m_pianoRollSnapMode = static_cast<int>(PianoRollSnapMode::fixed);
        }
    };

    updateColours();
    clearSelection();
}

void NotePropertiesBar::updateColours()
{
    const auto textColour = m_evs.m_applicationState.getButtonTextColour();
    for (auto &field : m_fields)
    {
        field.label.setColour(juce::Label::textColourId, textColour.withAlpha(0.85f));
        field.editor.setColour(juce::TextEditor::textColourId,
                               field.invalid ? juce::Colours::red : textColour);
    }
    m_snapLabel.setColour(juce::Label::textColourId, textColour.withAlpha(0.85f));
    m_snapBox.setColour(juce::ComboBox::textColourId, textColour);
    m_snapBox.setColour(juce::ComboBox::backgroundColourId, m_evs.m_applicationState.getBackgroundColour1());
    m_snapBox.setColour(juce::ComboBox::outlineColourId, m_evs.m_applicationState.getBorderColour());
    m_snapBox.setColour(juce::ComboBox::arrowColourId, textColour);
    repaint();
}

void NotePropertiesBar::configureField(Field &field, int focusOrder)
{
    field.editor.setExplicitFocusOrder(focusOrder);
    field.editor.setMultiLine(false);
    field.editor.setReturnKeyStartsNewLine(false);
    field.editor.setTabKeyUsedAsCharacter(false);
    field.editor.setSelectAllWhenFocused(false);
    field.editor.setReadOnly(true);
    field.editor.setFont(juce::Font(juce::FontOptions(valueFontHeight)));
    field.editor.applyFontToAllText(juce::Font(juce::FontOptions(valueFontHeight)));
    field.editor.setJustification(juce::Justification::centredLeft);
    field.editor.setBorder(juce::BorderSize<int>(0, 2, 0, 2));
    field.editor.setInputRestrictions(18);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    field.editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);

    field.editor.focusGainedCallback = [this, &field] { repaint(field.bounds.expanded(2)); };
    field.editor.textEditingStarted = [this, &field]
    {
        beginEditing(field);
        repaint(field.bounds.expanded(2));
    };
    field.editor.onReturnKey = [this, &field] { commit(field); };
    field.editor.onEscapeKey = [this, &field] { cancel(field); };
    field.editor.onFocusLost = [this, &field]
    {
        if (m_handlingEditorCallback)
            return;

        if (field.editor.isReadOnly())
        {
            repaint(field.bounds.expanded(2));
            return;
        }

        if (apply(field.property, field.editor.getText()))
        {
            refreshFromSelection();
        }
        else
        {
            m_handlingEditorCallback = true;
            field.editor.setText(field.displayedText, false);
            setInvalid(field, false);
            m_handlingEditorCallback = false;
        }

        field.editor.setReadOnly(true);
        field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        repaint(field.bounds.expanded(2));
    };
    field.editor.onTextChange = [this, &field]
    {
        if (!m_handlingEditorCallback)
            setInvalid(field, false);
    };
    field.editor.tabPressed = [this, &field](bool backwards)
    {
        if (field.editor.isReadOnly())
        {
            focusAdjacent(field, backwards);
            return;
        }

        const bool untouchedMixedValue = field.editor.getText().trim().isEmpty()
                                         && field.displayedText == juce::String::fromUTF8("\xe2\x80\x94");
        if (untouchedMixedValue || apply(field.property, field.editor.getText()))
        {
            field.editor.setReadOnly(true);
            field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            refreshFromSelection();
            focusAdjacent(field, backwards);
        }
        else
        {
            setInvalid(field, true);
        }
    };
    field.editor.wheelMoved = [this, &field](int direction) { scrub(field, direction); };
    field.editor.dragStarted = [this, &field] { beginUndoTransaction(field.property); };
    field.editor.dragUpdated = [this, &field](int stepDelta) { scrub(field, stepDelta, false); };
    field.editor.dragEnded = [this] { m_evs.m_edit.getUndoManager().beginNewTransaction(); };
}

void NotePropertiesBar::setSelectionProvider(SelectionProvider provider)
{
    m_selectionProvider = std::move(provider);
    refreshFromSelection(true);
}

void NotePropertiesBar::clearSelection()
{
    m_selection.clear();
    m_handlingEditorCallback = true;
    for (auto &field : m_fields)
    {
        field.displayedText = juce::String::fromUTF8("\xe2\x80\x94");
        field.editor.setText(field.displayedText, false);
        field.editor.setReadOnly(true);
        field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        field.label.setEnabled(false);
        field.editor.setEnabled(false);
        setInvalid(field, false);
    }
    m_handlingEditorCallback = false;
    repaint();
}

void NotePropertiesBar::refreshFromSelection(bool discardActiveEdit)
{
    auto newSelection = m_selectionProvider ? m_selectionProvider() : juce::Array<std::pair<te::MidiClip *, te::MidiNote *>>{};
    newSelection.removeIf([](const auto &item) { return item.first == nullptr || item.second == nullptr; });

    if (newSelection.isEmpty())
    {
        clearSelection();
        return;
    }

    bool selectionChanged = newSelection.size() != m_selection.size();
    if (!selectionChanged)
    {
        for (int i = 0; i < newSelection.size(); ++i)
        {
            if (newSelection.getReference(i).first != m_selection.getReference(i).first ||
                newSelection.getReference(i).second != m_selection.getReference(i).second)
            {
                selectionChanged = true;
                break;
            }
        }
    }

    m_selection = newSelection;
    m_handlingEditorCallback = true;

    // MidiViewport also broadcasts tool and delayed selection notifications.
    // Only an actual note-selection change may discard an in-progress value.
    if (discardActiveEdit && selectionChanged)
        for (auto &field : m_fields)
            if (field.editor.hasKeyboardFocus(true))
            {
                field.editor.setReadOnly(true);
                field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                field.editor.giveAwayKeyboardFocus();
            }

    auto commonValue = [this](auto getter, auto formatter)
    {
        const auto first = getter(m_selection.getFirst());
        for (const auto &item : m_selection)
            if (!nearlyEqual(static_cast<double>(getter(item)), static_cast<double>(first)))
                return juce::String::fromUTF8("\xe2\x80\x94");
        return formatter(first);
    };

    const auto startText = commonValue(
        [this](const auto &item) { return getGlobalStart(*item.first, *item.second); },
        [this](double value) { return formatPosition(value); });
    const auto endText = commonValue(
        [this](const auto &item) { return getGlobalStart(*item.first, *item.second) + item.second->getLengthBeats().inBeats(); },
        [this](double value) { return formatPosition(value); });
    const auto durationText = commonValue(
        [](const auto &item) { return item.second->getLengthBeats().inBeats(); },
        [this](double value) { return formatDuration(value); });
    const auto pitchText = commonValue(
        [](const auto &item) { return item.second->getNoteNumber(); },
        [](int value) { return juce::MidiMessage::getMidiNoteName(value, true, true, 3); });
    const auto velocityText = commonValue(
        [](const auto &item) { return item.second->getVelocity(); },
        [](int value) { return juce::String(value); });

    const std::array<juce::String, 5> texts{{startText, endText, durationText, pitchText, velocityText}};
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        field.displayedText = texts[i];
        field.label.setEnabled(true);
        field.editor.setEnabled(true);
        setInvalid(field, false);
        if ((discardActiveEdit && selectionChanged) || !field.editor.hasKeyboardFocus(true))
            field.editor.setText(field.displayedText, false);
    }

    m_handlingEditorCallback = false;
    repaint();
}

void NotePropertiesBar::beginEditing(Field &field)
{
    if (!field.editor.isEnabled())
        return;

    if (field.editor.getText() == juce::String::fromUTF8("\xe2\x80\x94"))
    {
        m_handlingEditorCallback = true;
        field.editor.clear();
        m_handlingEditorCallback = false;
    }
}

void NotePropertiesBar::commit(Field &field)
{
    if (m_handlingEditorCallback || !field.editor.isEnabled())
        return;

    if (!apply(field.property, field.editor.getText()))
    {
        setInvalid(field, true);
        return;
    }

    m_handlingEditorCallback = true;
    field.editor.setReadOnly(true);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    field.editor.giveAwayKeyboardFocus();
    m_handlingEditorCallback = false;
    refreshFromSelection();
}

void NotePropertiesBar::cancel(Field &field)
{
    m_handlingEditorCallback = true;
    field.editor.setText(field.displayedText, false);
    field.editor.setReadOnly(true);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setInvalid(field, false);
    field.editor.giveAwayKeyboardFocus();
    m_handlingEditorCallback = false;
}

void NotePropertiesBar::focusAdjacent(Field &field, bool backwards)
{
    auto index = static_cast<int>(&field - m_fields.data());
    index = (index + (backwards ? -1 : 1) + static_cast<int>(m_fields.size())) % static_cast<int>(m_fields.size());

    m_handlingEditorCallback = true;
    field.editor.giveAwayKeyboardFocus();
    m_handlingEditorCallback = false;
    m_fields[static_cast<size_t>(index)].editor.grabKeyboardFocus();
    m_fields[static_cast<size_t>(index)].editor.selectAll();
}

void NotePropertiesBar::scrub(Field &field, int stepDelta, bool shouldBeginUndoTransaction)
{
    if (!field.editor.isEnabled() || stepDelta == 0)
        return;

    juce::String relative;
    switch (field.property)
    {
    case Property::start:
    case Property::end:
    case Property::duration:
        relative = (stepDelta > 0 ? "+" : "") + juce::String(stepDelta) + "/16";
        break;
    case Property::pitch:
        relative = (stepDelta > 0 ? "+" : "") + juce::String(stepDelta) + " st";
        break;
    case Property::velocity:
        relative = (stepDelta > 0 ? "+" : "") + juce::String(stepDelta);
        break;
    }

    if (apply(field.property, relative, shouldBeginUndoTransaction))
    {
        refreshFromSelection();
        m_handlingEditorCallback = true;
        field.editor.setText(field.displayedText, false);
        m_handlingEditorCallback = false;
    }
}

void NotePropertiesBar::beginUndoTransaction(Property property)
{
    auto &undo = m_evs.m_edit.getUndoManager();
    switch (property)
    {
    case Property::start: undo.beginNewTransaction("Move MIDI Notes"); break;
    case Property::end:
    case Property::duration: undo.beginNewTransaction("Change MIDI Note Duration"); break;
    case Property::pitch: undo.beginNewTransaction("Change MIDI Note Pitch"); break;
    case Property::velocity: undo.beginNewTransaction("Change MIDI Note Velocity"); break;
    }
}

bool NotePropertiesBar::apply(Property property, const juce::String &input, bool shouldBeginUndoTransaction)
{
    m_selection = m_selectionProvider ? m_selectionProvider() : decltype(m_selection){};
    m_selection.removeIf([](const auto &item) { return item.first == nullptr || item.second == nullptr; });
    if (m_selection.isEmpty())
        return false;

    const auto text = input.trim();
    if (text.isEmpty() || text == juce::String::fromUTF8("\xe2\x80\x94"))
        return false;

    const bool relative = text.startsWithChar('+') || text.startsWithChar('-');
    double timingValue = 0.0;
    int integerValue = 0;

    if (property == Property::start || property == Property::end)
    {
        if (relative)
        {
            auto parsed = parseDuration(text.substring(1));
            if (!parsed)
                return false;
            timingValue = *parsed * (text.startsWithChar('-') ? -1.0 : 1.0);
        }
        else
        {
            auto parsed = parsePosition(text);
            if (!parsed)
                return false;
            timingValue = *parsed;
        }
    }
    else if (property == Property::duration)
    {
        auto parsed = parseDuration(relative ? text.substring(1) : text);
        if (!parsed)
            return false;
        timingValue = *parsed * (relative && text.startsWithChar('-') ? -1.0 : 1.0);
    }
    else if (property == Property::pitch)
    {
        if (relative)
        {
            auto relativeText = text.dropLastCharacters(text.endsWithIgnoreCase("st") ? 2 : 0).trim();
            auto parsed = parseStrictInt(relativeText);
            if (!parsed || !text.endsWithIgnoreCase("st"))
                return false;
            integerValue = *parsed;
        }
        else
        {
            auto parsed = parsePitch(text);
            if (!parsed)
                return false;
            integerValue = *parsed;
        }
    }
    else
    {
        auto parsed = parseStrictInt(text);
        if (!parsed)
            return false;
        integerValue = *parsed;
    }

    // Validate the complete operation before changing any selected note.
    for (const auto &item : m_selection)
    {
        const auto start = getGlobalStart(*item.first, *item.second);
        const auto length = item.second->getLengthBeats().inBeats();
        if (property == Property::start && (relative ? start + timingValue : timingValue) < 0.0)
            return false;
        if (property == Property::end && (relative ? length + timingValue : timingValue - start) <= 0.0)
            return false;
        if (property == Property::duration && (relative ? length + timingValue : timingValue) <= 0.0)
            return false;
        if (property == Property::pitch && !relative && (integerValue < 0 || integerValue > 127))
            return false;
    }

    auto &undo = m_evs.m_edit.getUndoManager();
    if (shouldBeginUndoTransaction)
        beginUndoTransaction(property);

    for (const auto &item : m_selection)
    {
        auto *clip = item.first;
        auto *note = item.second;
        const auto start = getGlobalStart(*clip, *note);
        const auto length = note->getLengthBeats().inBeats();

        switch (property)
        {
        case Property::start:
        {
            const auto newStart = relative ? start + timingValue : timingValue;
            note->setStartAndLength(tracktion::BeatPosition::fromBeats(getInternalStart(*clip, newStart)), note->getLengthBeats(), &undo);
            break;
        }
        case Property::end:
        {
            const auto newLength = relative ? length + timingValue : timingValue - start;
            note->setStartAndLength(note->getStartBeat(), tracktion::BeatDuration::fromBeats(newLength), &undo);
            break;
        }
        case Property::duration:
        {
            const auto newLength = relative ? length + timingValue : timingValue;
            note->setStartAndLength(note->getStartBeat(), tracktion::BeatDuration::fromBeats(newLength), &undo);
            break;
        }
        case Property::pitch:
            note->setNoteNumber(juce::jlimit(0, 127, relative ? note->getNoteNumber() + integerValue : integerValue), &undo);
            break;
        case Property::velocity:
            note->setVelocity(juce::jlimit(1, 127, relative ? note->getVelocity() + integerValue : integerValue), &undo);
            break;
        }
    }

    return true;
}

void NotePropertiesBar::setInvalid(Field &field, bool invalid)
{
    if (field.invalid == invalid)
        return;

    field.invalid = invalid;
    field.editor.setColour(juce::TextEditor::textColourId,
                           invalid ? juce::Colours::red : m_evs.m_applicationState.getButtonTextColour());
    field.editor.repaint();
}

juce::String NotePropertiesBar::formatPosition(double globalBeat) const
{
    const auto position = m_evs.m_edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(globalBeat));
    return PositionDisplayHelpers::formatBarsBeatsTicks(
        m_evs.m_edit.tempoSequence.getInternalSequence(), position, te::Edit::ticksPerQuarterNote);
}

std::optional<double> NotePropertiesBar::parsePosition(const juce::String &input) const
{
    const auto position = PositionDisplayHelpers::parseBarsBeatsTicks(
        m_evs.m_edit.tempoSequence.getInternalSequence(), input, te::Edit::ticksPerQuarterNote);
    if (!position)
        return {};

    return m_evs.m_edit.tempoSequence.toBeats(*position).inBeats();
}

juce::String NotePropertiesBar::formatDuration(double beats) const
{
    const std::array<std::pair<const char *, double>, 8> durations{{
        {"1/1", 4.0}, {"1/2", 2.0}, {"1/4", 1.0}, {"1/8", 0.5},
        {"1/16", 0.25}, {"1/32", 0.125}, {"1/64", 0.0625}, {"1/128", 0.03125}}};
    for (const auto &[name, value] : durations)
        if (nearlyEqual(beats, value))
            return name;

    const auto ticks = static_cast<int>(std::round(beats * te::Edit::ticksPerQuarterNote));
    return juce::String(ticks) + " ticks";
}

std::optional<double> NotePropertiesBar::parseDuration(const juce::String &input) const
{
    auto text = input.trim();
    if (text.endsWithIgnoreCase("ticks"))
    {
        auto ticks = parseStrictInt(text.dropLastCharacters(5).trim());
        if (!ticks || *ticks <= 0)
            return {};
        return *ticks / static_cast<double>(te::Edit::ticksPerQuarterNote);
    }

    auto parts = juce::StringArray::fromTokens(text, "/", "");
    if (parts.size() != 2)
        return {};
    auto numerator = parseStrictInt(parts[0]);
    auto denominator = parseStrictInt(parts[1]);
    if (!numerator || !denominator || *numerator <= 0 || *denominator <= 0)
        return {};
    return 4.0 * *numerator / *denominator;
}

std::optional<int> NotePropertiesBar::parsePitch(const juce::String &input) const
{
    const auto text = input.trim();
    if (auto number = parseStrictInt(text); number && !text.startsWithChar('+') && !text.startsWithChar('-'))
        return (*number >= 0 && *number <= 127) ? number : std::optional<int>{};

    if (text.length() < 2)
        return {};

    const auto letter = juce::CharacterFunctions::toUpperCase(text[0]);
    int pitchClass = -1;
    switch (letter)
    {
    case 'C': pitchClass = 0; break;
    case 'D': pitchClass = 2; break;
    case 'E': pitchClass = 4; break;
    case 'F': pitchClass = 5; break;
    case 'G': pitchClass = 7; break;
    case 'A': pitchClass = 9; break;
    case 'B': pitchClass = 11; break;
    default: return {};
    }

    int octaveStart = 1;
    if (text.length() > 1 && (text[1] == '#' || text[1] == 'b' || text[1] == 'B'))
    {
        pitchClass += text[1] == '#' ? 1 : -1;
        octaveStart = 2;
    }

    auto octave = parseStrictInt(text.substring(octaveStart));
    if (!octave)
        return {};

    const auto midiNote = (*octave + 2) * 12 + pitchClass;
    return (midiNote >= 0 && midiNote <= 127) ? std::optional<int>(midiNote) : std::optional<int>{};
}

double NotePropertiesBar::getGlobalStart(const te::MidiClip &clip, const te::MidiNote &note) const
{
    return clip.getStartBeat().inBeats() + note.getStartBeat().inBeats() - clip.getOffsetInBeats().inBeats();
}

double NotePropertiesBar::getInternalStart(const te::MidiClip &clip, double globalBeat) const
{
    return globalBeat - clip.getStartBeat().inBeats() + clip.getOffsetInBeats().inBeats();
}

void NotePropertiesBar::paint(juce::Graphics &g)
{
    const auto textColour = m_evs.m_applicationState.getButtonTextColour();
    auto selectionContent = m_selectionCountBounds.reduced(fieldPadding, 0);

    g.setFont(m_fields.front().label.getFont());
    g.setColour(textColour.withAlpha(m_selection.isEmpty() ? 0.4f : 0.85f));
    const auto selectionTitleWidth = juce::roundToInt(measureTextWidth(g.getCurrentFont(), "SELECTED NOTES:")) + 4;
    g.drawFittedText("SELECTED NOTES:", selectionContent.removeFromLeft(selectionTitleWidth),
                     juce::Justification::centredLeft, 1);
    selectionContent.removeFromLeft(juce::jmin(labelGap, selectionContent.getWidth()));

    g.setFont(m_fields.front().editor.getFont());
    g.setColour(textColour.withAlpha(m_selection.isEmpty() ? 0.5f : 1.0f));
    g.drawFittedText(juce::String(m_selection.size()), selectionContent,
                     juce::Justification::centredLeft, 1);

    for (const auto &field : m_fields)
    {
        if (field.editor.isReadOnly() || !field.editor.hasKeyboardFocus(true))
            continue;

        g.setColour(field.invalid ? juce::Colours::red : textColour.withAlpha(0.7f));
        g.drawRoundedRectangle(field.editor.getBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
    }
}

void NotePropertiesBar::resized()
{
    const auto titleFont = juce::Font(juce::FontOptions(titleFontHeight));
    const auto valueFont = juce::Font(juce::FontOptions(valueFontHeight));
    const std::array<juce::String, 5> stableValues{{"88.8.888", "88.8.888", "888 ticks", "G#10", "888"}};

    const auto selectionTitleWidth = juce::roundToInt(measureTextWidth(titleFont, "SELECTED NOTES:")) + 4;
    const auto selectionValueWidth = juce::roundToInt(measureTextWidth(valueFont, "8888")) + 4;
    const auto selectionCountWidth = (fieldPadding * 2) + selectionTitleWidth + labelGap + selectionValueWidth;
    int preferredPanelWidth = (panelPadding * 2) + selectionCountWidth + fieldGap;
    std::array<int, 5> titleWidths{};
    std::array<int, 5> valueWidths{};
    std::array<int, 5> fieldWidths{};

    int preferredFieldsWidth = 0;
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        titleWidths[i] = juce::roundToInt(measureTextWidth(titleFont, m_fields[i].label.getText())) + 4;
        valueWidths[i] = juce::roundToInt(measureTextWidth(valueFont, stableValues[i])) + 4;
        fieldWidths[i] = (fieldPadding * 2) + titleWidths[i] + labelGap + valueWidths[i];
        preferredFieldsWidth += fieldWidths[i];
    }

    preferredPanelWidth += preferredFieldsWidth + fieldGap * static_cast<int>(m_fields.size() - 1);
    auto fullArea = getLocalBounds().reduced(4, 1);
    auto snapArea = fullArea.removeFromRight(juce::jmin(170, fullArea.getWidth()));
    fullArea.removeFromRight(juce::jmin(fieldGap, fullArea.getWidth()));
    m_panelBounds = fullArea;
    m_panelBounds.setWidth(juce::jmin(m_panelBounds.getWidth(), preferredPanelWidth));

    snapArea = snapArea.reduced(2, 2);
    m_snapLabel.setBounds(snapArea.removeFromLeft(38));
    snapArea.removeFromLeft(juce::jmin(labelGap, snapArea.getWidth()));
    m_snapBox.setBounds(snapArea);

    auto area = m_panelBounds.reduced(panelPadding, 2);
    m_selectionCountBounds = area.removeFromLeft(juce::jmin(selectionCountWidth, area.getWidth()));
    area.removeFromLeft(juce::jmin(fieldGap, area.getWidth()));

    const auto totalFieldGaps = fieldGap * static_cast<int>(m_fields.size() - 1);
    int remainingWidth = juce::jmax(0, area.getWidth() - totalFieldGaps);
    int remainingPreferredWidth = preferredFieldsWidth;

    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        const auto isLast = i + 1 == m_fields.size();
        const auto width = isLast || remainingPreferredWidth == 0
                               ? remainingWidth
                               : juce::roundToInt(remainingWidth * fieldWidths[i]
                                                  / static_cast<double>(remainingPreferredWidth));
        field.bounds = area.removeFromLeft(juce::jmin(width, area.getWidth()));
        remainingWidth -= width;
        remainingPreferredWidth -= fieldWidths[i];

        auto content = field.bounds.reduced(juce::jmin(fieldPadding, field.bounds.getWidth() / 2), 0);
        const auto titleWidth = juce::jmin(titleWidths[i], content.getWidth());
        field.label.setBounds(content.removeFromLeft(titleWidth));
        content.removeFromLeft(juce::jmin(labelGap, content.getWidth()));
        field.editor.setBounds(content);

        if (!isLast)
            area.removeFromLeft(juce::jmin(fieldGap, area.getWidth()));
    }
}

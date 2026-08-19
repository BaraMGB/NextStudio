/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ClipPropertiesBar.h"
#include "PositionDisplayHelpers.h"
#include "Utilities.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr double epsilon = 1.0e-7;
constexpr int panelPadding = 6;
constexpr int fieldPadding = 6;
constexpr int labelGap = 4;
constexpr int fieldGap = 3;
constexpr float titleFontHeight = 9.0f;
constexpr float valueFontHeight = 14.0f;
constexpr int snapOffItem = 1, snapWholeItem = 2, snapAdaptiveItem = 10;
constexpr int lengthAdaptiveItem = 20, lengthWholeItem = 22;

float measureTextWidth(const juce::Font &font, const juce::String &text)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

std::optional<int> parseStrictInt(const juce::String &input)
{
    const auto text = input.trim();
    if (text.isEmpty())
        return {};
    int first = (text[0] == '+' || text[0] == '-') ? 1 : 0;
    if (first == text.length())
        return {};
    for (int i = first; i < text.length(); ++i)
        if (!juce::CharacterFunctions::isDigit(text[i]))
            return {};
    return text.getIntValue();
}
} // namespace

bool ClipPropertiesBar::PropertyEditor::keyPressed(const juce::KeyPress &key)
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
        if (editingStarted)
            editingStarted();
        selectAll();
        return true;
    }
    return isReadOnly() ? false : juce::TextEditor::keyPressed(key);
}

void ClipPropertiesBar::PropertyEditor::mouseDoubleClick(const juce::MouseEvent &event)
{
    if (!isReadOnly())
        return juce::TextEditor::mouseDoubleClick(event);
    setReadOnly(false);
    setMouseCursor(juce::MouseCursor::IBeamCursor);
    if (editingStarted)
        editingStarted();
    grabKeyboardFocus();
    selectAll();
}

void ClipPropertiesBar::PropertyEditor::mouseDown(const juce::MouseEvent &event)
{
    m_dragActive = false;
    m_lastDragStep = 0;
    juce::TextEditor::mouseDown(event);
}

void ClipPropertiesBar::PropertyEditor::mouseDrag(const juce::MouseEvent &event)
{
    if (!isReadOnly())
        return juce::TextEditor::mouseDrag(event);

    constexpr int pixelsPerStep = 4;
    const auto step = -(event.getDistanceFromDragStartY() / pixelsPerStep);
    if (!m_dragActive && std::abs(event.getDistanceFromDragStartY()) >= pixelsPerStep)
    {
        m_dragActive = true;
        if (dragStarted)
            dragStarted();
    }
    if (!m_dragActive)
        return juce::TextEditor::mouseDrag(event);

    event.source.enableUnboundedMouseMovement(true);
    if (const auto delta = step - m_lastDragStep; delta != 0 && dragUpdated)
        dragUpdated(delta);
    m_lastDragStep = step;
}

void ClipPropertiesBar::PropertyEditor::mouseUp(const juce::MouseEvent &event)
{
    if (!isReadOnly())
        return juce::TextEditor::mouseUp(event);
    if (m_dragActive)
    {
        event.source.enableUnboundedMouseMovement(false);
        if (dragEnded)
            dragEnded();
    }
    else
        juce::TextEditor::mouseUp(event);
    m_dragActive = false;
    m_lastDragStep = 0;
}

void ClipPropertiesBar::PropertyEditor::mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel)
{
    if (isReadOnly() && wheelMoved && std::abs(wheel.deltaY) > 0.0f)
        wheelMoved(wheel.deltaY > 0.0f ? 1 : -1);
}

ClipPropertiesBar::ClipPropertiesBar(EditViewState &evs) : m_evs(evs)
{
    const std::array<juce::String, 3> names{{"START", "END", "DURATION"}};
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        field.label.setText(names[i], juce::dontSendNotification);
        field.label.setJustificationType(juce::Justification::centredLeft);
        field.label.setFont(juce::Font(juce::FontOptions(titleFontHeight)));
        field.label.setBorderSize({});
        field.label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(field.label);
        addAndMakeVisible(field.editor);
        configureField(field, static_cast<int>(i) + 1);
    }
    m_snapLabel.setText("SNAP", juce::dontSendNotification);
    m_insertLengthLabel.setText("INSERT LENGTH", juce::dontSendNotification);
    for (auto *label : {&m_snapLabel, &m_insertLengthLabel})
    {
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(juce::FontOptions(titleFontHeight)));
        label->setBorderSize({});
        label->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*label);
    }

    m_snapBox.addItem("Off", snapOffItem);
    m_insertLengthBox.addItem("Adaptive", lengthAdaptiveItem);
    const std::array<int, 8> denominators{{1, 2, 4, 8, 16, 32, 64, 128}};
    for (size_t i = 0; i < denominators.size(); ++i)
    {
        const auto text = "1/" + juce::String(denominators[i]);
        m_snapBox.addItem(text, snapWholeItem + static_cast<int>(i));
        m_insertLengthBox.addItem(text, lengthWholeItem + static_cast<int>(i));
    }
    m_snapBox.addItem("Adaptive", snapAdaptiveItem);
    addAndMakeVisible(m_snapBox);
    addAndMakeVisible(m_insertLengthBox);

    const auto snapMode = static_cast<PianoRollSnapMode>(static_cast<int>(m_evs.m_clipSnapMode));
    const auto snapIndex = std::find(denominators.begin(), denominators.end(), static_cast<int>(m_evs.m_clipSnapDenominator));
    const auto snapItem = snapMode == PianoRollSnapMode::off ? snapOffItem
                          : snapMode == PianoRollSnapMode::adaptive ? snapAdaptiveItem
                          : snapIndex != denominators.end()
                                ? snapWholeItem + static_cast<int>(std::distance(denominators.begin(), snapIndex))
                                : snapWholeItem + 4;
    m_snapBox.setSelectedId(snapItem, juce::dontSendNotification);
    m_snapBox.onChange = [this, denominators]
    {
        const auto id = m_snapBox.getSelectedId();
        if (id == snapOffItem)
            m_evs.m_clipSnapMode = static_cast<int>(PianoRollSnapMode::off);
        else if (id == snapAdaptiveItem)
            m_evs.m_clipSnapMode = static_cast<int>(PianoRollSnapMode::adaptive);
        else if (id >= snapWholeItem && id < snapWholeItem + static_cast<int>(denominators.size()))
        {
            m_evs.m_clipSnapDenominator = denominators[static_cast<size_t>(id - snapWholeItem)];
            m_evs.m_clipSnapMode = static_cast<int>(PianoRollSnapMode::fixed);
        }
    };

    const auto lengthMode = static_cast<ClipInsertLengthMode>(static_cast<int>(m_evs.m_clipInsertLengthMode));
    auto lengthIndex = std::find(denominators.begin(), denominators.end(), static_cast<int>(m_evs.m_clipInsertLengthDenominator));
    m_insertLengthBox.setSelectedId(lengthMode == ClipInsertLengthMode::adaptive
                                        ? lengthAdaptiveItem
                                        : lengthWholeItem + static_cast<int>(std::distance(denominators.begin(), lengthIndex)),
                                    juce::dontSendNotification);
    if (lengthIndex == denominators.end() && lengthMode == ClipInsertLengthMode::fixed)
        m_insertLengthBox.setSelectedId(lengthWholeItem + 2, juce::dontSendNotification);
    m_insertLengthBox.onChange = [this, denominators]
    {
        const auto id = m_insertLengthBox.getSelectedId();
        if (id == lengthAdaptiveItem)
            m_evs.m_clipInsertLengthMode = static_cast<int>(ClipInsertLengthMode::adaptive);
        else if (id >= lengthWholeItem && id < lengthWholeItem + static_cast<int>(denominators.size()))
        {
            m_evs.m_clipInsertLengthDenominator = denominators[static_cast<size_t>(id - lengthWholeItem)];
            m_evs.m_clipInsertLengthMode = static_cast<int>(ClipInsertLengthMode::fixed);
        }
    };

    updateColours();
    clearSelection();
}

void ClipPropertiesBar::configureField(Field &field, int focusOrder)
{
    field.editor.setExplicitFocusOrder(focusOrder);
    field.editor.setMultiLine(false);
    field.editor.setReturnKeyStartsNewLine(false);
    field.editor.setTabKeyUsedAsCharacter(false);
    field.editor.setReadOnly(true);
    field.editor.setFont(juce::Font(juce::FontOptions(valueFontHeight)));
    field.editor.setJustification(juce::Justification::centredLeft);
    field.editor.setBorder(juce::BorderSize<int>(0, 2, 0, 2));
    field.editor.setInputRestrictions(18);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    field.editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    field.editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);

    field.editor.editingStarted = [this, &field] { beginEditing(field); };
    field.editor.onReturnKey = [this, &field] { commit(field); };
    field.editor.onEscapeKey = [this, &field] { cancel(field); };
    field.editor.onFocusLost = [this, &field]
    {
        if (m_handlingEditorCallback || field.editor.isReadOnly())
            return;
        if (apply(field.property, field.editor.getText()))
            refreshFromSelection();
        else
            cancel(field);
        field.editor.setReadOnly(true);
        field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    };
    field.editor.onTextChange = [this, &field]
    {
        if (!m_handlingEditorCallback)
            setInvalid(field, false);
    };
    field.editor.tabPressed = [this, &field](bool backwards)
    {
        if (field.editor.isReadOnly())
            return focusAdjacent(field, backwards);
        if (!apply(field.property, field.editor.getText()))
            return setInvalid(field, true);
        field.editor.setReadOnly(true);
        refreshFromSelection();
        focusAdjacent(field, backwards);
    };
    field.editor.wheelMoved = [this, &field](int steps) { scrub(field, steps); };
    field.editor.dragStarted = [this, &field] { beginScrub(field); };
    field.editor.dragUpdated = [this, &field](int steps) { scrub(field, steps, true); };
    field.editor.dragEnded = [this] { endScrub(); };
}

void ClipPropertiesBar::setEditHandlers(PreviewHandler previewHandler, CommitHandler commitHandler)
{
    m_previewHandler = std::move(previewHandler);
    m_commitHandler = std::move(commitHandler);
}

void ClipPropertiesBar::setTimingStepProvider(TimingStepProvider provider)
{
    m_timingStepProvider = std::move(provider);
}

void ClipPropertiesBar::updateColours()
{
    const auto text = m_evs.m_applicationState.getButtonTextColour();
    for (auto &field : m_fields)
    {
        field.label.setColour(juce::Label::textColourId, text.withAlpha(0.85f));
        field.editor.setColour(juce::TextEditor::textColourId, field.invalid ? juce::Colours::red : text);
    }
    for (auto *label : {&m_snapLabel, &m_insertLengthLabel})
        label->setColour(juce::Label::textColourId, text.withAlpha(0.85f));
    for (auto *box : {&m_snapBox, &m_insertLengthBox})
    {
        box->setColour(juce::ComboBox::textColourId, text);
        box->setColour(juce::ComboBox::backgroundColourId, m_evs.m_applicationState.getBackgroundColour1());
        box->setColour(juce::ComboBox::outlineColourId, m_evs.m_applicationState.getBorderColour());
        box->setColour(juce::ComboBox::arrowColourId, text);
    }
    repaint();
}

void ClipPropertiesBar::clearSelection()
{
    cancelScrub();
    m_selection.clearQuick();
    m_handlingEditorCallback = true;
    for (auto &field : m_fields)
    {
        field.displayedText = juce::String::fromUTF8("\xe2\x80\x94");
        field.editor.setText(field.displayedText, false);
        field.editor.setReadOnly(true);
        field.editor.setEnabled(false);
        field.label.setEnabled(false);
        setInvalid(field, false);
    }
    m_handlingEditorCallback = false;
    repaint();
}

void ClipPropertiesBar::refreshFromSelection(bool discardActiveEdit)
{
    auto selection = m_evs.m_selectionManager.getItemsOfType<te::Clip>();
    if (selection.isEmpty())
        return clearSelection();

    bool changed = selection.size() != m_selection.size();
    for (int i = 0; !changed && i < selection.size(); ++i)
        changed = selection[i] != m_selection[i];

    if (m_scrubActive)
    {
        if (changed)
            cancelScrub();
        else
            return;
    }

    m_selection = selection;

    if (discardActiveEdit && changed)
        for (auto &field : m_fields)
            if (field.editor.hasKeyboardFocus(true))
            {
                field.editor.setReadOnly(true);
                field.editor.giveAwayKeyboardFocus();
            }

    auto common = [this](auto getter, auto formatter)
    {
        const auto first = getter(*m_selection.getFirst());
        for (auto *clip : m_selection)
            if (std::abs(getter(*clip) - first) > epsilon)
                return juce::String::fromUTF8("\xe2\x80\x94");
        return formatter(first);
    };

    const auto start = common(
        [](const te::Clip &clip) { return clip.getPosition().getStart().inSeconds(); },
        [this](double seconds) { return formatPosition(tracktion::TimePosition::fromSeconds(seconds)); });
    const auto end = common(
        [](const te::Clip &clip) { return clip.getPosition().getEnd().inSeconds(); },
        [this](double seconds) { return formatPosition(tracktion::TimePosition::fromSeconds(seconds)); });
    const auto duration = common(
        [this](const te::Clip &clip) { return durationInBeats(clip.getPosition().time); },
        [this](double beats)
        {
            const auto startTime = tracktion::TimePosition();
            const auto endTime = m_evs.m_edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(beats));
            return formatDuration({startTime, endTime});
        });

    const std::array<juce::String, 3> values{{start, end, duration}};
    m_handlingEditorCallback = true;
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        field.displayedText = values[i];
        field.editor.setEnabled(true);
        field.label.setEnabled(true);
        setInvalid(field, false);
        if ((discardActiveEdit && changed) || !field.editor.hasKeyboardFocus(true))
            field.editor.setText(values[i], false);
    }
    m_handlingEditorCallback = false;
    repaint();
}

void ClipPropertiesBar::beginEditing(Field &field)
{
    if (field.editor.getText() == juce::String::fromUTF8("\xe2\x80\x94"))
        field.editor.clear();
}

void ClipPropertiesBar::commit(Field &field)
{
    if (!apply(field.property, field.editor.getText()))
        return setInvalid(field, true);
    m_handlingEditorCallback = true;
    field.editor.setReadOnly(true);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    field.editor.giveAwayKeyboardFocus();
    m_handlingEditorCallback = false;
    refreshFromSelection();
}

void ClipPropertiesBar::cancel(Field &field)
{
    m_handlingEditorCallback = true;
    field.editor.setText(field.displayedText, false);
    field.editor.setReadOnly(true);
    field.editor.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setInvalid(field, false);
    field.editor.giveAwayKeyboardFocus();
    m_handlingEditorCallback = false;
}

void ClipPropertiesBar::focusAdjacent(Field &field, bool backwards)
{
    auto index = static_cast<int>(&field - m_fields.data());
    index = (index + (backwards ? -1 : 1) + static_cast<int>(m_fields.size())) % static_cast<int>(m_fields.size());
    m_fields[static_cast<size_t>(index)].editor.grabKeyboardFocus();
    m_fields[static_cast<size_t>(index)].editor.selectAll();
}

void ClipPropertiesBar::beginScrub(Field &field)
{
    cancelScrub();
    m_scrubSelection = m_evs.m_selectionManager.getItemsOfType<te::Clip>();
    for (auto *clip : m_scrubSelection)
        m_scrubBase.add({clip, clip->getPosition()});
    if (m_scrubBase.isEmpty())
        return;

    m_scrubProperty = field.property;
    m_scrubSteps = 0;
    m_scrubPreview = m_scrubBase;
    m_scrubActive = true;
}

void ClipPropertiesBar::scrub(Field &field, int steps, bool fromDrag)
{
    if (!field.editor.isEnabled() || steps == 0)
        return;

    steps = juce::jlimit(-32, 32, steps);
    const auto stepBeats = m_timingStepProvider
                               ? juce::jmax(1.0 / te::Edit::ticksPerQuarterNote, m_timingStepProvider())
                               : 1.0 / te::Edit::ticksPerQuarterNote;
    const auto ticksPerStep = juce::jmax(1, juce::roundToInt(stepBeats * te::Edit::ticksPerQuarterNote));
    if (!fromDrag)
    {
        const auto ticks = static_cast<int64_t>(steps) * ticksPerStep;
        const auto relative = (ticks >= 0 ? "+" : "-") + juce::String(std::abs(ticks)) + " ticks";
        if (apply(field.property, relative))
            refreshFromSelection();
        return;
    }

    m_scrubSteps += steps;
    const auto ticks = static_cast<int64_t>(m_scrubSteps) * ticksPerStep;
    const auto relative = (ticks >= 0 ? "+" : "-")
                          + juce::String(std::abs(ticks)) + " ticks";
    if (auto plan = createEditPlan(field.property, relative, m_scrubSelection, &m_scrubBase))
    {
        m_scrubPreview = *plan;
        showPlan(m_scrubPreview);
        if (m_previewHandler)
            m_previewHandler(m_scrubPreview);
    }
}

void ClipPropertiesBar::endScrub()
{
    if (!m_scrubActive)
        return;

    const auto selection = m_evs.m_selectionManager.getItemsOfType<te::Clip>();
    bool selectionMatches = selection.size() == m_scrubSelection.size();
    for (int i = 0; selectionMatches && i < selection.size(); ++i)
        selectionMatches = selection[i] == m_scrubSelection[i];

    const auto property = m_scrubProperty;
    const auto plan = m_scrubPreview;
    const bool shouldCommit = selectionMatches && planChangesClips(plan);
    cancelScrub();

    if (shouldCommit)
    {
        if (m_commitHandler)
            m_commitHandler(property, plan);
        else
            applyPlan(plan);
    }
    refreshFromSelection();
}

void ClipPropertiesBar::cancelScrub()
{
    if (!m_scrubActive && m_scrubBase.isEmpty())
        return;
    m_scrubActive = false;
    m_scrubSteps = 0;
    m_scrubSelection.clearQuick();
    m_scrubBase.clearQuick();
    m_scrubPreview.clearQuick();
    if (m_previewHandler)
        m_previewHandler({});
}

bool ClipPropertiesBar::apply(Property property, const juce::String &input)
{
    m_selection = m_evs.m_selectionManager.getItemsOfType<te::Clip>();
    auto plan = createEditPlan(property, input, m_selection);
    if (!plan)
        return false;
    if (!planChangesClips(*plan))
        return true;

    if (m_commitHandler)
        m_commitHandler(property, *plan);
    else
        applyPlan(*plan);
    return true;
}

std::optional<juce::Array<ClipPropertyEdit>> ClipPropertiesBar::createEditPlan(
    Property property, const juce::String &input, const juce::Array<te::Clip *> &selection,
    const juce::Array<ClipPropertyEdit> *base) const
{
    const auto text = input.trim();
    if (selection.isEmpty() || text.isEmpty() || text == juce::String::fromUTF8("\xe2\x80\x94")
        || (base != nullptr && base->size() != selection.size()))
        return {};
    const bool relative = text.startsWithChar('+') || text.startsWithChar('-');

    std::optional<double> duration;
    std::optional<tracktion::TimePosition> absolutePosition;
    if (relative || property == Property::duration)
    {
        duration = parseDuration(relative ? text.substring(1) : text);
        if (!duration)
            return {};
        if (relative && text.startsWithChar('-'))
            *duration = -*duration;
    }
    else if (!(absolutePosition = parsePosition(text)))
        return {};

    for (int i = 0; i < selection.size(); ++i)
        if (selection[i] == nullptr || (base != nullptr && base->getReference(i).clip != selection[i]))
            return {};

    const auto firstPosition = base != nullptr ? base->getFirst().position : selection.getFirst()->getPosition();
    const auto startBeat = m_evs.m_edit.tempoSequence.toBeats(firstPosition.getStart()).inBeats();
    const auto endBeat = m_evs.m_edit.tempoSequence.toBeats(firstPosition.getEnd()).inBeats();
    juce::Array<ClipPropertyEdit> plan;

    if (property == Property::start)
    {
        const auto newStart = relative
            ? m_evs.m_edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(startBeat + *duration))
            : *absolutePosition;
        plan = EngineHelpers::calculateSelectedClipMove(
            (newStart - firstPosition.getStart()).inSeconds(), m_evs);
    }
    else
    {
        const auto newEndBeat = property == Property::duration
            ? startBeat + (relative ? endBeat - startBeat + *duration : *duration)
            : (relative ? endBeat + *duration
                        : m_evs.m_edit.tempoSequence.toBeats(*absolutePosition).inBeats());
        const auto newEnd = m_evs.m_edit.tempoSequence.toTime(
            tracktion::BeatPosition::fromBeats(newEndBeat));
        plan = EngineHelpers::calculateSelectedClipResize(
            false, (newEnd - firstPosition.getEnd()).inSeconds(), m_evs);
    }

    if (plan.size() != selection.size())
        return {};
    for (const auto &edit : plan)
        if (edit.clip == nullptr || !selection.contains(edit.clip)
            || edit.position.getLength() <= tracktion::TimeDuration()
            || edit.position.getStart() < tracktion::TimePosition()
            || edit.position.getEnd() > te::Edit::getMaximumEditEnd())
            return {};
    return plan;
}

void ClipPropertiesBar::applyPlan(const juce::Array<ClipPropertyEdit> &plan)
{
    for (const auto &edit : plan)
        if (edit.clip != nullptr)
            edit.clip->setPosition(edit.position);
}

void ClipPropertiesBar::showPlan(const juce::Array<ClipPropertyEdit> &plan)
{
    if (plan.isEmpty())
        return;
    auto common = [&plan](auto getter, auto formatter)
    {
        const auto first = getter(plan.getFirst());
        for (const auto &edit : plan)
            if (std::abs(getter(edit) - first) > epsilon)
                return juce::String::fromUTF8("\xe2\x80\x94");
        return formatter(first);
    };
    const std::array<juce::String, 3> values{{
        common([](const auto &edit) { return edit.position.getStart().inSeconds(); },
               [this](double value) { return formatPosition(tracktion::TimePosition::fromSeconds(value)); }),
        common([](const auto &edit) { return edit.position.getEnd().inSeconds(); },
               [this](double value) { return formatPosition(tracktion::TimePosition::fromSeconds(value)); }),
        common([this](const auto &edit) { return durationInBeats(edit.position.time); },
               [this](double value)
               {
                   return formatDuration({tracktion::TimePosition(), m_evs.m_edit.tempoSequence.toTime(
                       tracktion::BeatPosition::fromBeats(value))});
               })}};
    m_handlingEditorCallback = true;
    for (size_t i = 0; i < m_fields.size(); ++i)
        m_fields[i].editor.setText(values[i], false);
    m_handlingEditorCallback = false;
}

bool ClipPropertiesBar::planChangesClips(const juce::Array<ClipPropertyEdit> &plan) const
{
    for (const auto &edit : plan)
        if (edit.clip != nullptr
            && (std::abs(edit.position.getStart().inSeconds() - edit.clip->getPosition().getStart().inSeconds()) > epsilon
                || std::abs(edit.position.getEnd().inSeconds() - edit.clip->getPosition().getEnd().inSeconds()) > epsilon))
            return true;
    return false;
}

juce::String ClipPropertiesBar::formatPosition(tracktion::TimePosition time) const
{
    return PositionDisplayHelpers::formatBarsBeatsTicks(
        m_evs.m_edit.tempoSequence.getInternalSequence(), time, te::Edit::ticksPerQuarterNote);
}

juce::String ClipPropertiesBar::formatDuration(tracktion::TimeRange range) const
{
    const auto beats = durationInBeats(range);
    const std::array<std::pair<const char *, double>, 8> values{{
        {"1/1", 4.0}, {"1/2", 2.0}, {"1/4", 1.0}, {"1/8", 0.5},
        {"1/16", 0.25}, {"1/32", 0.125}, {"1/64", 0.0625}, {"1/128", 0.03125}}};
    for (const auto &[name, value] : values)
        if (std::abs(beats - value) < epsilon)
            return name;
    return juce::String(static_cast<int>(std::round(beats * te::Edit::ticksPerQuarterNote))) + " ticks";
}

std::optional<tracktion::TimePosition> ClipPropertiesBar::parsePosition(const juce::String &text) const
{
    return PositionDisplayHelpers::parseBarsBeatsTicks(
        m_evs.m_edit.tempoSequence.getInternalSequence(), text, te::Edit::ticksPerQuarterNote);
}

std::optional<double> ClipPropertiesBar::parseDuration(const juce::String &input) const
{
    const auto text = input.trim();
    if (text.endsWithIgnoreCase("ticks"))
    {
        const auto ticks = parseStrictInt(text.dropLastCharacters(5).trim());
        if (!ticks || *ticks < 0)
            return {};
        return *ticks / static_cast<double>(te::Edit::ticksPerQuarterNote);
    }
    const auto parts = juce::StringArray::fromTokens(text, "/", "");
    if (parts.size() != 2)
        return {};
    const auto numerator = parseStrictInt(parts[0]);
    const auto denominator = parseStrictInt(parts[1]);
    if (!numerator || !denominator || *numerator <= 0 || *denominator <= 0)
        return {};
    return 4.0 * *numerator / *denominator;
}

double ClipPropertiesBar::durationInBeats(tracktion::TimeRange range) const
{
    return m_evs.m_edit.tempoSequence.toBeats(range.getEnd()).inBeats()
           - m_evs.m_edit.tempoSequence.toBeats(range.getStart()).inBeats();
}

void ClipPropertiesBar::setInvalid(Field &field, bool invalid)
{
    field.invalid = invalid;
    field.editor.setColour(juce::TextEditor::textColourId,
                           invalid ? juce::Colours::red : m_evs.m_applicationState.getButtonTextColour());
    field.editor.repaint();
}

void ClipPropertiesBar::paint(juce::Graphics &g)
{
    const auto text = m_evs.m_applicationState.getButtonTextColour();
    auto selectionArea = m_selectionCountBounds.reduced(fieldPadding, 0);
    g.setFont(juce::Font(juce::FontOptions(titleFontHeight)));
    g.setColour(text.withAlpha(m_selection.isEmpty() ? 0.4f : 0.85f));
    const auto titleWidth = juce::roundToInt(measureTextWidth(g.getCurrentFont(), "SELECTED CLIPS:")) + 4;
    g.drawFittedText("SELECTED CLIPS:", selectionArea.removeFromLeft(titleWidth), juce::Justification::centredLeft, 1);
    selectionArea.removeFromLeft(labelGap);
    g.setFont(juce::Font(juce::FontOptions(valueFontHeight)));
    g.setColour(text.withAlpha(m_selection.isEmpty() ? 0.5f : 1.0f));
    g.drawFittedText(juce::String(m_selection.size()), selectionArea, juce::Justification::centredLeft, 1);

    g.setColour(m_evs.m_applicationState.getBorderColour());
    for (const auto &field : m_fields)
        g.drawVerticalLine(field.bounds.getX() - ((fieldGap + 1) / 2), 3.0f, static_cast<float>(getHeight() - 3));
    for (const auto x : {m_snapControlBounds.getX(), m_insertLengthControlBounds.getX()})
        if (x > 0)
            g.drawVerticalLine(x - ((fieldGap + 1) / 2), 3.0f, static_cast<float>(getHeight() - 3));
    for (const auto &field : m_fields)
        if (!field.editor.isReadOnly() && field.editor.hasKeyboardFocus(true))
        {
            g.setColour(field.invalid ? juce::Colours::red : text.withAlpha(0.7f));
            g.drawRoundedRectangle(field.editor.getBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
        }
}

void ClipPropertiesBar::resized()
{
    const auto titleFont = juce::Font(juce::FontOptions(titleFontHeight));
    const auto valueFont = juce::Font(juce::FontOptions(valueFontHeight));
    auto fullArea = getLocalBounds().reduced(4, 1);
    auto controls = fullArea.removeFromRight(juce::jmin(fullArea.getWidth(),
        juce::jmin(390, juce::jmax(300, fullArea.getWidth() / 3))));
    fullArea.removeFromRight(juce::jmin(fieldGap, fullArea.getWidth()));
    auto snapArea = controls.removeFromLeft(juce::roundToInt(controls.getWidth() * 0.38f));
    controls.removeFromLeft(juce::jmin(fieldGap, controls.getWidth()));
    auto lengthArea = controls;
    m_snapControlBounds = snapArea;
    m_insertLengthControlBounds = lengthArea;

    snapArea = snapArea.reduced(fieldPadding, 2);
    const auto snapLabelWidth = juce::roundToInt(measureTextWidth(titleFont, m_snapLabel.getText())) + 4;
    m_snapLabel.setBounds(snapArea.removeFromLeft(juce::jmin(snapLabelWidth, snapArea.getWidth())));
    snapArea.removeFromLeft(juce::jmin(labelGap, snapArea.getWidth()));
    m_snapBox.setBounds(snapArea);

    lengthArea = lengthArea.reduced(fieldPadding, 2);
    const auto lengthLabelWidth = juce::roundToInt(measureTextWidth(titleFont, m_insertLengthLabel.getText())) + 4;
    m_insertLengthLabel.setBounds(lengthArea.removeFromLeft(juce::jmin(lengthLabelWidth, lengthArea.getWidth())));
    lengthArea.removeFromLeft(juce::jmin(labelGap, lengthArea.getWidth()));
    m_insertLengthBox.setBounds(lengthArea);

    auto area = fullArea.reduced(panelPadding, 1);
    const auto selectionWidth = juce::roundToInt(measureTextWidth(titleFont, "SELECTED CLIPS:"))
                                + juce::roundToInt(measureTextWidth(valueFont, "8888"))
                                + fieldPadding * 2 + labelGap + 8;
    m_selectionCountBounds = area.removeFromLeft(juce::jmin(selectionWidth, area.getWidth()));
    area.removeFromLeft(juce::jmin(fieldGap, area.getWidth()));

    const std::array<juce::String, 3> stableValues{{"88.8.888", "88.8.888", "888 ticks"}};
    std::array<int, 3> preferred{};
    int total = 0;
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        preferred[i] = juce::roundToInt(measureTextWidth(titleFont, m_fields[i].label.getText())
                                        + measureTextWidth(valueFont, stableValues[i]))
                       + fieldPadding * 2 + labelGap + 8;
        total += preferred[i];
    }
    const auto available = juce::jmin(area.getWidth(), total + fieldGap * 2);
    area = area.removeFromLeft(available);
    int remaining = juce::jmax(0, available - fieldGap * 2);
    int remainingPreferred = total;
    for (size_t i = 0; i < m_fields.size(); ++i)
    {
        auto &field = m_fields[i];
        const bool last = i + 1 == m_fields.size();
        const auto width = last ? remaining : juce::roundToInt(remaining * preferred[i] / static_cast<double>(remainingPreferred));
        field.bounds = area.removeFromLeft(juce::jmin(width, area.getWidth()));
        auto content = field.bounds.reduced(fieldPadding, 0);
        const auto labelWidth = juce::jmin(content.getWidth(), juce::roundToInt(measureTextWidth(titleFont, field.label.getText())) + 4);
        field.label.setBounds(content.removeFromLeft(labelWidth));
        content.removeFromLeft(juce::jmin(labelGap, content.getWidth()));
        field.editor.setBounds(content);
        remaining -= width;
        remainingPreferred -= preferred[i];
        if (!last)
            area.removeFromLeft(juce::jmin(fieldGap, area.getWidth()));
    }

    // Keep the timeline controls directly beside the property fields, as in
    // NotePropertiesBar, instead of pinning them to the far-right window edge.
    auto controlsArea = getLocalBounds().reduced(4, 1);
    const auto controlsX = m_fields.back().bounds.getRight() + fieldGap;
    controlsArea.removeFromLeft(juce::jmin(controlsX - controlsArea.getX(), controlsArea.getWidth()));
    controlsArea = controlsArea.removeFromLeft(juce::jmin(390, controlsArea.getWidth()));
    auto finalSnapArea = controlsArea.removeFromLeft(juce::roundToInt(controlsArea.getWidth() * 0.38f));
    controlsArea.removeFromLeft(juce::jmin(fieldGap, controlsArea.getWidth()));
    auto finalLengthArea = controlsArea;
    m_snapControlBounds = finalSnapArea;
    m_insertLengthControlBounds = finalLengthArea;

    finalSnapArea = finalSnapArea.reduced(fieldPadding, 2);
    m_snapLabel.setBounds(finalSnapArea.removeFromLeft(juce::jmin(snapLabelWidth, finalSnapArea.getWidth())));
    finalSnapArea.removeFromLeft(juce::jmin(labelGap, finalSnapArea.getWidth()));
    m_snapBox.setBounds(finalSnapArea);

    finalLengthArea = finalLengthArea.reduced(fieldPadding, 2);
    m_insertLengthLabel.setBounds(finalLengthArea.removeFromLeft(juce::jmin(lengthLabelWidth, finalLengthArea.getWidth())));
    finalLengthArea.removeFromLeft(juce::jmin(labelGap, finalLengthArea.getWidth()));
    m_insertLengthBox.setBounds(finalLengthArea);
}

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "MidiNotePropertyEdit.h"
#include <array>
#include <functional>
#include <optional>

namespace te = tracktion_engine;

/** Compact editor for the notes selected in the piano roll. */
class NotePropertiesBar : public juce::Component
{
public:
    enum class Property
    {
        start,
        end,
        duration,
        pitch,
        velocity
    };

    using SelectionProvider = std::function<juce::Array<std::pair<te::MidiClip *, te::MidiNote *>>() >;
    using PreviewHandler = std::function<void(const juce::Array<MidiNotePropertyEdit> &)>;
    using CommitHandler = std::function<void(Property, const juce::Array<MidiNotePropertyEdit> &)>;
    using TimingStepProvider = std::function<double()>;

    explicit NotePropertiesBar(EditViewState &);

    void setSelectionProvider(SelectionProvider);
    void setEditHandlers(PreviewHandler, CommitHandler);
    void setTimingStepProvider(TimingStepProvider);
    void refreshFromSelection(bool discardActiveEdit = false);
    void clearSelection();
    void updateColours();

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    class PropertyEditor : public juce::TextEditor
    {
    public:
        std::function<void(bool)> tabPressed;
        std::function<void(int)> wheelMoved;
        std::function<void()> focusGainedCallback;
        std::function<void()> textEditingStarted;
        std::function<void()> dragStarted;
        std::function<void(int)> dragUpdated;
        std::function<void()> dragEnded;

        bool keyPressed(const juce::KeyPress &) override;
        void focusGained(FocusChangeType) override;
        void mouseDown(const juce::MouseEvent &) override;
        void mouseDoubleClick(const juce::MouseEvent &) override;
        void mouseDrag(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;
        void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;

    private:
        bool m_dragActive{false};
        int m_lastDragStep{0};
    };

    struct Field
    {
        Property property;
        juce::Label label;
        PropertyEditor editor;
        juce::String displayedText;
        juce::Rectangle<int> bounds;
        bool invalid{false};
    };

    void configureField(Field &, int focusOrder);
    void beginEditing(Field &);
    void commit(Field &);
    void cancel(Field &);
    void focusAdjacent(Field &, bool backwards);
    void scrub(Field &, int stepDelta, bool beginUndoTransaction = true);
    void beginScrub(Field &);
    void endScrub();
    void cancelScrub();
    void beginUndoTransaction(Property);
    bool apply(Property, const juce::String &, bool beginUndoTransaction = true);
    std::optional<juce::Array<MidiNotePropertyEdit>> createEditPlan(
        Property, const juce::String &,
        const juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> &,
        const juce::Array<MidiNotePropertyEdit> *base = nullptr) const;
    void applyPlan(const juce::Array<MidiNotePropertyEdit> &);
    void showPlan(const juce::Array<MidiNotePropertyEdit> &);
    bool planChangesNotes(const juce::Array<MidiNotePropertyEdit> &) const;
    void setInvalid(Field &, bool);

    juce::String formatPosition(double globalBeat) const;
    std::optional<double> parsePosition(const juce::String &) const;
    juce::String formatDuration(double beats) const;
    std::optional<double> parseDuration(const juce::String &) const;
    std::optional<int> parsePitch(const juce::String &) const;

    double getGlobalStart(const te::MidiClip &, const te::MidiNote &) const;
    double getInternalStart(const te::MidiClip &, double globalBeat) const;

    EditViewState &m_evs;
    SelectionProvider m_selectionProvider;
    PreviewHandler m_previewHandler;
    CommitHandler m_commitHandler;
    TimingStepProvider m_timingStepProvider;
    std::array<Field, 5> m_fields{{
        {Property::start, {}, {}, {}},
        {Property::end, {}, {}, {}},
        {Property::duration, {}, {}, {}},
        {Property::pitch, {}, {}, {}},
        {Property::velocity, {}, {}, {}}}};
    juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> m_selection;
    juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> m_scrubSelection;
    juce::Array<MidiNotePropertyEdit> m_scrubBase;
    juce::Array<MidiNotePropertyEdit> m_scrubPreview;
    Property m_scrubProperty{Property::start};
    int m_scrubSteps{0};
    bool m_scrubActive{false};
    juce::Label m_snapLabel;
    juce::ComboBox m_snapBox;
    juce::Label m_noteLengthLabel;
    juce::ComboBox m_noteLengthBox;
    juce::Rectangle<int> m_panelBounds;
    juce::Rectangle<int> m_selectionCountBounds;
    juce::Rectangle<int> m_snapControlBounds;
    juce::Rectangle<int> m_noteLengthControlBounds;
    bool m_handlingEditorCallback{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotePropertiesBar)
};

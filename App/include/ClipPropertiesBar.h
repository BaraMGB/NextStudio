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
#include "ClipPropertyEdit.h"
#include <array>
#include <functional>
#include <optional>

namespace te = tracktion_engine;

/** Compact editor for clips selected in the song editor. */
class ClipPropertiesBar : public juce::Component
{
public:
    enum class Property { start, end, duration };

    using PreviewHandler = std::function<void(const juce::Array<ClipPropertyEdit> &)>;
    using CommitHandler = std::function<void(Property, const juce::Array<ClipPropertyEdit> &)>;
    using TimingStepProvider = std::function<double()>;

    explicit ClipPropertiesBar(EditViewState &);

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
        std::function<void()> editingStarted;
        std::function<void()> dragStarted;
        std::function<void(int)> dragUpdated;
        std::function<void()> dragEnded;

        bool keyPressed(const juce::KeyPress &) override;
        void mouseDoubleClick(const juce::MouseEvent &) override;
        void mouseDown(const juce::MouseEvent &) override;
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
    void scrub(Field &, int steps, bool fromDrag = false);
    void beginScrub(Field &);
    void endScrub();
    void cancelScrub();
    bool apply(Property, const juce::String &);
    std::optional<juce::Array<ClipPropertyEdit>> createEditPlan(
        Property, const juce::String &, const juce::Array<te::Clip *> &,
        const juce::Array<ClipPropertyEdit> *base = nullptr) const;
    void applyPlan(const juce::Array<ClipPropertyEdit> &);
    void showPlan(const juce::Array<ClipPropertyEdit> &);
    bool planChangesClips(const juce::Array<ClipPropertyEdit> &) const;
    void setInvalid(Field &, bool);

    juce::String formatPosition(tracktion::TimePosition) const;
    juce::String formatDuration(tracktion::TimeRange) const;
    std::optional<tracktion::TimePosition> parsePosition(const juce::String &) const;
    std::optional<double> parseDuration(const juce::String &) const;
    double durationInBeats(tracktion::TimeRange) const;

    EditViewState &m_evs;
    PreviewHandler m_previewHandler;
    CommitHandler m_commitHandler;
    TimingStepProvider m_timingStepProvider;
    std::array<Field, 3> m_fields{{
        {Property::start, {}, {}, {}},
        {Property::end, {}, {}, {}},
        {Property::duration, {}, {}, {}}}};
    juce::Array<te::Clip *> m_selection;
    juce::Array<te::Clip *> m_scrubSelection;
    juce::Array<ClipPropertyEdit> m_scrubBase;
    juce::Array<ClipPropertyEdit> m_scrubPreview;
    Property m_scrubProperty{Property::start};
    int m_scrubSteps{0};
    bool m_scrubActive{false};
    bool m_handlingEditorCallback{false};
    juce::Label m_snapLabel;
    juce::ComboBox m_snapBox;
    juce::Label m_insertLengthLabel;
    juce::ComboBox m_insertLengthBox;
    juce::Rectangle<int> m_selectionCountBounds;
    juce::Rectangle<int> m_snapControlBounds;
    juce::Rectangle<int> m_insertLengthControlBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipPropertiesBar)
};

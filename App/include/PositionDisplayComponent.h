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
#include "ApplicationViewState.h"
#include "EditViewState.h"
#include "PositionDisplayInfoState.h"
#include "PositionDisplayLayout.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PositionDisplayField;

class PositionDisplayComponent : public juce::Component,
                                 public juce::SettableTooltipClient,
                                 private juce::AsyncUpdater,
                                 private juce::ValueTree::Listener,
                                 private juce::Timer
{
public:
    PositionDisplayComponent(EditViewState &editViewState, ApplicationViewState &appState);
    ~PositionDisplayComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void projectSaved();
    void clearSaveFeedback();
    void setSaveFeedbackBlocked(bool blocked);

    static constexpr int minimumWidth = 600;

private:
    enum class FieldId
    {
        bpm,
        timeSignature,
        position,
        time,
        loopIn,
        loopOut
    };

    struct DisplaySnapshot
    {
        double bpm{};
        int numerator{};
        int denominator{};
        tracktion::TimePosition position{};
        tracktion::TimeRange loopRange{};

        juce::String bpmText;
        juce::String timeSignatureText;
        juce::String positionText;
        juce::String timeText;
        juce::String loopInText;
        juce::String loopOutText;
    };

    struct DragState
    {
        bool active{false};
        FieldId field{FieldId::bpm};
        int segment{0};
        bool undoTransactionStarted{false};
        DisplaySnapshot anchor{};
    };

    void handleAsyncUpdate() override;
    void timerCallback() override;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;
    void valueTreeParentChanged(juce::ValueTree &) override;
    void valueTreeRedirected(juce::ValueTree &) override;

    void scheduleRefresh(bool rebuildTempoListeners = false);
    void rebuildTempoSequenceListeners();
    void refreshFromModel();
    void updateFieldStyles();
    PositionDisplayLayout::Layout calculateResponsiveLayout() const;
    void updateLayoutSeparators(const PositionDisplayLayout::Layout &layout);
    void applyFieldLayout(const PositionDisplayLayout::Layout &layout);
    void refreshSaveFeedbackTimer();
    void updateInfoTooltip();
    void updateFieldTitles();
    juce::String getDisplayedEditName() const;
    juce::String getSaveStatusText() const;

    PositionDisplayField &fieldForId(FieldId);
    const PositionDisplayField &fieldForId(FieldId) const;

    void beginDrag(FieldId field, int segmentIndex);
    void updateDrag(FieldId field, int segmentIndex, int stepDelta, juce::ModifierKeys modifiers);
    void endDrag(FieldId field);

    bool commitBpm(const juce::String &text);
    bool commitTimeSignature(const juce::String &text);
    bool commitTransportPositionFromBarsBeats(const juce::String &text);
    bool commitTransportPositionFromTime(const juce::String &text);
    bool commitLoopIn(const juce::String &text);
    bool commitLoopOut(const juce::String &text);

    void applyTempo(double bpm);
    void applyTimeSignature(int numerator, int denominator);
    void applyTransportPosition(tracktion::TimePosition position);
    void applyLoopIn(tracktion::TimePosition position);
    void applyLoopOut(tracktion::TimePosition position);

    EditViewState &m_editViewState;
    te::Edit &m_edit;
    ApplicationViewState &m_appState;
    PositionDisplayInfoState m_infoState;
    PositionDisplayLayout::Mode m_layoutMode{PositionDisplayLayout::Mode::full};
    juce::Rectangle<int> m_infoBounds;

    std::unique_ptr<PositionDisplayField> m_bpmField;
    std::unique_ptr<PositionDisplayField> m_timeSignatureField;
    std::unique_ptr<PositionDisplayField> m_positionField;
    std::unique_ptr<PositionDisplayField> m_timeField;
    std::unique_ptr<PositionDisplayField> m_loopInField;
    std::unique_ptr<PositionDisplayField> m_loopOutField;

    juce::Array<juce::ValueTree> m_tempoItemStates;
    juce::ValueTree m_themeState;
    bool m_needsTempoListenerRebuild{true};
    DisplaySnapshot m_snapshot{};
    DragState m_dragState{};
    int m_infoSeparatorX{};
    int m_leftGroupSeparatorX{};
    int m_rightGroupSeparatorX{};
    bool m_infoSeparatorVisible{false};
    bool m_sideSeparatorsVisible{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PositionDisplayComponent)
};

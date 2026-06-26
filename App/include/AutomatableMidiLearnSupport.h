/*
  ==============================================================================

    AutomatableMidiLearnSupport.h
    Created: 21 May 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomatableMidiLearnSupport final : private juce::Timer
{
public:
    AutomatableMidiLearnSupport(juce::Component &owner, te::AutomatableParameter::Ptr parameter);
    ~AutomatableMidiLearnSupport() override;

    void setParameter(te::AutomatableParameter::Ptr parameter);

    void addContextMenuItems(juce::PopupMenu &menu, int learnItemId, int removeItemId);
    bool handleContextMenuResult(int result, int learnItemId, int removeItemId);

    bool keyPressed(const juce::KeyPress &key);
    void paintOverChildren(juce::Graphics &g, juce::Rectangle<int> localBounds, bool mouseOver);

    void refreshMappingState();
    bool removeMidiMapping();
    void cancelMidiLearn();

    [[nodiscard]] bool isMapped() const { return m_midiControllerId >= 0; }
    [[nodiscard]] bool isLearning() const { return m_isMidiLearnPending; }
    [[nodiscard]] int getMidiControllerId() const { return m_midiControllerId; }
    [[nodiscard]] int getMidiChannel() const { return m_midiChannel; }

private:
    struct MidiLearnListener;
    struct MidiMappingSnapshot;
    friend struct MidiLearnListener;

    void beginMidiLearn();
    void finishMidiLearn();
    void restoreMidiMappingSnapshot();
    bool resolveMidiMappingConflict();
    void updateTooltip();
    void timerCallback() override;
    void handleMidiLearnStatusChanged(bool isActive);
    void handleMidiLearnAssignmentChanged(te::MidiLearnState::ChangeType);

    [[nodiscard]] te::MidiLearnState *getMidiLearnState() const;
    [[nodiscard]] te::ParameterControlMappings *getParameterControlMappings() const;
    [[nodiscard]] juce::String getMidiMappingLabel() const;

    juce::Component &m_owner;
    te::AutomatableParameter::Ptr m_parameter;
    std::unique_ptr<MidiLearnListener> m_midiLearnListener;
    std::unique_ptr<MidiMappingSnapshot> m_midiMappingSnapshot;
    bool m_isMidiLearnPending = false;
    int m_midiChannel = -1;
    int m_midiControllerId = -1;
    double m_midiLearnStartedMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomatableMidiLearnSupport)
};

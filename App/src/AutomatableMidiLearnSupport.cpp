/*
  ==============================================================================

    AutomatableMidiLearnSupport.cpp
    Created: 21 May 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "AutomatableMidiLearnSupport.h"

namespace
{
constexpr double midiLearnTimeoutMs = 10000.0;

juce::String getControllerDescription(int controllerId, int channel)
{
    const auto channelText = juce::String{"Channel "} + juce::String(channel);

    if (controllerId >= 0x40000)
        return "Channel Pressure (" + channelText + ")";

    if (controllerId >= 0x30000)
        return "RPN #" + juce::String(controllerId & 0x7fff) + " (" + channelText + ")";

    if (controllerId >= 0x20000)
        return "NRPN #" + juce::String(controllerId & 0x7fff) + " (" + channelText + ")";

    if (controllerId >= 0x10000)
    {
        const auto cc = controllerId & 0x7f;

        auto label = "CC " + juce::String(cc);
        const juce::String controllerName{juce::MidiMessage::getControllerName(cc)};

        if (controllerName.isNotEmpty())
            label << " (" << controllerName << ")";

        return label + " - " + channelText;
    }

    return "MIDI Controller";
}
}

struct AutomatableMidiLearnSupport::MidiLearnListener final : private te::MidiLearnState::Listener
{
    MidiLearnListener(AutomatableMidiLearnSupport &ownerIn, te::MidiLearnState &state)
        : te::MidiLearnState::Listener(state)
        , owner(ownerIn)
    {
    }

    void midiLearnStatusChanged(bool isActive) override { owner.handleMidiLearnStatusChanged(isActive); }
    void midiLearnAssignmentChanged(te::MidiLearnState::ChangeType changeType) override { owner.handleMidiLearnAssignmentChanged(changeType); }

    AutomatableMidiLearnSupport &owner;
};

struct AutomatableMidiLearnSupport::MidiMappingSnapshot final
{
    struct Entry
    {
        te::AutomatableParameter::Ptr parameter;
        int controllerId = -1;
        int channel = -1;
    };

    void capture(te::ParameterControlMappings &mappings)
    {
        entries.clear();

        for (int row = 0; row < mappings.getNumControllerIDs(); ++row)
        {
            auto mapping = mappings.getMappingForRow(row);

            if (mapping.parameter != nullptr && mapping.controllerID != 0)
                entries.push_back({mapping.parameter, mapping.controllerID, mapping.channelID});
        }
    }

    void restore(te::Edit &edit, te::ParameterControlMappings &mappings) const
    {
        for (int row = mappings.getNumControllerIDs(); --row >= 0;)
            mappings.removeMapping(row);

        mappings.saveToEdit();

        if (!entries.empty())
        {
            auto *undoManager = &edit.getUndoManager();
            auto state = edit.state.getOrCreateChildWithName(te::IDs::CONTROLLERMAPPINGS, undoManager);

            for (const auto &entry : entries)
            {
                if (entry.parameter == nullptr || entry.controllerId == 0)
                    continue;

                auto mappingState = te::createValueTree(te::IDs::MAP,
                                                        te::IDs::id, entry.controllerId,
                                                        te::IDs::channel, entry.channel,
                                                        te::IDs::param, entry.parameter->getFullName(),
                                                        te::IDs::pluginID, entry.parameter->getOwnerID());
                state.addChild(mappingState, -1, undoManager);
            }
        }

        mappings.loadFromEdit();
    }

    std::vector<Entry> entries;
};

AutomatableMidiLearnSupport::AutomatableMidiLearnSupport(juce::Component &owner, te::AutomatableParameter::Ptr parameter)
    : m_owner(owner)
{
    setParameter(parameter);
}

AutomatableMidiLearnSupport::~AutomatableMidiLearnSupport()
{
    cancelMidiLearn();
    m_midiLearnListener.reset();

    if (auto *tooltipClient = dynamic_cast<juce::SettableTooltipClient *>(&m_owner))
        tooltipClient->setTooltip(juce::String{});
}

void AutomatableMidiLearnSupport::setParameter(te::AutomatableParameter::Ptr parameter)
{
    cancelMidiLearn();
    m_midiLearnListener.reset();
    m_parameter = parameter;

    if (m_parameter != nullptr)
        m_midiLearnListener = std::make_unique<MidiLearnListener>(*this, m_parameter->getEdit().engine.getMidiLearnState());

    refreshMappingState();
}

void AutomatableMidiLearnSupport::addContextMenuItems(juce::PopupMenu &menu, int learnItemId, int removeItemId)
{
    refreshMappingState();
    menu.addItem(learnItemId, isMapped() ? "Change MIDI Mapping" : "MIDI Learn");
    menu.addItem(removeItemId, "Remove MIDI Mapping", isMapped());
}

bool AutomatableMidiLearnSupport::handleContextMenuResult(int result, int learnItemId, int removeItemId)
{
    if (result == learnItemId)
    {
        beginMidiLearn();
        return true;
    }

    if (result == removeItemId)
    {
        removeMidiMapping();
        return true;
    }

    return false;
}

bool AutomatableMidiLearnSupport::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::escapeKey && m_isMidiLearnPending)
    {
        cancelMidiLearn();
        return true;
    }

    return false;
}

void AutomatableMidiLearnSupport::paintOverChildren(juce::Graphics &g, juce::Rectangle<int> localBounds, bool mouseOver)
{
    if (m_parameter == nullptr)
        return;

    if (m_midiControllerId >= 0)
    {
        auto badgeArea = localBounds.toFloat();
        badgeArea.reduce(0.f, 5.f);
        auto badgeBounds = badgeArea.removeFromTop(5.0f).removeFromRight(5.f);
        g.setColour(juce::Colours::deepskyblue.withAlpha(mouseOver ? 0.95f : 0.75f));
        g.fillEllipse(badgeBounds);
    }

    if (m_isMidiLearnPending)
    {
        const auto bounds = localBounds.toFloat().reduced(3.0f);
        const auto elapsedMs = juce::Time::getMillisecondCounterHiRes() - m_midiLearnStartedMs;
        const auto pulse = 0.5f + 0.5f * std::sin((float)(elapsedMs * 0.012));
        const auto borderColour = juce::Colours::orange.withAlpha(0.5f + 0.35f * pulse);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds, 5.0f, 1.5f + pulse * 0.6f);
    }
}

void AutomatableMidiLearnSupport::beginMidiLearn()
{
    if (m_parameter == nullptr)
        return;

    auto *learnState = getMidiLearnState();
    auto *mappings = getParameterControlMappings();

    if (learnState == nullptr || mappings == nullptr)
        return;

    m_midiMappingSnapshot = std::make_unique<MidiMappingSnapshot>();
    m_midiMappingSnapshot->capture(*mappings);

    refreshMappingState();
    m_isMidiLearnPending = true;
    m_midiLearnStartedMs = juce::Time::getMillisecondCounterHiRes();
    updateTooltip();

    learnState->setActive(true);
    m_parameter->getEdit().getParameterChangeHandler().parameterChanged(*m_parameter, false);

    startTimerHz(30);
    m_owner.grabKeyboardFocus();
    m_owner.repaint();
}

void AutomatableMidiLearnSupport::cancelMidiLearn()
{
    if (m_parameter != nullptr)
    {
        auto &changeHandler = m_parameter->getEdit().getParameterChangeHandler();

        if (changeHandler.getPendingParam(false).get() == m_parameter.get())
            changeHandler.getPendingParam(true);
    }

    if (m_isMidiLearnPending)
        if (auto *learnState = getMidiLearnState())
            if (learnState->isActive())
                learnState->setActive(false);

    finishMidiLearn();
}

void AutomatableMidiLearnSupport::finishMidiLearn()
{
    m_isMidiLearnPending = false;
    stopTimer();
    refreshMappingState();
    m_midiMappingSnapshot.reset();
    m_owner.repaint();
}

bool AutomatableMidiLearnSupport::removeMidiMapping()
{
    auto *mappings = getParameterControlMappings();

    if (mappings == nullptr || m_parameter == nullptr)
        return false;

    bool removedAny = false;

    while (mappings->removeParameterMapping(*m_parameter))
        removedAny = true;

    if (removedAny)
    {
        refreshMappingState();

        if (auto *learnState = getMidiLearnState())
            learnState->assignmentChanged(te::MidiLearnState::removed);
    }

    return removedAny;
}

void AutomatableMidiLearnSupport::restoreMidiMappingSnapshot()
{
    auto *mappings = getParameterControlMappings();

    if (mappings == nullptr || m_parameter == nullptr || m_midiMappingSnapshot == nullptr)
        return;

    m_midiMappingSnapshot->restore(m_parameter->getEdit(), *mappings);
    refreshMappingState();

    if (auto *learnState = getMidiLearnState())
        juce::MessageManager::callAsync([learnState]
        {
            learnState->assignmentChanged(te::MidiLearnState::removed);
        });
}

bool AutomatableMidiLearnSupport::resolveMidiMappingConflict()
{
    auto *mappings = getParameterControlMappings();

    if (mappings == nullptr || m_parameter == nullptr || m_midiControllerId < 0)
        return true;

    juce::Array<int> conflictingRows;
    juce::StringArray conflictingParameters;

    for (int row = mappings->getNumControllerIDs(); --row >= 0;)
    {
        auto mapping = mappings->getMappingForRow(row);

        if (mapping.parameter != nullptr
            && mapping.parameter != m_parameter.get()
            && mapping.controllerID == m_midiControllerId
            && mapping.channelID == m_midiChannel)
        {
            conflictingRows.add(row);
            conflictingParameters.add(mapping.parameter->getFullName());
        }
    }

    if (conflictingRows.isEmpty())
        return true;

    const auto message = "This MIDI controller is already mapped to:\n\n"
                         + conflictingParameters.joinIntoString("\n")
                         + "\n\nWhat would you like to do?";

    const int result = juce::AlertWindow::showYesNoCancelBox(juce::AlertWindow::QuestionIcon,
                                                            "MIDI Controller Already Mapped",
                                                            message,
                                                            "Replace Old",
                                                            "Add New",
                                                            "Cancel");

    switch (result)
    {
    case 1:
        for (auto row : conflictingRows)
            mappings->removeMapping(row);

        if (auto *learnState = getMidiLearnState())
            juce::MessageManager::callAsync([learnState]
            {
                learnState->assignmentChanged(te::MidiLearnState::removed);
            });

        refreshMappingState();
        return true;

    case 2:
        refreshMappingState();
        return true;

    case 3:
    default:
        restoreMidiMappingSnapshot();
        return false;
    }
}

void AutomatableMidiLearnSupport::refreshMappingState()
{
    m_midiChannel = -1;
    m_midiControllerId = -1;

    if (auto *mappings = getParameterControlMappings())
        if (m_parameter != nullptr)
            mappings->getParameterMapping(*m_parameter, m_midiChannel, m_midiControllerId);

    updateTooltip();
}

void AutomatableMidiLearnSupport::updateTooltip()
{
    if (auto *tooltipClient = dynamic_cast<juce::SettableTooltipClient *>(&m_owner))
    {
        if (m_midiControllerId >= 0)
            tooltipClient->setTooltip("MIDI mapped: " + getMidiMappingLabel());
        else if (m_isMidiLearnPending)
            tooltipClient->setTooltip("Waiting for MIDI controller input. Press Esc to cancel.");
        else
            tooltipClient->setTooltip(juce::String{});
    }
}

void AutomatableMidiLearnSupport::timerCallback()
{
    if (!m_isMidiLearnPending)
    {
        stopTimer();
        return;
    }

    if ((juce::Time::getMillisecondCounterHiRes() - m_midiLearnStartedMs) >= midiLearnTimeoutMs)
    {
        cancelMidiLearn();
        return;
    }

    m_owner.repaint();
}

void AutomatableMidiLearnSupport::handleMidiLearnStatusChanged(bool isActive)
{
    if (!isActive && m_isMidiLearnPending)
        finishMidiLearn();

    updateTooltip();
    m_owner.repaint();
}

void AutomatableMidiLearnSupport::handleMidiLearnAssignmentChanged(te::MidiLearnState::ChangeType)
{
    refreshMappingState();

    if (m_isMidiLearnPending && m_midiControllerId >= 0)
    {
        resolveMidiMappingConflict();

        if (auto *learnState = getMidiLearnState())
            if (learnState->isActive())
                learnState->setActive(false);

        finishMidiLearn();
        return;
    }

    m_owner.repaint();
}

te::MidiLearnState *AutomatableMidiLearnSupport::getMidiLearnState() const
{
    if (m_parameter == nullptr)
        return nullptr;

    return &m_parameter->getEdit().engine.getMidiLearnState();
}

te::ParameterControlMappings *AutomatableMidiLearnSupport::getParameterControlMappings() const
{
    if (m_parameter == nullptr)
        return nullptr;

    return &m_parameter->getEdit().getParameterControlMappings();
}

juce::String AutomatableMidiLearnSupport::getMidiMappingLabel() const
{
    if (m_midiControllerId < 0)
        return {};

    return getControllerDescription(m_midiControllerId, m_midiChannel);
}

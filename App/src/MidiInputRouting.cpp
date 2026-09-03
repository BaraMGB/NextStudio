/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "MidiInputRouting.h"

namespace MidiInputRouting
{
namespace
{
const juce::Identifier automaticFocusProperty{"nextStudioAutomaticMidiFocus"};
const juce::Identifier routingVersionProperty{"nextStudioMidiInputRoutingVersion"};
constexpr int currentRoutingVersion = 1;

juce::ValueTree findDestination(const te::InputDeviceInstance &instance, te::EditItemID targetID)
{
    for (const auto &destination : instance.state)
        if (destination.hasType(te::IDs::INPUTDEVICEDESTINATION)
            && te::EditItemID::fromProperty(destination, te::IDs::targetID) == targetID)
            return destination;

    return {};
}

juce::Array<te::EditItemID> getAutomaticTargets(const te::InputDeviceInstance &instance)
{
    juce::Array<te::EditItemID> result;

    for (const auto &destination : instance.state)
        if (destination.hasType(te::IDs::INPUTDEVICEDESTINATION)
            && (bool)destination.getProperty(automaticFocusProperty, false))
        {
            const auto targetID = te::EditItemID::fromProperty(destination, te::IDs::targetID);
            if (targetID.isValid())
                result.addIfNotAlreadyThere(targetID);
        }

    return result;
}

bool containsDevice(const juce::Array<te::InputDevice *> &devices, const te::InputDevice &device)
{
    return devices.contains(const_cast<te::InputDevice *>(&device));
}

bool hasManualTargetOnAnotherEnabledDevice(te::Edit &edit,
                                           const te::InputDeviceInstance &instance,
                                           te::EditItemID targetID)
{
    if (!targetID.isValid())
        return false;

    for (auto *otherInstance : edit.getAllInputDevices())
        if (otherInstance != nullptr
            && otherInstance != &instance
            && otherInstance->getInputDevice().isEnabled()
            && isManualTarget(*otherInstance, targetID))
            return true;

    return false;
}

void appendError(UpdateResult &result, const juce::String &error)
{
    if (error.isEmpty())
        return;

    result.error = result.error.isEmpty() ? error : result.error + "\n" + error;
}

UpdateResult removeTarget(te::InputDeviceInstance &instance, te::EditItemID targetID, juce::UndoManager *undoManager)
{
    UpdateResult result;
    const auto removal = instance.removeTarget(targetID, undoManager);

    if (removal.wasOk())
        result.routingChanged = true;
    else
        appendError(result, removal.getErrorMessage());

    return result;
}

UpdateResult addAutomaticTarget(te::InputDeviceInstance &instance,
                                te::EditItemID targetID,
                                juce::UndoManager *undoManager)
{
    UpdateResult result;
    auto addition = instance.setTarget(targetID, false, undoManager, 0);

    if (!addition.has_value())
    {
        appendError(result, addition.error());
        return result;
    }

    addition.value()->state.setProperty(automaticFocusProperty, true, undoManager);
    result.routingChanged = true;
    result.metadataChanged = true;
    return result;
}
} // namespace

bool isAutomaticFocusTarget(const te::InputDeviceInstance &instance, te::EditItemID targetID)
{
    const auto destination = findDestination(instance, targetID);
    return destination.isValid() && (bool)destination.getProperty(automaticFocusProperty, false);
}

bool isManualTarget(const te::InputDeviceInstance &instance, te::EditItemID targetID)
{
    const auto destination = findDestination(instance, targetID);
    return destination.isValid() && !(bool)destination.getProperty(automaticFocusProperty, false);
}

te::EditItemID findMidiFocusTarget(const juce::Array<te::Track *> &selectedTracks,
                                   const juce::Array<te::Clip *> &selectedClips,
                                   const juce::Identifier &midiTrackProperty)
{
    te::EditItemID focusedTrackID;

    for (auto *track : selectedTracks)
        if (track != nullptr
            && track->isAudioTrack()
            && (bool)track->state.getProperty(midiTrackProperty))
            focusedTrackID = track->itemID;

    if (focusedTrackID.isValid())
        return focusedTrackID;

    for (auto *clip : selectedClips)
        if (clip != nullptr)
            if (auto *track = clip->getTrack();
                track != nullptr
                && track->isAudioTrack()
                && (bool)track->state.getProperty(midiTrackProperty))
                focusedTrackID = track->itemID;

    return focusedTrackID;
}

UpdateResult setManualTarget(te::InputDeviceInstance &instance,
                             te::EditItemID targetID,
                             bool shouldBeManual,
                             juce::UndoManager *undoManager)
{
    UpdateResult result;
    auto destination = findDestination(instance, targetID);

    if (shouldBeManual)
    {
        if (destination.isValid())
        {
            if ((bool)destination.getProperty(automaticFocusProperty, false))
            {
                destination.removeProperty(automaticFocusProperty, undoManager);
                result.metadataChanged = true;
            }

            return result;
        }

        auto addition = instance.setTarget(targetID, false, undoManager, 0);
        if (addition.has_value())
            result.routingChanged = true;
        else
            appendError(result, addition.error());

        return result;
    }

    if (destination.isValid() && !(bool)destination.getProperty(automaticFocusProperty, false))
        return removeTarget(instance, targetID, undoManager);

    return result;
}

UpdateResult reconcileAutomaticFocus(te::Edit &edit,
                                     const juce::Array<te::InputDevice *> &focusDevices,
                                     te::EditItemID focusedTrackID,
                                     const juce::Array<te::InputDevice *> &devicesYieldingToManualTargets,
                                     juce::UndoManager *undoManager)
{
    UpdateResult result;

    for (auto *instance : edit.getAllInputDevices())
    {
        if (instance == nullptr)
            continue;

        const bool isFocusDevice = containsDevice(focusDevices, instance->getInputDevice());
        const bool manualTargetExists = focusedTrackID.isValid() && isManualTarget(*instance, focusedTrackID);
        const bool yieldsToAnotherManualTarget = containsDevice(devicesYieldingToManualTargets,
                                                                 instance->getInputDevice())
                                                && hasManualTargetOnAnotherEnabledDevice(edit,
                                                                                         *instance,
                                                                                         focusedTrackID);
        const auto desiredAutomaticTarget = isFocusDevice
                                                    && focusedTrackID.isValid()
                                                    && !manualTargetExists
                                                    && !yieldsToAnotherManualTarget
                                                ? focusedTrackID
                                                : te::EditItemID{};

        auto automaticTargets = getAutomaticTargets(*instance);
        for (auto targetID : automaticTargets)
            if (targetID != desiredAutomaticTarget)
                result.merge(removeTarget(*instance, targetID, undoManager));

        if (desiredAutomaticTarget.isValid()
            && !isAutomaticFocusTarget(*instance, desiredAutomaticTarget)
            && !isManualTarget(*instance, desiredAutomaticTarget))
        {
            instance->getInputDevice().setMonitorMode(te::InputDevice::MonitorMode::on);
            result.merge(addAutomaticTarget(*instance, desiredAutomaticTarget, undoManager));
        }
    }

    return result;
}

UpdateResult clearAutomaticFocus(te::Edit &edit)
{
    return reconcileAutomaticFocus(edit, {}, {});
}

UpdateResult migrateLegacyFocusTargets(te::Edit &edit,
                                       const juce::Array<te::InputDevice *> &focusDevices)
{
    UpdateResult result;

    if ((int)edit.state.getProperty(routingVersionProperty, 0) >= currentRoutingVersion)
        return result;

    for (auto *instance : edit.getAllInputDevices())
    {
        if (instance == nullptr || !containsDevice(focusDevices, instance->getInputDevice()))
            continue;

        const auto targets = instance->getTargets();
        for (auto targetID : targets)
            result.merge(removeTarget(*instance, targetID, nullptr));
    }

    if (result.error.isEmpty())
    {
        edit.state.setProperty(routingVersionProperty, currentRoutingVersion, nullptr);
        result.metadataChanged = true;
    }

    return result;
}

} // namespace MidiInputRouting

/*
  ==============================================================================

    AutomationWriteGuard.h
    Created: 21 May 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <set>

namespace te = tracktion_engine;

class AutomationWriteGuard
{
public:
    // Tracktion reads automation globally. While writing, a parameter that was just
    // touched by the user/MIDI/host must ignore read-back UI updates until punch-out,
    // otherwise controls can fight the curve and appear to jitter.
    static void markTouched(te::AutomatableParameter *parameter)
    {
        if (!shouldGuard(parameter))
            return;

        const juce::ScopedLock sl(getLock());
        getTouchedParameters().insert(getKey(*parameter));
    }

    static void markTouched(const te::AutomatableParameter::Ptr &parameter)
    {
        markTouched(parameter.get());
    }

    static bool isTouched(te::AutomatableParameter *parameter)
    {
        if (parameter == nullptr)
            return false;

        if (!shouldGuard(parameter))
            return false;

        const juce::ScopedLock sl(getLock());
        return getTouchedParameters().contains(getKey(*parameter));
    }

    static bool isTouched(const te::AutomatableParameter::Ptr &parameter)
    {
        return isTouched(parameter.get());
    }

    static void clear()
    {
        const juce::ScopedLock sl(getLock());
        getTouchedParameters().clear();
    }

private:
    struct ParameterKey
    {
        // AutomatableParameter::paramID is unique within its owner in Tracktion's
        // automation model, but not globally. Use the owner ID to disambiguate.
        te::EditItemID ownerID;
        juce::String paramID;

        bool operator<(const ParameterKey &other) const noexcept
        {
            if (ownerID != other.ownerID)
                return ownerID < other.ownerID;

            return paramID < other.paramID;
        }
    };

    static ParameterKey getKey(te::AutomatableParameter &parameter)
    {
        return {parameter.getOwnerID(), parameter.paramID};
    }

    static bool shouldGuard(te::AutomatableParameter *parameter)
    {
        if (parameter == nullptr)
            return false;

        auto &edit = parameter->getEdit();
        auto &transport = edit.getTransport();
        auto &automationRecordManager = edit.getAutomationRecordManager();

        return automationRecordManager.isWritingAutomation()
            && (transport.isPlaying() || transport.isRecording());
    }

    static juce::CriticalSection &getLock()
    {
        static juce::CriticalSection lock;
        return lock;
    }

    static std::set<ParameterKey> &getTouchedParameters()
    {
        static std::set<ParameterKey> touchedParameters;
        return touchedParameters;
    }
};

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug
{
enum class CommandType
{
    help,
    ping,
    systemState,
    transportState,
    stateDump,
    play,
    stop,
    screenshot,
    ensureTrack,
    selectTrack,
    ensureMidiClip,
    ensureMidiNote,
    setPluginParameter,
    quit,
    unknown
};

struct Command
{
    CommandType type{CommandType::unknown};
    juce::String rawLine;
    juce::String argument;
    juce::var arguments;
    juce::String parseError;
    bool jsonRequest{false};
};
} // namespace NextStudio::Debug

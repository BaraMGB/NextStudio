#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug
{
enum class CommandType
{
    help,
    ping,
    play,
    stop,
    screenshot,
    quit,
    unknown
};

struct Command
{
    CommandType type{CommandType::unknown};
    juce::String rawLine;
    juce::String argument;
};
} // namespace NextStudio::Debug

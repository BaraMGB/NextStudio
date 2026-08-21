#pragma once

#include "DebugCommand.h"

namespace NextStudio::Debug
{
/** Parses one legacy command line. All command execution happens later on the
    JUCE message thread. Empty input produces CommandType::unknown. */
Command parseCommandLine(const juce::String &line);

/** Returns true only for a JSON Lines debug-shell response object. */
bool isResponseLine(const juce::String &line);
} // namespace NextStudio::Debug

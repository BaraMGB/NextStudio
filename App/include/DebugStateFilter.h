#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::AgentDebug
{
inline constexpr int maximumStateStringLength = 120;

/** Filters binary-like data and bounds text included in agent state artifacts. */
juce::String sanitiseStateString(const juce::String &value);
} // namespace NextStudio::AgentDebug

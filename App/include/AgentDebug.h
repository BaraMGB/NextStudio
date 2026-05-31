#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace NextStudio::Debug
{
class DebugHost;
}

namespace NextStudio::AgentDebug
{
juce::var createStateDump(const NextStudio::Debug::DebugHost &debugHost);
juce::File writeStateDump(const NextStudio::Debug::DebugHost &debugHost, const juce::File &outputDirectory = {});
juce::File captureSnapshot(const NextStudio::Debug::DebugHost &debugHost, const juce::File &outputDirectory = {}, int maxWidth = 640);
} // namespace NextStudio::AgentDebug

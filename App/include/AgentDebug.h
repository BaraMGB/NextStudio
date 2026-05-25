#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

class MainComponent;

namespace NextStudio::AgentDebug
{
juce::var createStateDump(const MainComponent &mainComponent);
juce::String createStateDumpJson(const MainComponent &mainComponent);
juce::File writeStateDump(const MainComponent &mainComponent, const juce::File &outputDirectory = {});
juce::File captureSnapshot(const MainComponent &mainComponent, const juce::File &outputDirectory = {}, int maxWidth = 640);
bool executeCommand(MainComponent &mainComponent, const juce::String &commandName, const juce::String &argument = {});
} // namespace NextStudio::AgentDebug

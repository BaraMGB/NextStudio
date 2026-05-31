#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug::SessionEnvironment
{
juce::File createDebugSessionTempDirectory();
juce::File getDebugArtifactsDirectory(const juce::File &sessionTempDirectory);
} // namespace NextStudio::Debug::SessionEnvironment

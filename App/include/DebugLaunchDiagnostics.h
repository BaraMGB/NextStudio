#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug::LaunchDiagnostics
{
juce::File getDiagnosticsDirectory();
juce::File getDebugShellSingleInstanceRejectionFile();
bool recordDebugShellSingleInstanceRejection(const juce::String &commandLine, const juce::String &requestId);
} // namespace NextStudio::Debug::LaunchDiagnostics

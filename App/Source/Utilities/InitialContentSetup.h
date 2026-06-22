#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace InitialContentSetup
{
bool validateAndPrepareRoot(const juce::File &root, juce::String &errorMessage);
void populateBundledContent(const juce::File &root);
} // namespace InitialContentSetup

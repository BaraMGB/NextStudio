#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug
{
/** Writes and decodes a PNG before reporting success. Deletes partial output on failure. */
bool writeValidatedPng(const juce::Image &image, const juce::File &file);
} // namespace NextStudio::Debug

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"

namespace ThemeHelpers
{
juce::String getDefaultBuiltInThemeName();
juce::StringArray getBuiltInThemeNames();
bool applyBuiltInTheme(ApplicationViewState &appState, const juce::String &themeName);
void applyLookAndFeelColours(juce::LookAndFeel &lookAndFeel, ApplicationViewState &appState);
} // namespace ThemeHelpers

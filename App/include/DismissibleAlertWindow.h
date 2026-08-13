#pragma once

#include <JuceHeader.h>

class DismissibleAlertWindow final : public juce::AlertWindow
{
public:
    DismissibleAlertWindow(const juce::String &title, const juce::String &message)
        : juce::AlertWindow(title, message, juce::AlertWindow::NoIcon)
    {
    }

    void inputAttemptWhenModal() override { exitModalState(0); }
};

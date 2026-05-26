#pragma once

#include "DebugAppController.h"

namespace NextStudio::Debug
{
class DebugShell final : private juce::Timer
{
public:
    explicit DebugShell(MainComponent &mainComponent);
    ~DebugShell() override;

    void start();
    void stop();

private:
    void timerCallback() override;

    static Command parseCommand(const juce::String &line);
    void handleLine(const juce::String &line);
    void writeResponse(const Result &result);

    DebugAppController m_controller;
    juce::CriticalSection m_outputLock;
};
} // namespace NextStudio::Debug

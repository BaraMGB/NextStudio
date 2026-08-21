#pragma once

#include "DebugAppController.h"

#include <memory>
#include <thread>

namespace NextStudio::Debug
{
/** Owns stdin/stdout transport only. Input is read on a blocking worker and
    every command is dispatched to the JUCE message thread before execution. */
class DebugShell final
{
public:
    explicit DebugShell(DebugHost &debugHost);
    ~DebugShell();

    void start();
    void stop();

    // Internal transport hooks used by the platform stdin worker.
    struct InputState;
    void handleLine(const juce::String &line);
    void handleInputClosed();

private:
    void writeResponse(const Result &result);

    DebugAppController m_controller;
    juce::CriticalSection m_outputLock;
    std::shared_ptr<InputState> m_inputState;
    std::thread m_inputThread;
};
} // namespace NextStudio::Debug

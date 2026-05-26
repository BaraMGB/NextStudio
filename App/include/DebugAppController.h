#pragma once

#include "DebugCommand.h"
#include "DebugResult.h"

class MainComponent;

namespace NextStudio::Debug
{
class DebugAppController
{
public:
    explicit DebugAppController(MainComponent &mainComponent);

    Result execute(const Command &command);

private:
    Result handleHelp() const;
    Result handlePing() const;
    Result handlePlay() const;
    Result handleStop() const;
    Result handleScreenshot(const Command &command) const;
    Result handleQuit() const;

    MainComponent &m_mainComponent;
};
} // namespace NextStudio::Debug

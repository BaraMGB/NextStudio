#pragma once

#include "DebugCommand.h"
#include "DebugHost.h"
#include "DebugResult.h"

namespace NextStudio::Debug
{
class DebugAppController
{
public:
    explicit DebugAppController(DebugHost &debugHost);

    Result execute(const Command &command);

private:
    Result handleHelp() const;
    Result handlePing() const;
    Result handleSystemState() const;
    Result handleTransportState() const;
    Result handleStateDump(const Command &command) const;
    Result handlePlay() const;
    Result handleStop() const;
    Result handleScreenshot(const Command &command) const;
    Result handleQuit() const;

    DebugHost &m_debugHost;
};
} // namespace NextStudio::Debug

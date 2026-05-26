#include "DebugShell.h"

#include "MainComponent.h"
#include "Logging.h"

#include <iostream>

#if JUCE_LINUX || JUCE_MAC
#include <poll.h>
#include <unistd.h>
#endif

namespace NextStudio::Debug
{
namespace
{
void logAndStopShell(DebugShell &shell)
{
    NS_LOG_INFO(app, "debug shell input closed");
    shell.stop();
}
} // namespace

DebugShell::DebugShell(MainComponent &mainComponent)
    : m_controller(mainComponent)
{
}

DebugShell::~DebugShell()
{
    stop();
}

void DebugShell::start()
{
    startTimer(100);
    writeResponse(Result::success("ready", "debug shell started"));
}

void DebugShell::stop()
{
    stopTimer();
}

void DebugShell::timerCallback()
{
#if JUCE_LINUX || JUCE_MAC
    pollfd descriptor{};
    descriptor.fd = STDIN_FILENO;
    descriptor.events = POLLIN | POLLHUP | POLLERR;

    const auto pollResult = ::poll(&descriptor, 1, 0);
    if (pollResult <= 0)
        return;

    const bool hasInput = (descriptor.revents & POLLIN) != 0;
    const bool hasTerminalError = (descriptor.revents & POLLERR) != 0;
    const bool reachedEndOfInput = (descriptor.revents & POLLHUP) != 0;

    if (hasTerminalError)
    {
        logAndStopShell(*this);
        return;
    }

    if (reachedEndOfInput)
    {
        std::string rawLine;
        while (std::getline(std::cin, rawLine))
            handleLine(juce::String(rawLine).trim());

        logAndStopShell(*this);
        return;
    }

    if (!hasInput)
        return;
#else
    auto *input = std::cin.rdbuf();
    if (input == nullptr || input->in_avail() <= 0)
        return;
#endif

    std::string rawLine;
    if (!std::getline(std::cin, rawLine))
    {
        logAndStopShell(*this);
        return;
    }

    handleLine(juce::String(rawLine).trim());
}

Command DebugShell::parseCommand(const juce::String &line)
{
    Command command;
    command.rawLine = line;

    auto parts = juce::StringArray::fromTokens(line, true);
    parts.trim();
    parts.removeEmptyStrings();

    if (parts.isEmpty())
        return command;

    const auto name = parts[0].toLowerCase();
    if (parts.size() > 1)
        command.argument = parts[1];

    if (name == "help")
        command.type = CommandType::help;
    else if (name == "ping")
        command.type = CommandType::ping;
    else if (name == "screenshot")
        command.type = CommandType::screenshot;
    else if (name == "quit" || name == "exit")
        command.type = CommandType::quit;

    return command;
}

void DebugShell::handleLine(const juce::String &line)
{
    if (line.isEmpty())
        return;

    const auto command = parseCommand(line);
    const auto result = m_controller.execute(command);
    writeResponse(result);

    if (command.type == CommandType::quit)
        stop();
}

void DebugShell::writeResponse(const Result &result)
{
    const juce::ScopedLock lock(m_outputLock);
    std::cout << result.toResponseLine() << std::endl;
}
} // namespace NextStudio::Debug

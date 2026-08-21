#include "DebugShell.h"

#include "DebugProtocol.h"
#include "Logging.h"

#include <atomic>
#include <iostream>
#include <mutex>

#if JUCE_WINDOWS
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif

namespace NextStudio::Debug
{
struct DebugShell::InputState
{
    std::atomic<bool> active{true};
    std::mutex shellMutex;
    DebugShell *shell{nullptr};
};

namespace
{
void dispatchLine(const std::shared_ptr<DebugShell::InputState> &state, std::string rawLine)
{
    if (!rawLine.empty() && rawLine.back() == '\r')
        rawLine.pop_back();
    const auto line = juce::String::fromUTF8(rawLine.data(), static_cast<int>(rawLine.size())).trim();

    juce::MessageManager::callAsync(
        [state, line]
        {
            DebugShell *shell = nullptr;
            {
                std::scoped_lock lock(state->shellMutex);
                if (state->active.load(std::memory_order_acquire))
                    shell = state->shell;
            }
            if (shell != nullptr)
                shell->handleLine(line);
        });
}

void dispatchInputClosed(const std::shared_ptr<DebugShell::InputState> &state)
{
    juce::MessageManager::callAsync(
        [state]
        {
            DebugShell *shell = nullptr;
            {
                std::scoped_lock lock(state->shellMutex);
                if (state->active.load(std::memory_order_acquire))
                    shell = state->shell;
            }
            if (shell != nullptr)
                shell->handleInputClosed();
        });
}

void consumeBytes(const std::shared_ptr<DebugShell::InputState> &state,
                  std::string &pending,
                  const char *data,
                  size_t size)
{
    pending.append(data, size);
    while (true)
    {
        const auto newline = pending.find('\n');
        if (newline == std::string::npos)
            return;
        auto line = pending.substr(0, newline);
        pending.erase(0, newline + 1);
        dispatchLine(state, std::move(line));
    }
}

void readInput(const std::shared_ptr<DebugShell::InputState> &state)
{
    std::string pending;
    char buffer[4096];

#if JUCE_WINDOWS
    const auto input = ::GetStdHandle(STD_INPUT_HANDLE);
    if (input == nullptr || input == INVALID_HANDLE_VALUE)
    {
        dispatchInputClosed(state);
        return;
    }

    while (state->active.load(std::memory_order_acquire))
    {
        DWORD bytesRead = 0;
        const auto succeeded = ::ReadFile(input, buffer, static_cast<DWORD>(sizeof(buffer)), &bytesRead, nullptr);
        if (!succeeded)
        {
            const auto error = ::GetLastError();
            if (error != ERROR_OPERATION_ABORTED && error != ERROR_BROKEN_PIPE && error != ERROR_HANDLE_EOF)
                NS_LOG_WARN(app, "debug shell stdin ReadFile failed: " + juce::String(static_cast<int>(error)));
            break;
        }
        if (bytesRead == 0)
            break;
        consumeBytes(state, pending, buffer, static_cast<size_t>(bytesRead));
    }
#else
    while (state->active.load(std::memory_order_acquire))
    {
        pollfd descriptor{};
        descriptor.fd = STDIN_FILENO;
        descriptor.events = POLLIN | POLLHUP | POLLERR;
        const auto pollResult = ::poll(&descriptor, 1, 100);
        if (pollResult < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (pollResult == 0)
            continue;
        if ((descriptor.revents & POLLERR) != 0)
            break;

        const auto bytesRead = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytesRead <= 0)
            break;
        consumeBytes(state, pending, buffer, static_cast<size_t>(bytesRead));
    }
#endif

    if (!pending.empty())
        dispatchLine(state, std::move(pending));
    dispatchInputClosed(state);
}
} // namespace

DebugShell::DebugShell(DebugHost &debugHost)
    : m_controller(debugHost)
{
}

DebugShell::~DebugShell()
{
    stop();
}

void DebugShell::start()
{
    if (m_inputState != nullptr)
        return;

    m_inputState = std::make_shared<InputState>();
    m_inputState->shell = this;
    m_inputThread = std::thread(readInput, m_inputState);
    writeResponse(Result::success("ready", "debug shell started"));
}

void DebugShell::stop()
{
    const auto state = m_inputState;
    if (state == nullptr)
        return;

    state->active.store(false, std::memory_order_release);

#if JUCE_WINDOWS
    if (m_inputThread.joinable())
        ::CancelSynchronousIo(static_cast<HANDLE>(m_inputThread.native_handle()));
#endif

    if (m_inputThread.joinable())
        m_inputThread.join();

    {
        std::scoped_lock lock(state->shellMutex);
        state->shell = nullptr;
    }
    m_inputState.reset();
}

void DebugShell::handleLine(const juce::String &line)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (line.isEmpty())
        return;

    const auto command = parseCommandLine(line);
    const auto result = m_controller.execute(command);
    writeResponse(result);

    if (command.type == CommandType::quit)
        stop();
}

void DebugShell::handleInputClosed()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    NS_LOG_INFO(app, "debug shell input closed; requesting application quit");
    Command quitCommand;
    quitCommand.type = CommandType::quit;
    m_controller.execute(quitCommand);
    stop();
}

void DebugShell::writeResponse(const Result &result)
{
    const juce::ScopedLock lock(m_outputLock);
    std::cout << result.toResponseLine() << std::endl;
}
} // namespace NextStudio::Debug

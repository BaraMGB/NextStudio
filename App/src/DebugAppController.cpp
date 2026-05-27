#include "DebugAppController.h"

#include "MainComponent.h"
#include "Logging.h"
#include "Utilities.h"

namespace NextStudio::Debug
{
DebugAppController::DebugAppController(MainComponent &mainComponent)
    : m_mainComponent(mainComponent)
{
}

Result DebugAppController::execute(const Command &command)
{
    switch (command.type)
    {
    case CommandType::help:
        return handleHelp();
    case CommandType::ping:
        return handlePing();
    case CommandType::systemState:
        return handleSystemState();
    case CommandType::transportState:
        return handleTransportState();
    case CommandType::stateDump:
        return handleStateDump(command);
    case CommandType::play:
        return handlePlay();
    case CommandType::stop:
        return handleStop();
    case CommandType::screenshot:
        return handleScreenshot(command);
    case CommandType::quit:
        return handleQuit();
    case CommandType::unknown:
        break;
    }

    return Result::failure("unknown-command", "Unknown command. Try 'help'.");
}

Result DebugAppController::handleHelp() const
{
    auto result = Result::success();
    result.fields.set("commands", "help ping system-state transport-state state-dump play stop screenshot quit");
    return result;
}

Result DebugAppController::handlePing() const
{
    auto result = Result::success();
    result.fields.set("app", ProjectInfo::projectName);
    result.fields.set("version", ProjectInfo::versionString);
    result.fields.set("mode", "debug-shell");
    return result;
}

Result DebugAppController::handleSystemState() const
{
    const auto hasEdit = m_mainComponent.getCurrentEdit() != nullptr;
    const auto hasEditViewState = m_mainComponent.getEditViewState() != nullptr;
    const auto hasEditComponent = m_mainComponent.getEditComponent() != nullptr;
    const auto hasHeader = m_mainComponent.getHeaderComponent() != nullptr;
    const auto hasLowerRange = m_mainComponent.getLowerRangeComponent() != nullptr;
    const auto readyForPlayback = hasEdit && hasEditViewState && hasEditComponent;

    auto result = Result::success();
    result.fields.set("debugMode", m_mainComponent.isDebugMode() ? "true" : "false");
    result.fields.set("currentEditAvailable", hasEdit ? "true" : "false");
    result.fields.set("editViewStateAvailable", hasEditViewState ? "true" : "false");
    result.fields.set("editComponentAvailable", hasEditComponent ? "true" : "false");
    result.fields.set("headerComponentAvailable", hasHeader ? "true" : "false");
    result.fields.set("lowerRangeComponentAvailable", hasLowerRange ? "true" : "false");
    result.fields.set("readyForPlayback", readyForPlayback ? "true" : "false");

    if (auto *edit = m_mainComponent.getCurrentEdit())
    {
        auto &transport = edit->getTransport();
        result.fields.set("transportPlaying", transport.isPlaying() ? "true" : "false");
        result.fields.set("transportRecording", transport.isRecording() ? "true" : "false");
        result.fields.set("transportPositionSeconds", juce::String(transport.getPosition().inSeconds(), 3));
    }

    return result;
}

Result DebugAppController::handleTransportState() const
{
    if (auto *edit = m_mainComponent.getCurrentEdit())
    {
        auto &transport = edit->getTransport();
        auto result = Result::success();
        result.fields.set("playing", transport.isPlaying() ? "true" : "false");
        result.fields.set("recording", transport.isRecording() ? "true" : "false");
        result.fields.set("looping", static_cast<bool>(transport.looping) ? "true" : "false");
        result.fields.set("positionSeconds", juce::String(transport.getPosition().inSeconds(), 3));
        return result;
    }

    return Result::failure("not-ready", "Edit is unavailable");
}

Result DebugAppController::handleStateDump(const Command &command) const
{
    if (command.argument.isNotEmpty())
        return Result::failure("invalid-argument", "state-dump does not accept an argument");

    const auto file = m_mainComponent.writeAgentStateDump();
    if (file == juce::File() || !file.existsAsFile())
        return Result::failure("io-error", "Failed to write state dump");

    auto result = Result::success();
    result.fields.set("path", file.getFullPathName());
    return result;
}

Result DebugAppController::handlePlay() const
{
    if (auto *editComponent = m_mainComponent.getEditComponent())
    {
        EngineHelpers::play(editComponent->getEditViewState());

        auto result = Result::success();
        result.fields.set("playing", "true");
        return result;
    }

    return Result::failure("not-ready", "Edit component is unavailable");
}

Result DebugAppController::handleStop() const
{
    if (auto *editComponent = m_mainComponent.getEditComponent())
    {
        EngineHelpers::stopPlay(editComponent->getEditViewState());

        auto result = Result::success();
        result.fields.set("playing", "false");
        return result;
    }

    return Result::failure("not-ready", "Edit component is unavailable");
}

Result DebugAppController::handleScreenshot(const Command &command) const
{
    int maxWidth = 640;
    if (command.argument.isNotEmpty())
    {
        maxWidth = command.argument.getIntValue();
        if (maxWidth <= 0)
            return Result::failure("invalid-argument", "screenshot expects an optional positive maxWidth");
    }

    const auto file = m_mainComponent.captureAgentSnapshot(maxWidth);
    auto result = Result::success();
    result.fields.set("path", file.getFullPathName());
    return result;
}

Result DebugAppController::handleQuit() const
{
    if (auto *app = juce::JUCEApplication::getInstance())
    {
        NS_LOG_INFO(app, "debug shell requested quit");
        app->quit();

        auto result = Result::success();
        result.fields.set("quitting", "true");
        return result;
    }

    return Result::failure("no-app-instance", "No JUCE application instance");
}
} // namespace NextStudio::Debug

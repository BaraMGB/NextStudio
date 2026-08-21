#include "DebugProtocol.h"

namespace NextStudio::Debug
{
namespace
{
CommandType commandTypeForName(const juce::String &name)
{
    if (name == "help") return CommandType::help;
    if (name == "ping") return CommandType::ping;
    if (name == "system-state" || name == "system_state") return CommandType::systemState;
    if (name == "transport-state" || name == "transport_state") return CommandType::transportState;
    if (name == "state-dump" || name == "state_dump") return CommandType::stateDump;
    if (name == "play") return CommandType::play;
    if (name == "stop") return CommandType::stop;
    if (name == "screenshot") return CommandType::screenshot;
    if (name == "ensure-track") return CommandType::ensureTrack;
    if (name == "select-track") return CommandType::selectTrack;
    if (name == "ensure-midi-clip") return CommandType::ensureMidiClip;
    if (name == "ensure-midi-note") return CommandType::ensureMidiNote;
    if (name == "set-plugin-parameter") return CommandType::setPluginParameter;
    if (name == "quit" || name == "exit") return CommandType::quit;
    return CommandType::unknown;
}
} // namespace

Command parseCommandLine(const juce::String &line)
{
    Command command;
    command.rawLine = line;
    const auto trimmed = line.trim();
    if (trimmed.isEmpty())
        return command;

    if (trimmed.startsWithChar('{'))
    {
        command.jsonRequest = true;
        const auto request = juce::JSON::parse(trimmed);
        auto *object = request.getDynamicObject();
        if (object == nullptr)
        {
            command.parseError = "request must be a valid JSON object";
            return command;
        }

        const auto commandValue = object->getProperty("command");
        if (!commandValue.isString() || commandValue.toString().trim().isEmpty())
        {
            command.parseError = "request.command must be a non-empty string";
            return command;
        }

        command.type = commandTypeForName(commandValue.toString().trim().toLowerCase());
        command.arguments = object->getProperty("arguments");
        if (!command.arguments.isVoid() && command.arguments.getDynamicObject() == nullptr)
            command.parseError = "request.arguments must be an object when present";
        return command;
    }

    auto parts = juce::StringArray::fromTokens(trimmed, true);
    parts.trim();
    parts.removeEmptyStrings();
    if (parts.isEmpty())
        return command;

    if (parts.size() > 1)
    {
        const auto firstSpace = trimmed.indexOfAnyOf(" \t");
        if (firstSpace >= 0)
            command.argument = trimmed.substring(firstSpace).trim();
    }

    command.type = commandTypeForName(parts[0].toLowerCase());
    return command;
}

bool isResponseLine(const juce::String &line)
{
    const auto parsed = juce::JSON::parse(line);
    if (auto *object = parsed.getDynamicObject())
    {
        const auto status = object->getProperty("status").toString();
        return status == "ok" || status == "error";
    }

    return false;
}
} // namespace NextStudio::Debug

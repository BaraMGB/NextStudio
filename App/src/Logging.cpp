#include "Logging.h"

namespace NextStudio::Logging
{
namespace
{
constexpr bool isDebugLoggingEnabled()
{
#if defined(DEBUG) || defined(_DEBUG) || defined(DEBUG_OR_RELWITHDEBINFO)
    return true;
#else
    return false;
#endif
}

bool shouldLog(Level level)
{
    return level != Level::debug || isDebugLoggingEnabled();
}
} // namespace

const char *toString(Level level)
{
    switch (level)
    {
    case Level::debug:
        return "debug";
    case Level::info:
        return "info";
    case Level::warn:
        return "warn";
    case Level::error:
        return "error";
    }

    return "unknown";
}

const char *toString(Category category)
{
    switch (category)
    {
    case Category::app:
        return "app";
    case Category::ui:
        return "ui";
    case Category::viewstate:
        return "viewstate";
    case Category::selection:
        return "selection";
    case Category::workflow:
        return "workflow";
    case Category::project:
        return "project";
    case Category::edit:
        return "edit";
    case Category::plugins:
        return "plugins";
    case Category::filesystem:
        return "filesystem";
    case Category::autosave:
        return "autosave";
    case Category::transport:
        return "transport";
    case Category::engine:
        return "engine";
    case Category::setup:
        return "setup";
    }

    return "unknown";
}

void log(Level level, Category category, const juce::String &message, const char *file, int line)
{
    if (!shouldLog(level))
        return;

    juce::String logLine;
    logLine << juce::Time::getCurrentTime().toString(true, true, true, true)
            << " [" << toString(level) << "]"
            << " [" << toString(category) << "] "
            << message;

    if (file != nullptr && *file != '\0')
        logLine << " (" << juce::File::createFileWithoutCheckingPath(file).getFileName() << ':' << line << ')';

    juce::Logger::writeToLog(logLine);
}
} // namespace NextStudio::Logging

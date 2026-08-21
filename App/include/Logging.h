#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Logging
{
enum class Level
{
    debug,
    info,
    warn,
    error
};

enum class Category
{
    app,
    ui,
    viewstate,
    selection,
    workflow,
    project,
    edit,
    plugins,
    filesystem,
    autosave,
    transport,
    engine,
    setup
};

void log(Level level, Category category, const juce::String &message, const char *file = nullptr, int line = 0);

const char *toString(Level level);
const char *toString(Category category);

inline juce::String toLogString(const juce::String &value) { return value; }
inline juce::String toLogString(const char *value) { return value != nullptr ? juce::String(value) : juce::String("<null>"); }
inline juce::String toLogString(const std::string &value) { return juce::String(value); }
inline juce::String toLogString(bool value) { return value ? "true" : "false"; }

template <typename T>
inline juce::String toLogString(const T &value)
{
    return juce::String(value);
}
} // namespace NextStudio::Logging

#define NS_LOG_DEBUG(category, message) NextStudio::Logging::log(NextStudio::Logging::Level::debug, NextStudio::Logging::Category::category, (message), __FILE__, __LINE__)
#define NS_LOG_INFO(category, message) NextStudio::Logging::log(NextStudio::Logging::Level::info, NextStudio::Logging::Category::category, (message), __FILE__, __LINE__)
#define NS_LOG_WARN(category, message) NextStudio::Logging::log(NextStudio::Logging::Level::warn, NextStudio::Logging::Category::category, (message), __FILE__, __LINE__)
#define NS_LOG_ERROR(category, message) NextStudio::Logging::log(NextStudio::Logging::Level::error, NextStudio::Logging::Category::category, (message), __FILE__, __LINE__)

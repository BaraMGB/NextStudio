#include "DebugLaunchDiagnostics.h"

#include "Logging.h"

namespace NextStudio::Debug::LaunchDiagnostics
{
juce::File getDiagnosticsDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile(ProjectInfo::projectName)
                   .getChildFile("debug")
                   .getChildFile("launch-rejections");
    dir.createDirectory();
    return dir;
}

juce::File getDebugShellSingleInstanceRejectionFile()
{
    return getDiagnosticsDirectory().getChildFile("debug-shell-last-rejection.json");
}

bool recordDebugShellSingleInstanceRejection(const juce::String &commandLine, const juce::String &requestId)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("reason", "single-instance-conflict");
    object->setProperty("application", ProjectInfo::projectName);
    object->setProperty("version", ProjectInfo::versionString);
    object->setProperty("commandLine", commandLine);
    object->setProperty("requestId", requestId);
    object->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    object->setProperty("unixMs", juce::var(static_cast<juce::int64>(juce::Time::currentTimeMillis())));

    const auto file = getDebugShellSingleInstanceRejectionFile();
    if (!file.replaceWithText(juce::JSON::toString(juce::var(object), true)))
    {
        NS_LOG_ERROR(app, "failed to write debug-shell single-instance rejection marker: " + file.getFullPathName());
        return false;
    }

    NS_LOG_WARN(app, "debug-shell single-instance rejection marker written: " + file.getFullPathName());
    return true;
}
} // namespace NextStudio::Debug::LaunchDiagnostics

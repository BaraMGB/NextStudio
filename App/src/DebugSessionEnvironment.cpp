#include "DebugSessionEnvironment.h"

namespace NextStudio::Debug::SessionEnvironment
{
juce::File createDebugSessionTempDirectory()
{
    auto baseDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile(ProjectInfo::projectName)
                       .getChildFile("debug-shell");
    if (baseDir.createDirectory().failed())
        return {};

    const auto sessionName = "session-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S")
                             + "-" + juce::String(juce::Random::getSystemRandom().nextInt(1000000));
    auto sessionDir = baseDir.getChildFile(sessionName);
    if (sessionDir.createDirectory().failed() || !sessionDir.isDirectory())
        return {};
    return sessionDir;
}

juce::File getDebugArtifactsDirectory(const juce::File &sessionTempDirectory)
{
    if (sessionTempDirectory == juce::File())
        return {};

    auto agentDir = sessionTempDirectory.getChildFile("agent-debug");
    if (agentDir.createDirectory().failed() || !agentDir.isDirectory())
        return {};
    return agentDir;
}
} // namespace NextStudio::Debug::SessionEnvironment

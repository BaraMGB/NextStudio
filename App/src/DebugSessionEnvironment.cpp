#include "DebugSessionEnvironment.h"

namespace NextStudio::Debug::SessionEnvironment
{
juce::File createDebugSessionTempDirectory()
{
    auto baseDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile(ProjectInfo::projectName)
                       .getChildFile("debug-shell");
    baseDir.createDirectory();

    const auto sessionName = "session-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S")
                             + "-" + juce::String(juce::Random::getSystemRandom().nextInt(1000000));
    auto sessionDir = baseDir.getChildFile(sessionName);
    sessionDir.createDirectory();
    return sessionDir;
}

juce::File getDebugArtifactsDirectory(const juce::File &sessionTempDirectory)
{
    auto agentDir = sessionTempDirectory.getChildFile("agent-debug");
    agentDir.createDirectory();
    return agentDir;
}
} // namespace NextStudio::Debug::SessionEnvironment

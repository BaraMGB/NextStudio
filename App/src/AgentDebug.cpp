#include "AgentDebug.h"

#include "DebugHost.h"
#include "Logging.h"

namespace te = tracktion_engine;

namespace NextStudio::AgentDebug
{
namespace
{
constexpr int maxStringLength = 120;

juce::String lowerRangeViewToString(LowerRangeView view)
{
    switch (view)
    {
    case LowerRangeView::none:
        return "none";
    case LowerRangeView::midiEditor:
        return "midiEditor";
    case LowerRangeView::pluginRack:
        return "pluginRack";
    case LowerRangeView::mixer:
        return "mixer";
    }

    return "unknown";
}

juce::String trackTypeToString(const te::Track &track)
{
    if (track.isMasterTrack())
        return "master";
    if (track.isFolderTrack())
        return "folder";
    if (track.isAudioTrack())
        return (bool) track.state.getProperty(IDs::isMidiTrack) ? "midi" : "audio";
    return "other";
}

juce::String clipTypeToString(const te::Clip &clip)
{
    if (dynamic_cast<const te::MidiClip *>(&clip) != nullptr)
        return "midi";
    if (dynamic_cast<const te::WaveAudioClip *>(&clip) != nullptr)
        return "audio";
    return "other";
}

juce::String sanitiseString(const juce::String &value)
{
    if (value.isEmpty())
        return {};

    bool containsBinaryLikeData = false;
    for (auto character : value)
    {
        if ((character < 32 && character != '\n' && character != '\r' && character != '\t') || character == 0xfffd)
        {
            containsBinaryLikeData = true;
            break;
        }
    }

    if (containsBinaryLikeData)
        return "<filtered-binary-data>";

    if (value.length() > maxStringLength)
        return value.substring(0, maxStringLength) + "…<truncated>";

    return value;
}

juce::var buildPluginSummary(const te::Plugin &plugin)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("name", sanitiseString(plugin.getName()));
    object->setProperty("type", sanitiseString(const_cast<te::Plugin &>(plugin).getPluginType()));
    object->setProperty("enabled", plugin.isEnabled());
    object->setProperty("childStateCount", plugin.state.getNumChildren());
    object->setProperty("propertyCount", plugin.state.getNumProperties());
    return object;
}

juce::var buildTrackSummary(const te::Track &track, const te::SelectionManager &selectionManager)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("id", track.itemID.toString());
    object->setProperty("name", sanitiseString(track.getName()));
    object->setProperty("type", trackTypeToString(track));
    object->setProperty("selected", selectionManager.isSelected(const_cast<te::Track *>(&track)));
    object->setProperty("colour", track.getColour().toDisplayString(true));
    object->setProperty("pluginCount", track.pluginList.getPlugins().size());
    object->setProperty("showLowerRange", (bool) track.state.getProperty(IDs::showLowerRange, false));

    juce::Array<juce::var> plugins;
    for (auto *plugin : track.pluginList.getPlugins())
        if (plugin != nullptr)
            plugins.add(buildPluginSummary(*plugin));
    object->setProperty("plugins", plugins);

    if (auto *audioTrack = dynamic_cast<const te::AudioTrack *>(&track))
        object->setProperty("clipCount", audioTrack->getClips().size());

    return object;
}

juce::var buildSelectionSummary(const EditViewState &evs)
{
    auto *object = new juce::DynamicObject();
    auto selectedTracks = evs.m_selectionManager.getItemsOfType<te::Track>();
    auto selectedClips = evs.m_selectionManager.getItemsOfType<te::Clip>();

    object->setProperty("selectedTrackCount", selectedTracks.size());
    object->setProperty("selectedClipCount", selectedClips.size());

    juce::Array<juce::var> trackNames;
    for (auto *track : selectedTracks)
        if (track != nullptr)
            trackNames.add(sanitiseString(track->getName()));
    object->setProperty("selectedTracks", trackNames);

    juce::Array<juce::var> clipSummaries;
    for (auto *clip : selectedClips)
    {
        if (clip == nullptr)
            continue;

        auto *clipObject = new juce::DynamicObject();
        clipObject->setProperty("name", sanitiseString(clip->getName()));
        clipObject->setProperty("type", clipTypeToString(*clip));
        clipObject->setProperty("track", clip->getTrack() != nullptr ? sanitiseString(clip->getTrack()->getName()) : juce::String());
        clipObject->setProperty("startSeconds", clip->getPosition().getStart().inSeconds());
        clipObject->setProperty("lengthSeconds", clip->getPosition().getLength().inSeconds());
        clipSummaries.add(clipObject);
    }
    object->setProperty("selectedClips", clipSummaries);

    return object;
}

juce::File getOutputDirectory(const NextStudio::Debug::DebugHost &debugHost, const juce::File &requestedDirectory)
{
    auto directory = requestedDirectory;
    if (directory == juce::File())
        directory = debugHost.getDebugArtifactsDirectory();

    directory.createDirectory();
    return directory;
}

juce::String createTimestampToken()
{
    return juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
}
} // namespace

juce::var createStateDump(const NextStudio::Debug::DebugHost &debugHost)
{
    auto *root = new juce::DynamicObject();
    root->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    root->setProperty("application", ProjectInfo::projectName);
    root->setProperty("version", ProjectInfo::versionString);

    auto *window = new juce::DynamicObject();
    const auto bounds = debugHost.getScreenBounds();
    window->setProperty("x", bounds.getX());
    window->setProperty("y", bounds.getY());
    window->setProperty("width", bounds.getWidth());
    window->setProperty("height", bounds.getHeight());
    root->setProperty("window", window);

    const auto &appState = debugHost.getApplicationState();
    auto *ui = new juce::DynamicObject();
    ui->setProperty("sidebarWidth", (int) appState.m_sidebarWidth);
    ui->setProperty("sidebarCollapsed", (bool) appState.m_sidebarCollapsed);
    ui->setProperty("setupComplete", (bool) appState.m_setupComplete);
    ui->setProperty("workDir", sanitiseString(appState.m_workDir.get()));
    root->setProperty("ui", ui);

    if (auto *evs = debugHost.getEditViewState())
    {
        auto *edit = new juce::DynamicObject();
        const auto editFile = te::EditFileOperations(evs->m_edit).getEditFile();
        edit->setProperty("file", sanitiseString(editFile.getFullPathName()));
        edit->setProperty("trackCount", te::getAllTracks(evs->m_edit).size());
        edit->setProperty("audioTrackCount", te::getAudioTracks(evs->m_edit).size());
        edit->setProperty("lowerRangeView", lowerRangeViewToString(evs->getLowerRangeView()));
        edit->setProperty("needsAutosave", (bool) evs->m_needAutoSave);
        edit->setProperty("selection", buildSelectionSummary(*evs));

        auto &transport = evs->m_edit.getTransport();
        auto *transportState = new juce::DynamicObject();
        transportState->setProperty("playing", transport.isPlaying());
        transportState->setProperty("recording", transport.isRecording());
        transportState->setProperty("looping", (bool) transport.looping);
        transportState->setProperty("positionSeconds", transport.getPosition().inSeconds());
        edit->setProperty("transport", transportState);

        juce::Array<juce::var> tracks;
        for (auto *track : te::getAllTracks(evs->m_edit))
            if (track != nullptr)
                tracks.add(buildTrackSummary(*track, evs->m_selectionManager));
        edit->setProperty("tracks", tracks);

        root->setProperty("edit", edit);
    }

    return root;
}

juce::File writeStateDump(const NextStudio::Debug::DebugHost &debugHost, const juce::File &outputDirectory)
{
    const auto directory = getOutputDirectory(debugHost, outputDirectory);
    const auto file = directory.getNonexistentChildFile("state-dump-" + createTimestampToken(), ".json", false);
    if (!file.replaceWithText(juce::JSON::toString(createStateDump(debugHost), true)))
    {
        NS_LOG_ERROR(app, "failed to write agent state dump: " + file.getFullPathName());
        return {};
    }

    NS_LOG_INFO(app, "agent state dump written: " + file.getFullPathName());
    return file;
}

juce::File captureSnapshot(const NextStudio::Debug::DebugHost &debugHost, const juce::File &outputDirectory, int maxWidth)
{
    const auto directory = getOutputDirectory(debugHost, outputDirectory);
    const auto bounds = debugHost.getLocalBounds();
    const auto width = juce::jmax(1, bounds.getWidth());
    const auto scale = maxWidth > 0 ? juce::jmin(1.0f, (float) maxWidth / (float) width) : 1.0f;
    const auto image = debugHost.createSnapshot(bounds, scale);

    const auto file = directory.getNonexistentChildFile("ui-snapshot-" + createTimestampToken(), ".png", false);
    juce::PNGImageFormat png;
    if (auto stream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream()))
        png.writeImageToStream(image, *stream);

    NS_LOG_INFO(app, "agent UI snapshot written: " + file.getFullPathName());
    return file;
}

} // namespace NextStudio::AgentDebug

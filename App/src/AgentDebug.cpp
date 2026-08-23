#include "AgentDebug.h"

#include "DebugHost.h"
#include "DebugSnapshotWriter.h"
#include "DebugStateFilter.h"
#include "Logging.h"

namespace te = tracktion_engine;

namespace NextStudio::AgentDebug
{
namespace
{
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

juce::var buildPluginSummary(const te::Plugin &plugin)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("id", plugin.itemID.toString());
    object->setProperty("name", sanitiseStateString(plugin.getName()));
    object->setProperty("type", sanitiseStateString(const_cast<te::Plugin &>(plugin).getPluginType()));
    object->setProperty("enabled", plugin.isEnabled());
    object->setProperty("childStateCount", plugin.state.getNumChildren());
    object->setProperty("propertyCount", plugin.state.getNumProperties());

    juce::Array<juce::var> parameters;
    for (auto parameter : const_cast<te::Plugin &>(plugin).getAutomatableParameters())
    {
        if (parameter == nullptr)
            continue;
        auto *parameterObject = new juce::DynamicObject();
        parameterObject->setProperty("id", sanitiseStateString(parameter->paramID));
        parameterObject->setProperty("name", sanitiseStateString(parameter->getParameterName()));
        parameterObject->setProperty("value", parameter->getCurrentValue());
        parameterObject->setProperty("normalisedValue", parameter->getCurrentNormalisedValue());
        parameters.add(parameterObject);
    }
    object->setProperty("parameters", parameters);
    return object;
}

juce::var buildClipSummary(const te::Clip &clip)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("id", clip.itemID.toString());
    object->setProperty("name", sanitiseStateString(clip.getName()));
    object->setProperty("type", clipTypeToString(clip));
    object->setProperty("startSeconds", clip.getPosition().getStart().inSeconds());
    object->setProperty("lengthSeconds", clip.getPosition().getLength().inSeconds());

    if (auto *midiClip = dynamic_cast<const te::MidiClip *>(&clip))
    {
        juce::Array<juce::var> notes;
        for (auto *note : midiClip->getSequence().getNotes())
        {
            if (note == nullptr)
                continue;
            auto *noteObject = new juce::DynamicObject();
            noteObject->setProperty("key", juce::String(note->getNoteNumber()) + "@" + juce::String(note->getStartBeat().inBeats(), 6));
            noteObject->setProperty("noteNumber", note->getNoteNumber());
            noteObject->setProperty("startBeats", note->getStartBeat().inBeats());
            noteObject->setProperty("lengthBeats", note->getLengthBeats().inBeats());
            noteObject->setProperty("velocity", note->getVelocity());
            notes.add(noteObject);
        }
        object->setProperty("notes", notes);
        object->setProperty("noteCount", notes.size());
    }

    return object;
}

juce::var buildTrackSummary(const te::Track &track, const te::SelectionManager &selectionManager)
{
    auto *object = new juce::DynamicObject();
    object->setProperty("id", track.itemID.toString());
    object->setProperty("name", sanitiseStateString(track.getName()));
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
    {
        juce::Array<juce::var> clips;
        for (auto *clip : audioTrack->getClips())
            if (clip != nullptr)
                clips.add(buildClipSummary(*clip));
        object->setProperty("clipCount", clips.size());
        object->setProperty("clips", clips);
    }

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
            trackNames.add(sanitiseStateString(track->getName()));
    object->setProperty("selectedTracks", trackNames);

    juce::Array<juce::var> clipSummaries;
    for (auto *clip : selectedClips)
    {
        if (clip == nullptr)
            continue;

        auto *clipObject = new juce::DynamicObject();
        clipObject->setProperty("name", sanitiseStateString(clip->getName()));
        clipObject->setProperty("type", clipTypeToString(*clip));
        clipObject->setProperty("track", clip->getTrack() != nullptr ? sanitiseStateString(clip->getTrack()->getName()) : juce::String());
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

    if (directory == juce::File() || directory.createDirectory().failed() || !directory.isDirectory())
    {
        NS_LOG_ERROR(filesystem, "failed to create agent debug directory: " + directory.getFullPathName());
        return {};
    }

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
    ui->setProperty("lowerRangeCollapsed", (bool) appState.m_lowerRangeCollapsed);
    ui->setProperty("setupComplete", (bool) appState.m_setupComplete);
    ui->setProperty("workDir", sanitiseStateString(appState.m_workDir.get()));
    root->setProperty("ui", ui);

    if (auto *evs = debugHost.getEditViewState())
    {
        auto *edit = new juce::DynamicObject();
        const auto editFile = te::EditFileOperations(evs->m_edit).getEditFile();
        edit->setProperty("file", sanitiseStateString(editFile.getFullPathName()));
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
    if (directory == juce::File())
        return {};

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
    if (directory == juce::File() || bounds.isEmpty() || maxWidth <= 0)
    {
        NS_LOG_ERROR(app, "agent UI snapshot rejected invalid output directory, bounds, or width");
        return {};
    }

    const auto scale = juce::jmin(1.0f, static_cast<float>(maxWidth) / static_cast<float>(bounds.getWidth()));
    const auto image = debugHost.createSnapshot(bounds, scale);
    if (!image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
    {
        NS_LOG_ERROR(app, "agent UI snapshot capture returned an invalid image");
        return {};
    }

    const auto file = directory.getNonexistentChildFile("ui-snapshot-" + createTimestampToken(), ".png", false);
    if (!NextStudio::Debug::writeValidatedPng(image, file))
    {
        NS_LOG_ERROR(filesystem, "failed to write a valid agent UI snapshot: " + file.getFullPathName());
        return {};
    }

    NS_LOG_INFO(app, "agent UI snapshot written: " + file.getFullPathName());
    return file;
}

} // namespace NextStudio::AgentDebug

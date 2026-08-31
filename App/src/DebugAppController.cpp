#include "DebugAppController.h"

#include "Logging.h"

#include <cmath>
#include <limits>

namespace te = tracktion_engine;

namespace NextStudio::Debug
{
namespace
{
const juce::DynamicObject *getArguments(const Command &command)
{
    return command.arguments.getDynamicObject();
}

bool readRequiredString(const juce::DynamicObject &arguments, const juce::Identifier &key, juce::String &value)
{
    const auto raw = arguments.getProperty(key);
    if (!raw.isString())
        return false;
    value = raw.toString().trim();
    return value.isNotEmpty() && value.length() <= 200 && !value.containsAnyOf("\r\n");
}

bool readRequiredDouble(const juce::DynamicObject &arguments, const juce::Identifier &key, double &value)
{
    const auto raw = arguments.getProperty(key);
    if (!raw.isInt() && !raw.isInt64() && !raw.isDouble())
        return false;
    value = static_cast<double>(raw);
    return std::isfinite(value);
}

bool readRequiredInt(const juce::DynamicObject &arguments, const juce::Identifier &key, int &value)
{
    const auto raw = arguments.getProperty(key);
    if (!raw.isInt() && !raw.isInt64())
        return false;
    const auto candidate = static_cast<juce::int64>(raw);
    if (candidate < std::numeric_limits<int>::min() || candidate > std::numeric_limits<int>::max())
        return false;
    value = static_cast<int>(candidate);
    return true;
}

bool isMidiTrack(const te::Track &track)
{
    return track.isAudioTrack() && static_cast<bool>(track.state.getProperty(IDs::isMidiTrack, false));
}
} // namespace

DebugAppController::DebugAppController(DebugHost &debugHost)
    : m_debugHost(debugHost)
{
}

Result DebugAppController::execute(const Command &command)
{
    if (command.parseError.isNotEmpty())
        return Result::failure("invalid-request", command.parseError);

    const auto acceptsLegacyArgument = command.type == CommandType::screenshot;
    const auto acceptsJsonArguments = command.type == CommandType::ensureTrack
                                      || command.type == CommandType::selectTrack
                                      || command.type == CommandType::ensureMidiClip
                                      || command.type == CommandType::ensureMidiNote
                                      || command.type == CommandType::setPluginParameter;
    if (command.argument.isNotEmpty() && !acceptsLegacyArgument)
        return Result::failure("invalid-argument", "command does not accept a legacy argument");
    if (command.jsonRequest && command.arguments.getDynamicObject() != nullptr
        && command.arguments.getDynamicObject()->getProperties().size() > 0 && !acceptsJsonArguments)
        return Result::failure("invalid-argument", "command does not accept JSON arguments");

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
    case CommandType::projectSaveAs:
        return handleProjectSaveAs();
    case CommandType::ensureTrack:
        return handleEnsureTrack(command);
    case CommandType::selectTrack:
        return handleSelectTrack(command);
    case CommandType::ensureMidiClip:
        return handleEnsureMidiClip(command);
    case CommandType::ensureMidiNote:
        return handleEnsureMidiNote(command);
    case CommandType::setPluginParameter:
        return handleSetPluginParameter(command);
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
    result.fields.set("commands", "help ping system-state transport-state state-dump play stop screenshot project-save-as ensure-track select-track ensure-midi-clip ensure-midi-note set-plugin-parameter quit");
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
    const auto hasEdit = m_debugHost.getCurrentEdit() != nullptr;
    const auto hasEditViewState = m_debugHost.getEditViewState() != nullptr;
    const auto hasEditComponent = m_debugHost.hasEditComponent();
    const auto hasHeader = m_debugHost.hasHeaderComponent();
    const auto hasLowerRange = m_debugHost.hasLowerRangeComponent();
    const auto readyForPlayback = hasEdit && hasEditViewState && hasEditComponent;

    auto result = Result::success();
    result.fields.set("debugMode", m_debugHost.isDebugMode() ? "true" : "false");
    result.fields.set("settingsPath", m_debugHost.getApplicationState().getSettingsFile().getFullPathName());
    result.fields.set("debugArtifactsPath", m_debugHost.getDebugArtifactsDirectory().getFullPathName());
    result.fields.set("currentEditAvailable", hasEdit ? "true" : "false");
    result.fields.set("editViewStateAvailable", hasEditViewState ? "true" : "false");
    result.fields.set("editComponentAvailable", hasEditComponent ? "true" : "false");
    result.fields.set("headerComponentAvailable", hasHeader ? "true" : "false");
    result.fields.set("lowerRangeComponentAvailable", hasLowerRange ? "true" : "false");
    result.fields.set("readyForPlayback", readyForPlayback ? "true" : "false");

    if (auto *edit = m_debugHost.getCurrentEdit())
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
    if (auto *edit = m_debugHost.getCurrentEdit())
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

    const auto file = m_debugHost.writeStateDump();
    if (file == juce::File() || !file.existsAsFile())
        return Result::failure("io-error", "Failed to write state dump");

    auto result = Result::success();
    result.fields.set("path", file.getFullPathName());
    return result;
}

Result DebugAppController::handlePlay() const
{
    if (!m_debugHost.play())
        return Result::failure("not-ready", "Edit component is unavailable");

    auto result = Result::success();
    result.fields.set("playing", "true");
    return result;
}

Result DebugAppController::handleStop() const
{
    if (!m_debugHost.stop())
        return Result::failure("not-ready", "Edit component is unavailable");

    auto result = Result::success();
    result.fields.set("playing", "false");
    return result;
}

Result DebugAppController::handleScreenshot(const Command &command) const
{
    constexpr int maximumSnapshotWidth = 8192;
    int maxWidth = 640;
    if (command.argument.isNotEmpty())
    {
        if (!command.argument.containsOnly("0123456789"))
            return Result::failure("invalid-argument", "screenshot expects an optional integer maxWidth from 1 to 8192");

        const auto parsedWidth = command.argument.getLargeIntValue();
        if (parsedWidth <= 0 || parsedWidth > maximumSnapshotWidth)
            return Result::failure("invalid-argument", "screenshot expects an optional integer maxWidth from 1 to 8192");
        maxWidth = static_cast<int>(parsedWidth);
    }

    const auto file = m_debugHost.captureSnapshot(maxWidth);
    if (file == juce::File() || !file.existsAsFile() || file.getSize() <= 0)
        return Result::failure("io-error", "Failed to capture a valid PNG screenshot");

    auto result = Result::success();
    result.fields.set("path", file.getFullPathName());
    return result;
}

Result DebugAppController::handleProjectSaveAs() const
{
    if (!m_debugHost.showProjectSaveAs())
        return Result::failure("not-ready", "Project browser is unavailable");

    auto result = Result::success();
    result.fields.set("projectBrowserMode", "saveProjectAs");
    return result;
}

Result DebugAppController::handleEnsureTrack(const Command &command) const
{
    const auto *arguments = getArguments(command);
    juce::String type;
    juce::String name;
    if (arguments == nullptr || !readRequiredString(*arguments, "type", type)
        || !readRequiredString(*arguments, "name", name))
        return Result::failure("invalid-argument", "ensure-track requires string arguments type and name");

    type = type.toLowerCase();
    if (type != "midi" && type != "audio")
        return Result::failure("invalid-argument", "ensure-track type must be midi or audio");

    auto *edit = m_debugHost.getCurrentEdit();
    auto *viewState = m_debugHost.getEditViewState();
    if (edit == nullptr || viewState == nullptr)
        return Result::failure("not-ready", "Edit view state is unavailable");

    const auto wantsMidi = type == "midi";
    for (auto *track : te::getAudioTracks(*edit))
    {
        if (track != nullptr && track->getName() == name && isMidiTrack(*track) == wantsMidi)
        {
            auto result = Result::success();
            result.fields.set("trackId", track->itemID.toString());
            result.fields.set("name", track->getName());
            result.fields.set("type", type);
            result.fields.set("created", "false");
            return result;
        }
    }

    auto *track = m_debugHost.createAudioTrack(wantsMidi, name);
    if (track == nullptr)
        return Result::failure("edit-error", "Failed to create track");
    auto result = Result::success();
    result.fields.set("trackId", track->itemID.toString());
    result.fields.set("name", track->getName());
    result.fields.set("type", type);
    result.fields.set("created", "true");
    return result;
}

Result DebugAppController::handleSelectTrack(const Command &command) const
{
    const auto *arguments = getArguments(command);
    juce::String trackId;
    if (arguments == nullptr || !readRequiredString(*arguments, "trackId", trackId))
        return Result::failure("invalid-argument", "select-track requires string argument trackId");

    auto *edit = m_debugHost.getCurrentEdit();
    auto *viewState = m_debugHost.getEditViewState();
    if (edit == nullptr || viewState == nullptr)
        return Result::failure("not-ready", "Edit view state is unavailable");

    auto *track = te::findTrackForID(*edit, te::EditItemID::fromVar(trackId));
    if (track == nullptr)
        return Result::failure("not-found", "Track was not found");

    viewState->m_selectionManager.selectOnly(track);
    auto result = Result::success();
    result.fields.set("trackId", track->itemID.toString());
    result.fields.set("name", track->getName());
    result.fields.set("selected", "true");
    return result;
}

Result DebugAppController::handleEnsureMidiClip(const Command &command) const
{
    const auto *arguments = getArguments(command);
    juce::String trackId;
    juce::String name;
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;
    if (arguments == nullptr || !readRequiredString(*arguments, "trackId", trackId)
        || !readRequiredString(*arguments, "name", name)
        || !readRequiredDouble(*arguments, "startSeconds", startSeconds)
        || !readRequiredDouble(*arguments, "lengthSeconds", lengthSeconds)
        || startSeconds < 0.0 || lengthSeconds <= 0.0 || startSeconds + lengthSeconds > 86400.0)
        return Result::failure("invalid-argument", "ensure-midi-clip requires trackId, name, startSeconds >= 0, and lengthSeconds > 0");

    auto *edit = m_debugHost.getCurrentEdit();
    if (edit == nullptr)
        return Result::failure("not-ready", "Edit is unavailable");

    auto *track = dynamic_cast<te::AudioTrack *>(te::findTrackForID(*edit, te::EditItemID::fromVar(trackId)));
    if (track == nullptr)
        return Result::failure("not-found", "Track was not found");
    if (!isMidiTrack(*track))
        return Result::failure("wrong-type", "Track is not a MIDI track");

    for (auto *clip : track->getClips())
    {
        auto *midiClip = dynamic_cast<te::MidiClip *>(clip);
        if (midiClip != nullptr && midiClip->getName() == name
            && std::abs(midiClip->getPosition().getStart().inSeconds() - startSeconds) < 1.0e-7
            && std::abs(midiClip->getPosition().getLength().inSeconds() - lengthSeconds) < 1.0e-7)
        {
            auto result = Result::success();
            result.fields.set("clipId", midiClip->itemID.toString());
            result.fields.set("trackId", track->itemID.toString());
            result.fields.set("created", "false");
            return result;
        }
    }

    auto clip = track->insertMIDIClip(name,
                                      {tracktion::TimePosition::fromSeconds(startSeconds), tracktion::TimePosition::fromSeconds(startSeconds + lengthSeconds)},
                                      nullptr);
    if (clip == nullptr)
        return Result::failure("edit-error", "Failed to create MIDI clip");

    auto result = Result::success();
    result.fields.set("clipId", clip->itemID.toString());
    result.fields.set("trackId", track->itemID.toString());
    result.fields.set("created", "true");
    return result;
}

Result DebugAppController::handleEnsureMidiNote(const Command &command) const
{
    const auto *arguments = getArguments(command);
    juce::String clipId;
    int noteNumber = 0;
    int velocity = 0;
    double startBeats = 0.0;
    double lengthBeats = 0.0;
    if (arguments == nullptr || !readRequiredString(*arguments, "clipId", clipId)
        || !readRequiredInt(*arguments, "noteNumber", noteNumber)
        || !readRequiredInt(*arguments, "velocity", velocity)
        || !readRequiredDouble(*arguments, "startBeats", startBeats)
        || !readRequiredDouble(*arguments, "lengthBeats", lengthBeats)
        || noteNumber < 0 || noteNumber > 127 || velocity < 1 || velocity > 127
        || startBeats < 0.0 || lengthBeats <= 0.0)
        return Result::failure("invalid-argument", "ensure-midi-note requires clipId, noteNumber 0..127, velocity 1..127, startBeats >= 0, and lengthBeats > 0");

    auto *edit = m_debugHost.getCurrentEdit();
    if (edit == nullptr)
        return Result::failure("not-ready", "Edit is unavailable");

    auto *clip = dynamic_cast<te::MidiClip *>(te::findClipForID(*edit, te::EditItemID::fromVar(clipId)));
    if (clip == nullptr)
        return Result::failure("not-found", "MIDI clip was not found");
    if (startBeats + lengthBeats > clip->getLengthInBeats().inBeats() + 1.0e-7)
        return Result::failure("invalid-argument", "MIDI note must fit inside the clip");

    for (auto *note : clip->getSequence().getNotes())
    {
        if (note != nullptr && note->getNoteNumber() == noteNumber
            && std::abs(note->getStartBeat().inBeats() - startBeats) < 1.0e-7
            && std::abs(note->getLengthBeats().inBeats() - lengthBeats) < 1.0e-7)
        {
            note->setVelocity(velocity, &edit->getUndoManager());
            auto result = Result::success();
            result.fields.set("clipId", clip->itemID.toString());
            result.fields.set("noteKey", juce::String(noteNumber) + "@" + juce::String(startBeats, 6));
            result.fields.set("created", "false");
            result.fields.set("velocity", juce::String(note->getVelocity()));
            return result;
        }
    }

    auto *note = clip->getSequence().addNote(noteNumber,
                                             tracktion::BeatPosition::fromBeats(startBeats),
                                             tracktion::BeatDuration::fromBeats(lengthBeats),
                                             velocity,
                                             0,
                                             &edit->getUndoManager());
    if (note == nullptr)
        return Result::failure("edit-error", "Failed to create MIDI note");

    auto result = Result::success();
    result.fields.set("clipId", clip->itemID.toString());
    result.fields.set("noteKey", juce::String(noteNumber) + "@" + juce::String(startBeats, 6));
    result.fields.set("created", "true");
    result.fields.set("velocity", juce::String(note->getVelocity()));
    return result;
}

Result DebugAppController::handleSetPluginParameter(const Command &command) const
{
    const auto *arguments = getArguments(command);
    juce::String pluginId;
    juce::String parameterId;
    double value = 0.0;
    if (arguments == nullptr || !readRequiredString(*arguments, "pluginId", pluginId)
        || !readRequiredString(*arguments, "parameterId", parameterId)
        || !readRequiredDouble(*arguments, "value", value))
        return Result::failure("invalid-argument", "set-plugin-parameter requires pluginId, parameterId, and numeric value");

    auto *edit = m_debugHost.getCurrentEdit();
    if (edit == nullptr)
        return Result::failure("not-ready", "Edit is unavailable");

    auto plugin = te::findPluginForID(*edit, te::EditItemID::fromVar(pluginId));
    if (plugin == nullptr)
        return Result::failure("not-found", "Plugin was not found");

    te::AutomatableParameter::Ptr parameter;
    for (auto candidate : plugin->getAutomatableParameters())
        if (candidate != nullptr && candidate->paramID == parameterId)
            parameter = candidate;
    if (parameter == nullptr)
        return Result::failure("not-found", "Plugin parameter was not found");

    const auto range = parameter->getValueRange();
    if (value < range.getStart() || value > range.getEnd())
        return Result::failure("invalid-argument", "Plugin parameter value is outside its native range");

    parameter->setParameter(static_cast<float>(value), juce::sendNotification);
    auto result = Result::success();
    result.fields.set("pluginId", plugin->itemID.toString());
    result.fields.set("parameterId", parameter->paramID);
    result.fields.set("value", juce::String(parameter->getCurrentValue(), 6));
    result.fields.set("normalisedValue", juce::String(parameter->getCurrentNormalisedValue(), 6));
    return result;
}

Result DebugAppController::handleQuit() const
{
    NS_LOG_INFO(app, "debug shell requested quit");
    m_debugHost.requestQuit();

    auto result = Result::success();
    result.fields.set("quitting", "true");
    return result;
}
} // namespace NextStudio::Debug

/*
  ==============================================================================

    TrackPresetAdapter.h
    Created: 23 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "PluginPresetInterface.h"
#include "ArpeggiatorPlugin.h"
#include "SimpleSynthPlugin.h"
#include "SoundFontPlugin.h"
#include "Utilities.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class TrackPresetAdapterBase : public PluginPresetInterface
{
public:
    enum class PresetKind
    {
        audio,
        midi
    };

    TrackPresetAdapterBase(te::AudioTrack &track, ApplicationViewState &appState, PresetKind presetKind)
        : m_track(track),
          m_appState(appState),
          m_presetKind(presetKind)
    {
        m_initialPresetLoaded = true;
    }

    te::AudioTrack &getTrack() const { return m_track; }
    PresetKind getPresetKind() const { return m_presetKind; }

    // Presets should capture plugin/modifier setup, but not song automation.
    static void sanitiseState(juce::ValueTree v, juce::UndoManager *um = nullptr)
    {
        std::vector<juce::ValueTree> nodesToProcess;
        nodesToProcess.reserve(64);
        nodesToProcess.push_back(v);

        int safeguard = 0;
        const int maxNodes = 20000;

        while (!nodesToProcess.empty())
        {
            if (++safeguard > maxNodes)
                break;

            auto current = nodesToProcess.back();
            nodesToProcess.pop_back();

            for (int i = current.getNumChildren(); --i >= 0;)
            {
                auto child = current.getChild(i);
                if (child.hasType(te::IDs::AUTOMATIONCURVE))
                    current.removeChild(i, um);
                else
                    nodesToProcess.push_back(child);
            }
        }
    }

    bool getInitialPresetLoaded() override { return m_initialPresetLoaded; }
    void setInitialPresetLoaded(bool loaded) override { m_initialPresetLoaded = loaded; }

    juce::String getLastLoadedPresetName() override { return m_lastLoadedPresetName; }
    void setLastLoadedPresetName(const juce::String &name) override { m_lastLoadedPresetName = name; }

    ApplicationViewState &getApplicationViewState() override { return m_appState; }

    juce::Array<juce::File> getAdditionalPresetSearchDirectories() const override
    {
        juce::Array<juce::File> dirs;
        auto userPresetDir = juce::File(m_appState.m_presetDir.get());
        auto legacyDir = userPresetDir.getChildFile("TrackPresets");
        auto primaryDir = userPresetDir.getChildFile(getPresetSubfolder());

        if (legacyDir != primaryDir)
            dirs.add(legacyDir);

        return dirs;
    }

    bool isPresetVisibleInList(const juce::ValueTree &state) const override
    {
        if (!state.hasType(juce::Identifier("PLUGIN")))
            return false;

        const auto type = state.getProperty("type").toString();
        if (type == getPluginTypeName())
            return true;

        if (type != "AudioTrack")
            return false;

        // IMPORTANT: this method is called while rebuilding preset lists during normal UI
        // interactions (e.g. track selection). We must keep it cheap and avoid any validation
        // path that instantiates plugins, because that made track selection / creation laggy.
        // Full compatibility checks therefore happen later in applyPresetState() when the user
        // actually loads a preset.
        //
        // Legacy track presets used the generic AudioTrack type for both audio and MIDI targets,
        // so here we only apply a cheap heuristic and filter out obvious MIDI-only cases from
        // audio targets.
        if (m_presetKind == PresetKind::midi)
            return true;

        return !legacyPresetRequiresMidiTarget(state);
    }

    bool isPresetCompatible(const juce::ValueTree &state) const override
    {
        if (!isPresetVisibleInList(state))
            return false;

        const auto type = state.getProperty("type").toString();

        if (type == getPluginTypeName())
            return isPreparedPresetCompatible(state);

        if (type == "AudioTrack")
            return isLegacyPresetCompatible(state);

        return false;
    }

protected:
    static std::optional<EngineHelpers::PluginChainRole> getNonExternalPluginRoleForType(const juce::String &type)
    {
        if (type == ArpeggiatorPlugin::xmlTypeName)
            return EngineHelpers::PluginChainRole::midiEffect;

        if (type == te::SamplerPlugin::xmlTypeName || type == SoundFontPlugin::xmlTypeName || type == SimpleSynthPlugin::xmlTypeName)
            return EngineHelpers::PluginChainRole::instrument;

        if (type == te::ExternalPlugin::xmlTypeName || type == te::RackInstance::xmlTypeName)
            return std::nullopt;

        return EngineHelpers::PluginChainRole::audioEffect;
    }

    std::unique_ptr<juce::PluginDescription> findExternalPluginDescription(const juce::ValueTree &pluginState) const
    {
        const auto uniqueId = (int) pluginState[te::IDs::uniqueId].toString().getHexValue64();
        const auto deprecatedUid = (int) pluginState[te::IDs::uid].toString().getHexValue64();
        const auto fileOrIdentifier = pluginState[te::IDs::filename].toString();
        const auto name = pluginState[te::IDs::name].toString();
        const auto manufacturer = pluginState[te::IDs::manufacturer].toString();

        auto &engine = m_track.edit.engine;
        auto &knownPluginList = engine.getPluginManager().knownPluginList;
        const auto knownTypes = knownPluginList.getTypes();

        if (uniqueId != 0)
            for (auto desc : knownTypes)
                if (desc.uniqueId == uniqueId)
                    return std::make_unique<juce::PluginDescription>(desc);

        if (deprecatedUid != 0)
            for (auto desc : knownTypes)
                if (desc.deprecatedUid == deprecatedUid)
                    return std::make_unique<juce::PluginDescription>(desc);

        if (fileOrIdentifier.isNotEmpty())
        {
            for (auto desc : knownTypes)
                if (desc.fileOrIdentifier == fileOrIdentifier)
                    return std::make_unique<juce::PluginDescription>(desc);

            if (auto desc = engine.getEngineBehaviour().findDescriptionForFileOrID(fileOrIdentifier))
                return desc;
        }

        if (name.isNotEmpty())
            for (auto desc : knownTypes)
                if (desc.name == name && (manufacturer.isEmpty() || desc.manufacturerName == manufacturer))
                    return std::make_unique<juce::PluginDescription>(desc);

        return {};
    }

    std::optional<EngineHelpers::PluginChainRole> getPluginRoleFromState(const juce::ValueTree &pluginState) const
    {
        const auto type = pluginState.getProperty("type").toString();

        if (const auto role = getNonExternalPluginRoleForType(type))
            return role;

        if (type == te::ExternalPlugin::xmlTypeName)
            if (auto desc = findExternalPluginDescription(pluginState))
                return EngineHelpers::getPluginChainRole(*desc, type);

        return std::nullopt;
    }

    bool legacyPresetRequiresMidiTarget(const juce::ValueTree &state) const
    {
        auto pluginsTree = state.getChildWithName("PLUGINS");
        if (!pluginsTree.isValid())
            return false;

        for (const auto &pluginState : pluginsTree)
            if (const auto role = getPluginRoleFromState(pluginState); role && *role != EngineHelpers::PluginChainRole::audioEffect)
                return true;

        return false;
    }

    struct PreparedPluginState
    {
        juce::ValueTree state;
        EngineHelpers::PluginChainRole role = EngineHelpers::PluginChainRole::audioEffect;
    };

    juce::ValueTree createBasePresetState() const
    {
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", getPluginTypeName(), nullptr);
        state.setProperty("presetClass", "TrackPreset", nullptr);
        state.setProperty("schemaVersion", 2, nullptr);
        state.setProperty("trackKind", m_presetKind == PresetKind::midi ? "midi" : "audio", nullptr);
        return state;
    }

    juce::ValueTree createFactoryDefaultTrackState() const
    {
        auto state = createBasePresetState();
        state.setProperty("volume", 0.0f, nullptr);
        state.setProperty("pan", 0.0f, nullptr);
        state.addChild(juce::ValueTree("PLUGINS"), -1, nullptr);
        state.addChild(juce::ValueTree("MODIFIERS"), -1, nullptr);
        return state;
    }

    void appendVolumePanState(juce::ValueTree &state) const
    {
        if (auto volPlugin = m_track.getVolumePlugin())
        {
            state.setProperty("volume", volPlugin->getVolumeDb(), nullptr);
            state.setProperty("pan", volPlugin->getPan(), nullptr);
        }
    }

    void appendModifierState(juce::ValueTree &state) const
    {
        if (auto *ml = m_track.getModifierList())
        {
            juce::ValueTree modsTree("MODIFIERS");
            for (auto mod : ml->getModifiers())
            {
                auto modifierState = mod->state.createCopy();
                sanitiseState(modifierState);
                modsTree.addChild(modifierState, -1, nullptr);
            }

            state.addChild(modsTree, -1, nullptr);
        }
    }

    template <typename Predicate>
    void appendPluginState(juce::ValueTree &state, Predicate &&shouldIncludePlugin) const
    {
        juce::ValueTree pluginsTree("PLUGINS");

        for (auto plugin : m_track.pluginList)
        {
            if (plugin == m_track.getVolumePlugin() || plugin == m_track.getLevelMeterPlugin())
                continue;

            if (!shouldIncludePlugin(*plugin))
                continue;

            auto pluginState = plugin->state.createCopy();
            sanitiseState(pluginState);
            pluginsTree.addChild(pluginState, -1, nullptr);
        }

        state.addChild(pluginsTree, -1, nullptr);
    }

    void appendReservedPluginState(juce::ValueTree &state) const
    {
        juce::ValueTree reservedTree("RESERVEDPLUGINS");

        if (auto *volumePlugin = m_track.getVolumePlugin())
        {
            auto assignments = volumePlugin->state.getChildWithName(te::IDs::MODIFIERASSIGNMENTS);
            if (assignments.isValid() && assignments.getNumChildren() > 0)
            {
                auto volumeState = assignments.createCopy();
                sanitiseState(volumeState);
                volumeState.setProperty("pluginType", te::VolumeAndPanPlugin::xmlTypeName, nullptr);
                reservedTree.addChild(volumeState, -1, nullptr);
            }
        }

        if (reservedTree.getNumChildren() > 0)
            state.addChild(reservedTree, -1, nullptr);
    }

    void applyVolumePanState(const juce::ValueTree &state) const
    {
        if (auto volPlugin = m_track.getVolumePlugin())
        {
            if (state.hasProperty("volume"))
                volPlugin->setVolumeDb(state.getProperty("volume"));

            if (state.hasProperty("pan"))
                volPlugin->setPan(state.getProperty("pan"));
        }
    }

    // Modifier assignments are resolved when the plugin/parameter is created,
    // so source IDs must already point at the newly inserted track-local modifiers.
    static void remapModifierAssignmentSources(juce::ValueTree state, const std::map<te::EditItemID, te::EditItemID> &remappedIDs)
    {
        if (te::ModifierList::isModifier(state.getType()) && state.hasProperty(te::IDs::source))
        {
            const auto oldID = te::EditItemID::fromProperty(state, te::IDs::source);
            if (const auto it = remappedIDs.find(oldID); it != remappedIDs.end() && it->second.isValid())
                state.setProperty(te::IDs::source, it->second, nullptr);
        }

        for (const auto &child : state)
            remapModifierAssignmentSources(child, remappedIDs);
    }

    void restoreReservedPluginState(const juce::ValueTree &state, const std::map<te::EditItemID, te::EditItemID> &remappedIDs) const
    {
        auto *volumePlugin = m_track.getVolumePlugin();
        if (volumePlugin == nullptr)
            return;

        auto targetAssignments = volumePlugin->state.getOrCreateChildWithName(te::IDs::MODIFIERASSIGNMENTS, &m_track.edit.getUndoManager());
        while (targetAssignments.getNumChildren() > 0)
            targetAssignments.removeChild(0, &m_track.edit.getUndoManager());

        auto reservedTree = state.getChildWithName("RESERVEDPLUGINS");
        if (!reservedTree.isValid())
            return;

        for (const auto &reservedState : reservedTree)
        {
            const auto pluginType = reservedState.getProperty("pluginType").toString();
            if (pluginType != te::VolumeAndPanPlugin::xmlTypeName)
                continue;

            auto sourceAssignments = reservedState.createCopy();
            sourceAssignments.removeProperty("pluginType", nullptr);
            remapModifierAssignmentSources(sourceAssignments, remappedIDs);
            sanitiseState(sourceAssignments);

            for (const auto &assignmentState : sourceAssignments)
                targetAssignments.addChild(assignmentState.createCopy(), -1, &m_track.edit.getUndoManager());
        }
    }

    void restoreModifierState(const juce::ValueTree &state, std::map<te::EditItemID, te::EditItemID> &remappedIDs) const
    {
        if (auto *ml = m_track.getModifierList())
        {
            auto existingModifiers = ml->getModifiers();
            for (int i = existingModifiers.size(); --i >= 0;)
                if (auto modifier = existingModifiers.getUnchecked(i))
                    modifier->remove();

            auto modsTree = state.getChildWithName("MODIFIERS");
            if (!modsTree.isValid())
                return;

            int insertIndex = 0;
            for (const auto &modifierState : modsTree)
            {
                if (!te::ModifierList::isModifier(modifierState.getType()))
                    continue;

                auto stateCopy = modifierState.createCopy();
                te::EditItemID::remapIDs(stateCopy, nullptr, m_track.edit, &remappedIDs);
                sanitiseState(stateCopy);
                ml->insertModifier(stateCopy, insertIndex++, nullptr);
            }
        }
    }

    void clearNonReservedPlugins() const
    {
        auto *volumePlugin = m_track.getVolumePlugin();
        auto *levelMeterPlugin = m_track.getLevelMeterPlugin();

        for (int i = m_track.pluginList.size() - 1; i >= 0; --i)
        {
            auto *plugin = m_track.pluginList[i];
            if (plugin != volumePlugin && plugin != levelMeterPlugin)
                plugin->deleteFromParent();
        }
    }

    bool collectPreparedPluginStates(const juce::ValueTree &state, std::vector<PreparedPluginState> &prepared) const
    {
        prepared.clear();

        auto pluginsTree = state.getChildWithName("PLUGINS");
        if (!pluginsTree.isValid())
            return true;

        for (const auto &pluginState : pluginsTree)
        {
            auto stateCopy = pluginState.createCopy();
            sanitiseState(stateCopy);

            auto tempPlugin = m_track.edit.engine.getPluginManager().createNewPlugin(m_track.edit, stateCopy);
            if (tempPlugin == nullptr)
            {
                GUIHelpers::log("TrackPresetAdapter: Failed to create plugin from preset state for track '" + m_track.getName() + "'.");
                return false;
            }

            prepared.push_back({stateCopy, EngineHelpers::getPluginChainRole(*tempPlugin)});
        }

        return true;
    }

    static bool validateAudioPreparedPlugins(const std::vector<PreparedPluginState> &prepared)
    {
        for (const auto &plugin : prepared)
            if (plugin.role != EngineHelpers::PluginChainRole::audioEffect)
                return false;

        return true;
    }

    static bool validateMidiPreparedPlugins(const std::vector<PreparedPluginState> &prepared)
    {
        bool seenInstrument = false;
        bool seenAudio = false;

        for (const auto &plugin : prepared)
        {
            switch (plugin.role)
            {
            case EngineHelpers::PluginChainRole::midiEffect:
                if (seenInstrument || seenAudio)
                    return false;
                break;

            case EngineHelpers::PluginChainRole::instrument:
                if (seenInstrument || seenAudio)
                    return false;
                seenInstrument = true;
                break;

            case EngineHelpers::PluginChainRole::audioEffect:
                seenAudio = true;
                break;
            }
        }

        return true;
    }

    template <typename Validator>
    bool validatePreparedPreset(const juce::ValueTree &state, Validator &&validator) const
    {
        std::vector<PreparedPluginState> prepared;
        return collectPreparedPluginStates(state, prepared) && validator(prepared);
    }

    void applyPreparedPlugins(const std::vector<PreparedPluginState> &prepared, std::map<te::EditItemID, te::EditItemID> &remappedIDs) const
    {
        int insertIndex = 0;
        for (const auto &plugin : prepared)
        {
            auto stateCopy = plugin.state.createCopy();
            te::EditItemID::remapIDs(stateCopy, nullptr, m_track.edit, &remappedIDs);
            remapModifierAssignmentSources(stateCopy, remappedIDs);
            sanitiseState(stateCopy);
            m_track.pluginList.insertPlugin(stateCopy, insertIndex++);
        }
    }

    // Safety net matching Tracktion's track-paste behaviour: if an assignment still
    // points at an old modifier ID, redirect it to the remapped one or remove it.
    void repairModifierAssignments(const std::map<te::EditItemID, te::EditItemID> &remappedIDs) const
    {
        for (auto param : m_track.getAllAutomatableParams())
        {
            auto assignments = param->getAssignments();

            for (int i = assignments.size(); --i >= 0;)
            {
                auto ass = assignments.getUnchecked(i);

                if (dynamic_cast<te::MacroParameter::Assignment *>(ass.get()) != nullptr)
                    continue;

                const auto oldID = te::EditItemID::fromProperty(ass->state, te::IDs::source);
                if (const auto it = remappedIDs.find(oldID); it != remappedIDs.end() && it->second.isValid())
                {
                    ass->state.setProperty(te::IDs::source, it->second, nullptr);
                    continue;
                }

                if (auto tt = te::getTrackContainingModifier(m_track.edit, te::findModifierForID(m_track.edit, oldID)))
                    if (tt == &m_track || te::TrackList::isFixedTrack(tt->state) || m_track.isAChildOf(*tt))
                        continue;

                param->removeModifier(*ass);
            }
        }
    }

    template <typename Validator>
    bool restorePreparedPreset(const juce::ValueTree &state, Validator &&validator, const juce::String &context) const
    {
        if (!isPresetVisibleInList(state))
            return false;

        std::vector<PreparedPluginState> prepared;
        if (!collectPreparedPluginStates(state, prepared) || !validator(prepared))
        {
            GUIHelpers::log(context + ": Rejected incompatible track preset for track '" + getTrack().getName() + "'.");
            return false;
        }

        std::map<te::EditItemID, te::EditItemID> remappedIDs;
        sanitiseState(getTrack().state, &getTrack().edit.getUndoManager());
        applyVolumePanState(state);
        restoreModifierState(state, remappedIDs);
        restoreReservedPluginState(state, remappedIDs);
        clearNonReservedPlugins();
        applyPreparedPlugins(prepared, remappedIDs);
        repairModifierAssignments(remappedIDs);
        return true;
    }

    virtual bool isPreparedPresetCompatible(const juce::ValueTree &state) const = 0;
    virtual bool isLegacyPresetCompatible(const juce::ValueTree &state) const = 0;

private:
    te::AudioTrack &m_track;
    ApplicationViewState &m_appState;
    PresetKind m_presetKind;
    bool m_initialPresetLoaded = false;
    juce::String m_lastLoadedPresetName;
};

class AudioTrackPresetAdapter : public TrackPresetAdapterBase
{
public:
    AudioTrackPresetAdapter(te::AudioTrack &track, ApplicationViewState &appState)
        : TrackPresetAdapterBase(track, appState, PresetKind::audio)
    {
    }

    juce::ValueTree getPluginState() override
    {
        auto state = createBasePresetState();
        appendVolumePanState(state);
        appendPluginState(state, [] (te::Plugin &plugin)
                          { return EngineHelpers::getPluginChainRole(plugin) == EngineHelpers::PluginChainRole::audioEffect; });
        appendModifierState(state);
        appendReservedPluginState(state);
        return state;
    }

    bool applyPresetState(const juce::ValueTree &state) override
    {
        // This is the intentional "expensive" path used only for actual preset loading.
        // Unlike isPresetVisibleInList(), deeper validation is allowed here because the user
        // explicitly requested applying the preset.
        return restorePreparedPreset(state, validateAudioPreparedPlugins, "AudioTrackPresetAdapter");
    }

    void restorePluginState(const juce::ValueTree &state) override
    {
        applyPresetState(state);
    }

    juce::ValueTree getFactoryDefaultState() override
    {
        return createFactoryDefaultTrackState();
    }

    juce::String getPresetSubfolder() const override { return "TrackPresets/Audio"; }
    juce::String getPluginTypeName() const override { return "AudioTrackPreset"; }

protected:
    bool isPreparedPresetCompatible(const juce::ValueTree &state) const override
    {
        return validatePreparedPreset(state, validateAudioPreparedPlugins);
    }

    bool isLegacyPresetCompatible(const juce::ValueTree &state) const override
    {
        return validatePreparedPreset(state, validateAudioPreparedPlugins);
    }
};

class MidiTrackPresetAdapter : public TrackPresetAdapterBase
{
public:
    MidiTrackPresetAdapter(te::AudioTrack &track, ApplicationViewState &appState)
        : TrackPresetAdapterBase(track, appState, PresetKind::midi)
    {
    }

    juce::ValueTree getPluginState() override
    {
        auto state = createBasePresetState();
        appendVolumePanState(state);
        appendPluginState(state, [] (te::Plugin &)
                          { return true; });
        appendModifierState(state);
        appendReservedPluginState(state);
        return state;
    }

    bool applyPresetState(const juce::ValueTree &state) override
    {
        // This is the intentional "expensive" path used only for actual preset loading.
        // Unlike isPresetVisibleInList(), deeper validation is allowed here because the user
        // explicitly requested applying the preset.
        return restorePreparedPreset(state, validateMidiPreparedPlugins, "MidiTrackPresetAdapter");
    }

    void restorePluginState(const juce::ValueTree &state) override
    {
        applyPresetState(state);
    }

    juce::ValueTree getFactoryDefaultState() override
    {
        return createFactoryDefaultTrackState();
    }

    juce::String getPresetSubfolder() const override { return "TrackPresets/Midi"; }
    juce::String getPluginTypeName() const override { return "MidiTrackPreset"; }

protected:
    bool isPreparedPresetCompatible(const juce::ValueTree &state) const override
    {
        return validatePreparedPreset(state, validateMidiPreparedPlugins);
    }

    bool isLegacyPresetCompatible(const juce::ValueTree &state) const override
    {
        return validatePreparedPreset(state, validateMidiPreparedPlugins);
    }
};

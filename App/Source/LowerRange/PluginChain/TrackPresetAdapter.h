/*
  ==============================================================================

    TrackPresetAdapter.h
    Created: 23 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginPresetInterface.h"
#include "Utilities/Utilities.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class TrackPresetAdapter : public PluginPresetInterface
{
public:
    TrackPresetAdapter(te::AudioTrack &track, ApplicationViewState &appState)
        : m_track(track),
          m_appState(appState)
    {
        m_initialPresetLoaded = true; // Prevent loading "init" preset automatically, preserving current track state
    }

    /**
     * Recursively traverses the ValueTree and removes automation data (curves).
     * This is used to ensure that presets don't accidentally contain song-specific automation.
     * Uses an iterative approach (stack-based) to prevent stack overflow on deep trees.
     */
    static void sanitiseState(juce::ValueTree v, juce::UndoManager *um = nullptr)
    {
        // Iterative approach to avoid stack overflow with deep trees
        std::vector<juce::ValueTree> nodesToProcess;
        nodesToProcess.reserve(64);
        nodesToProcess.push_back(v);

        int safeguard = 0;
        const int maxNodes = 20000; // Emergency brake for extremely deep/large trees

        while (!nodesToProcess.empty())
        {
            if (++safeguard > maxNodes)
            {
                // In a real scenario, we might want to log this
                break;
            }

            auto current = nodesToProcess.back();
            nodesToProcess.pop_back();

            for (int i = current.getNumChildren(); --i >= 0;)
            {
                auto child = current.getChild(i);
                if (child.hasType(te::IDs::AUTOMATIONCURVE))
                {
                    current.removeChild(i, um);
                }
                else
                {
                    nodesToProcess.push_back(child);
                }
            }
        }
    }

    juce::ValueTree getPluginState() override
    {
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", getPluginTypeName(), nullptr);

        if (auto volPlugin = m_track.getVolumePlugin())
        {
            state.setProperty("volume", volPlugin->getVolumeDb(), nullptr);
            state.setProperty("pan", volPlugin->getPan(), nullptr);
        }

        juce::ValueTree pluginsTree("PLUGINS");
        for (auto plugin : m_track.pluginList)
        {
            if (plugin == m_track.getVolumePlugin() || plugin == m_track.getLevelMeterPlugin() || plugin == m_track.getEqualiserPlugin())
                continue;

            auto pState = plugin->state.createCopy();
            sanitiseState(pState);
            pluginsTree.addChild(pState, -1, nullptr);
        }
        state.addChild(pluginsTree, -1, nullptr);

        if (auto *ml = m_track.getModifierList())
        {
            juce::ValueTree modsTree("MODIFIERS");
            for (auto mod : ml->getModifiers())
            {
                auto mState = mod->state.createCopy();
                sanitiseState(mState);
                modsTree.addChild(mState, -1, nullptr);
            }

            state.addChild(modsTree, -1, nullptr);
        }

        return state;
    }

    void restorePluginState(const juce::ValueTree &state) override
    {
        if (state.getProperty("type").toString() != getPluginTypeName())
            return;

        // Clear existing automation to prevent conflicts with the new preset state
        sanitiseState(m_track.state, &m_track.edit.getUndoManager());

        if (auto volPlugin = m_track.getVolumePlugin())
        {
            if (state.hasProperty("volume"))
                volPlugin->setVolumeDb(state.getProperty("volume"));

            if (state.hasProperty("pan"))
                volPlugin->setPan(state.getProperty("pan"));
        }

        if (auto *ml = m_track.getModifierList())
        {
            for (int i = ml->getModifiers().size() - 1; i >= 0; --i)
                if (auto m = ml->getModifiers()[i])
                    m->remove();

            auto modsTree = state.getChildWithName("MODIFIERS");
            if (modsTree.isValid())
            {
                int insertIndex = 0;
                for (const auto &mState : modsTree)
                {
                    if (te::ModifierList::isModifier(mState.getType()))
                    {
                        auto mStateCopy = mState.createCopy();
                        sanitiseState(mStateCopy);
                        ml->insertModifier(mStateCopy, insertIndex++, nullptr);
                    }
                }
            }
        }

        for (int i = m_track.pluginList.size() - 1; i >= 0; --i)
        {
            auto p = m_track.pluginList[i];
            if (p != m_track.getVolumePlugin() && p != m_track.getLevelMeterPlugin() && p != m_track.getEqualiserPlugin())
            {
                p->deleteFromParent();
            }
        }

        auto pluginsTree = state.getChildWithName("PLUGINS");
        if (pluginsTree.isValid())
        {
            int insertIndex = 0;
            for (const auto &pState : pluginsTree)
            {
                auto pStateCopy = pState.createCopy();
                sanitiseState(pStateCopy);
                m_track.pluginList.insertPlugin(pStateCopy, insertIndex++);
            }
        }
    }

    juce::ValueTree getFactoryDefaultState() override
    {
        // Default state: 0dB, Center Pan, No plugins
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", getPluginTypeName(), nullptr);
        state.setProperty("volume", 0.0f, nullptr);
        state.setProperty("pan", 0.0f, nullptr);
        state.addChild(juce::ValueTree("PLUGINS"), -1, nullptr);
        state.addChild(juce::ValueTree("MODIFIERS"), -1, nullptr);
        return state;
    }

    bool getInitialPresetLoaded() override { return m_initialPresetLoaded; }
    void setInitialPresetLoaded(bool loaded) override { m_initialPresetLoaded = loaded; }

    juce::String getLastLoadedPresetName() override { return m_lastLoadedPresetName; }
    void setLastLoadedPresetName(const juce::String &name) override { m_lastLoadedPresetName = name; }

    juce::String getPresetSubfolder() const override { return "TrackPresets"; }
    juce::String getPluginTypeName() const override { return "AudioTrack"; }
    ApplicationViewState &getApplicationViewState() override { return m_appState; }

    te::AudioTrack &getTrack() const { return m_track; }

private:
    te::AudioTrack &m_track;
    ApplicationViewState &m_appState;
    bool m_initialPresetLoaded = false;
    juce::String m_lastLoadedPresetName;
};

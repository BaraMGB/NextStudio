/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>
#include <map>

namespace juce
{
class PluginDescription;
}

struct PluginIdentity
{
    enum class Kind
    {
        external,
        internal
    };

    static PluginIdentity fromDescription(const juce::PluginDescription &, const juce::String &xmlType = {});

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool operator==(const PluginIdentity &) const;
    [[nodiscard]] bool operator!=(const PluginIdentity &other) const { return !(*this == other); }

    Kind kind{Kind::external};
    juce::String format;
    juce::String fileOrIdentifier;
    juce::String xmlType;
    int uniqueId{};
    int deprecatedUid{};
};

class PluginCollections : public juce::ChangeBroadcaster
{
public:
    struct Collection
    {
        juce::String id;
        juce::String name;
    };

    using PersistCallback = std::function<void()>;

    explicit PluginCollections(juce::ValueTree applicationState, PersistCallback persist = {});

    [[nodiscard]] juce::Array<Collection> getCollections() const;
    [[nodiscard]] juce::String getCollectionName(const juce::String &collectionId) const;

    bool addCollection(const juce::String &name, juce::String *createdId = nullptr);
    bool renameCollection(const juce::String &collectionId, const juce::String &newName);
    bool removeCollection(const juce::String &collectionId);
    [[nodiscard]] bool isCollectionNameAvailable(const juce::String &name, const juce::String &exceptCollectionId = {}) const;

    [[nodiscard]] bool isFavorite(const PluginIdentity &) const;
    bool setFavorite(const PluginIdentity &, const juce::String &displayName, bool shouldBeFavorite);
    void sortByFavorite(juce::Array<juce::PluginDescription> &, bool favoritesFirst) const;

    [[nodiscard]] bool isInCollection(const PluginIdentity &, const juce::String &collectionId) const;
    bool setCollectionMembership(const PluginIdentity &, const juce::String &displayName, const juce::String &collectionId, bool shouldBeMember);

    [[nodiscard]] juce::ValueTree getState() const { return m_state; }

    static juce::String normaliseCollectionName(const juce::String &name);

private:
    juce::ValueTree findCollection(const juce::String &collectionId) const;
    juce::ValueTree findPlugin(const PluginIdentity &) const;
    juce::ValueTree getOrCreatePlugin(const PluginIdentity &, const juce::String &displayName);
    static bool pluginNodeHasCollections(const juce::ValueTree &);
    static juce::String identityKey(const PluginIdentity &);
    static PluginIdentity identityFromNode(const juce::ValueTree &);
    void rebuildPluginIndex();
    void removePluginIfEmpty(juce::ValueTree);
    void initialiseDefaults();
    void changed();

    juce::ValueTree m_state;
    PersistCallback m_persist;
    std::map<juce::String, juce::ValueTree> m_pluginIndex;
};

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

#include "PluginCollections.h"

namespace
{
const juce::Identifier pluginCollectionsType{"PluginCollections"};
const juce::Identifier collectionsType{"Collections"};
const juce::Identifier collectionType{"Collection"};
const juce::Identifier pluginsType{"Plugins"};
const juce::Identifier pluginType{"Plugin"};
const juce::Identifier collectionRefType{"CollectionRef"};

const juce::Identifier versionProperty{"version"};
const juce::Identifier idProperty{"id"};
const juce::Identifier nameProperty{"name"};
const juce::Identifier kindProperty{"kind"};
const juce::Identifier formatProperty{"format"};
const juce::Identifier fileProperty{"file"};
const juce::Identifier xmlTypeProperty{"xmlType"};
const juce::Identifier uniqueIdProperty{"uniqueId"};
const juce::Identifier deprecatedUidProperty{"deprecatedUid"};
const juce::Identifier favoriteProperty{"favorite"};
const juce::Identifier collectionIdProperty{"collectionId"};

const juce::Identifier legacyCategoriesType{"Categories"};
const juce::Identifier legacyCategoryType{"Category"};
const juce::Identifier legacyCategoryRefType{"CategoryRef"};
const juce::Identifier legacyCategoryIdProperty{"categoryId"};

constexpr auto internalFormatName = "TracktionInternal";
constexpr int schemaVersion = 2;

struct CollectionNameComparator
{
    static int compareElements(const PluginCollections::Collection &first, const PluginCollections::Collection &second) { return first.name.compareNatural(second.name, false); }
};

bool migrateLegacyCategories(juce::ValueTree state)
{
    const auto legacyCategories = state.getChildWithName(legacyCategoriesType);
    if (!legacyCategories.isValid())
        return false;

    auto collections = state.getOrCreateChildWithName(collectionsType, nullptr);
    for (const auto legacyCategory : legacyCategories)
    {
        if (!legacyCategory.hasType(legacyCategoryType))
            continue;

        juce::ValueTree collection(collectionType);
        collection.setProperty(idProperty, legacyCategory[idProperty], nullptr);
        collection.setProperty(nameProperty, legacyCategory[nameProperty], nullptr);
        collections.addChild(collection, -1, nullptr);
    }

    auto plugins = state.getOrCreateChildWithName(pluginsType, nullptr);
    for (auto plugin : plugins)
    {
        for (int index = plugin.getNumChildren(); --index >= 0;)
        {
            const auto legacyRef = plugin.getChild(index);
            if (!legacyRef.hasType(legacyCategoryRefType))
                continue;

            juce::ValueTree collectionRef(collectionRefType);
            collectionRef.setProperty(collectionIdProperty, legacyRef[legacyCategoryIdProperty], nullptr);
            plugin.addChild(collectionRef, -1, nullptr);
            plugin.removeChild(index, nullptr);
        }
    }

    state.removeChild(legacyCategories, nullptr);
    return true;
}
} // namespace

PluginIdentity PluginIdentity::fromDescription(const juce::PluginDescription &description, const juce::String &explicitXmlType)
{
    PluginIdentity result;
    if (description.pluginFormatName == internalFormatName)
    {
        result.kind = Kind::internal;
        result.xmlType = explicitXmlType.isNotEmpty() ? explicitXmlType : description.category;
        return result;
    }

    result.kind = Kind::external;
    result.format = description.pluginFormatName;
    result.fileOrIdentifier = description.fileOrIdentifier;
    result.uniqueId = description.uniqueId;
    result.deprecatedUid = description.deprecatedUid;
    return result;
}

bool PluginIdentity::isValid() const
{
    if (kind == Kind::internal)
        return xmlType.isNotEmpty();

    return format.isNotEmpty() && (fileOrIdentifier.isNotEmpty() || uniqueId != 0 || deprecatedUid != 0);
}

bool PluginIdentity::operator==(const PluginIdentity &other) const
{
    if (kind != other.kind)
        return false;

    if (kind == Kind::internal)
        return xmlType == other.xmlType;

    return format == other.format && fileOrIdentifier == other.fileOrIdentifier && uniqueId == other.uniqueId && deprecatedUid == other.deprecatedUid;
}

PluginCollections::PluginCollections(juce::ValueTree applicationState, PersistCallback persist)
    : m_persist(std::move(persist))
{
    m_state = applicationState.getChildWithName(pluginCollectionsType);
    if (!m_state.isValid())
    {
        m_state = juce::ValueTree(pluginCollectionsType);
        applicationState.addChild(m_state, -1, nullptr);
        initialiseDefaults();
    }
    else
    {
        migrateLegacyCategories(m_state);
        m_state.getOrCreateChildWithName(collectionsType, nullptr);
        m_state.getOrCreateChildWithName(pluginsType, nullptr);
        m_state.setProperty(versionProperty, schemaVersion, nullptr);
    }

    rebuildPluginIndex();
}

juce::Array<PluginCollections::Collection> PluginCollections::getCollections() const
{
    juce::Array<Collection> result;
    const auto collections = m_state.getChildWithName(collectionsType);
    for (const auto collection : collections)
    {
        if (!collection.hasType(collectionType))
            continue;

        const auto id = collection[idProperty].toString();
        const auto name = collection[nameProperty].toString();
        if (id.isNotEmpty() && name.isNotEmpty())
            result.add({id, name});
    }

    CollectionNameComparator comparator;
    result.sort(comparator, true);
    return result;
}

juce::String PluginCollections::getCollectionName(const juce::String &collectionId) const
{
    const auto collection = findCollection(collectionId);
    return collection.isValid() ? collection[nameProperty].toString() : juce::String();
}

juce::String PluginCollections::normaliseCollectionName(const juce::String &name) { return name.trim().substring(0, 64); }

bool PluginCollections::isCollectionNameAvailable(const juce::String &name, const juce::String &exceptCollectionId) const
{
    const auto trimmed = name.trim();
    if (trimmed.isEmpty() || trimmed.length() > 64)
        return false;
    const auto candidate = normaliseCollectionName(trimmed);

    const juce::StringArray reservedNames{"All Plugins", "Favorites", "My Collections"};
    for (const auto &reserved : reservedNames)
        if (candidate.equalsIgnoreCase(reserved))
            return false;

    for (const auto &collection : getCollections())
        if (collection.id != exceptCollectionId && collection.name.equalsIgnoreCase(candidate))
            return false;

    return true;
}

bool PluginCollections::addCollection(const juce::String &name, juce::String *createdId)
{
    if (!isCollectionNameAvailable(name))
        return false;
    const auto normalised = normaliseCollectionName(name);

    const auto id = juce::Uuid().toString();
    juce::ValueTree collection(collectionType);
    collection.setProperty(idProperty, id, nullptr);
    collection.setProperty(nameProperty, normalised, nullptr);
    m_state.getOrCreateChildWithName(collectionsType, nullptr).addChild(collection, -1, nullptr);

    if (createdId != nullptr)
        *createdId = id;

    changed();
    return true;
}

bool PluginCollections::renameCollection(const juce::String &collectionId, const juce::String &newName)
{
    auto collection = findCollection(collectionId);
    if (!collection.isValid() || !isCollectionNameAvailable(newName, collectionId))
        return false;
    const auto normalised = normaliseCollectionName(newName);

    if (collection[nameProperty].toString() == normalised)
        return true;

    collection.setProperty(nameProperty, normalised, nullptr);
    changed();
    return true;
}

bool PluginCollections::removeCollection(const juce::String &collectionId)
{
    auto collection = findCollection(collectionId);
    if (!collection.isValid())
        return false;

    auto collections = m_state.getChildWithName(collectionsType);
    collections.removeChild(collection, nullptr);

    auto plugins = m_state.getChildWithName(pluginsType);
    for (int pluginIndex = plugins.getNumChildren(); --pluginIndex >= 0;)
    {
        auto plugin = plugins.getChild(pluginIndex);
        for (int refIndex = plugin.getNumChildren(); --refIndex >= 0;)
        {
            const auto ref = plugin.getChild(refIndex);
            if (ref.hasType(collectionRefType) && ref[collectionIdProperty].toString() == collectionId)
                plugin.removeChild(refIndex, nullptr);
        }
        removePluginIfEmpty(plugin);
    }

    changed();
    return true;
}

bool PluginCollections::isFavorite(const PluginIdentity &identity) const
{
    const auto plugin = findPlugin(identity);
    return plugin.isValid() && (bool)plugin.getProperty(favoriteProperty, false);
}

bool PluginCollections::setFavorite(const PluginIdentity &identity, const juce::String &displayName, bool shouldBeFavorite)
{
    if (!identity.isValid())
        return false;

    auto plugin = shouldBeFavorite ? getOrCreatePlugin(identity, displayName) : findPlugin(identity);
    if (!plugin.isValid())
        return false;

    const bool oldValue = plugin.getProperty(favoriteProperty, false);
    if (oldValue == shouldBeFavorite)
        return false;

    if (shouldBeFavorite)
        plugin.setProperty(favoriteProperty, true, nullptr);
    else
        plugin.removeProperty(favoriteProperty, nullptr);

    removePluginIfEmpty(plugin);
    changed();
    return true;
}

void PluginCollections::sortByFavorite(juce::Array<juce::PluginDescription> &plugins, bool favoritesFirst) const
{
    struct FavoriteComparator
    {
        static int compareNames(const juce::PluginDescription &first, const juce::PluginDescription &second) { return first.name.compareNatural(second.name, false); }

        int compareElements(const juce::PluginDescription &first, const juce::PluginDescription &second) const
        {
            const bool firstFavorite = collections.isFavorite(PluginIdentity::fromDescription(first, first.category));
            const bool secondFavorite = collections.isFavorite(PluginIdentity::fromDescription(second, second.category));
            if (firstFavorite != secondFavorite)
                return firstFavorite == favoritesFirst ? -1 : 1;
            return compareNames(first, second);
        }

        const PluginCollections &collections;
        bool favoritesFirst;
    } comparator{*this, favoritesFirst};

    plugins.sort(comparator, true);
}

bool PluginCollections::isInCollection(const PluginIdentity &identity, const juce::String &collectionId) const
{
    const auto plugin = findPlugin(identity);
    if (!plugin.isValid())
        return false;

    for (const auto ref : plugin)
        if (ref.hasType(collectionRefType) && ref[collectionIdProperty].toString() == collectionId)
            return true;

    return false;
}

bool PluginCollections::setCollectionMembership(const PluginIdentity &identity, const juce::String &displayName, const juce::String &collectionId, bool shouldBeMember)
{
    if (!identity.isValid() || !findCollection(collectionId).isValid())
        return false;

    auto plugin = shouldBeMember ? getOrCreatePlugin(identity, displayName) : findPlugin(identity);
    if (!plugin.isValid())
        return false;

    int existingIndex = -1;
    for (int i = 0; i < plugin.getNumChildren(); ++i)
    {
        const auto ref = plugin.getChild(i);
        if (ref.hasType(collectionRefType) && ref[collectionIdProperty].toString() == collectionId)
        {
            existingIndex = i;
            break;
        }
    }

    if (shouldBeMember == (existingIndex >= 0))
        return false;

    if (shouldBeMember)
    {
        juce::ValueTree ref(collectionRefType);
        ref.setProperty(collectionIdProperty, collectionId, nullptr);
        plugin.addChild(ref, -1, nullptr);
    }
    else
    {
        plugin.removeChild(existingIndex, nullptr);
        removePluginIfEmpty(plugin);
    }

    changed();
    return true;
}

juce::ValueTree PluginCollections::findCollection(const juce::String &collectionId) const
{
    const auto collections = m_state.getChildWithName(collectionsType);
    for (const auto collection : collections)
        if (collection.hasType(collectionType) && collection[idProperty].toString() == collectionId)
            return collection;

    return {};
}

juce::ValueTree PluginCollections::findPlugin(const PluginIdentity &identity) const
{
    const auto found = m_pluginIndex.find(identityKey(identity));
    return found != m_pluginIndex.end() ? found->second : juce::ValueTree();
}

juce::ValueTree PluginCollections::getOrCreatePlugin(const PluginIdentity &identity, const juce::String &displayName)
{
    auto plugin = findPlugin(identity);
    if (!plugin.isValid())
    {
        plugin = juce::ValueTree(pluginType);
        plugin.setProperty(kindProperty, identity.kind == PluginIdentity::Kind::internal ? "internal" : "external", nullptr);
        plugin.setProperty(nameProperty, displayName, nullptr);
        if (identity.kind == PluginIdentity::Kind::internal)
        {
            plugin.setProperty(xmlTypeProperty, identity.xmlType, nullptr);
        }
        else
        {
            plugin.setProperty(formatProperty, identity.format, nullptr);
            plugin.setProperty(fileProperty, identity.fileOrIdentifier, nullptr);
            plugin.setProperty(uniqueIdProperty, identity.uniqueId, nullptr);
            plugin.setProperty(deprecatedUidProperty, identity.deprecatedUid, nullptr);
        }
        m_state.getOrCreateChildWithName(pluginsType, nullptr).addChild(plugin, -1, nullptr);
        m_pluginIndex[identityKey(identity)] = plugin;
    }
    else if (displayName.isNotEmpty() && plugin[nameProperty].toString() != displayName)
    {
        plugin.setProperty(nameProperty, displayName, nullptr);
    }

    return plugin;
}

bool PluginCollections::pluginNodeHasCollections(const juce::ValueTree &plugin)
{
    if ((bool)plugin.getProperty(favoriteProperty, false))
        return true;

    for (const auto child : plugin)
        if (child.hasType(collectionRefType))
            return true;

    return false;
}

juce::String PluginCollections::identityKey(const PluginIdentity &identity)
{
    if (identity.kind == PluginIdentity::Kind::internal)
        return "internal:" + juce::String(identity.xmlType.length()) + ":" + identity.xmlType;

    return "external:" + juce::String(identity.format.length()) + ":" + identity.format + juce::String(identity.fileOrIdentifier.length()) + ":" + identity.fileOrIdentifier + ":" + juce::String(identity.uniqueId) + ":" + juce::String(identity.deprecatedUid);
}

PluginIdentity PluginCollections::identityFromNode(const juce::ValueTree &plugin)
{
    PluginIdentity identity;
    if (plugin[kindProperty].toString() == "internal")
    {
        identity.kind = PluginIdentity::Kind::internal;
        identity.xmlType = plugin[xmlTypeProperty].toString();
    }
    else
    {
        identity.kind = PluginIdentity::Kind::external;
        identity.format = plugin[formatProperty].toString();
        identity.fileOrIdentifier = plugin[fileProperty].toString();
        identity.uniqueId = plugin.getProperty(uniqueIdProperty, 0);
        identity.deprecatedUid = plugin.getProperty(deprecatedUidProperty, 0);
    }
    return identity;
}

void PluginCollections::rebuildPluginIndex()
{
    m_pluginIndex.clear();
    const auto plugins = m_state.getChildWithName(pluginsType);
    for (const auto plugin : plugins)
    {
        if (!plugin.hasType(pluginType))
            continue;

        const auto identity = identityFromNode(plugin);
        if (identity.isValid())
            m_pluginIndex.emplace(identityKey(identity), plugin);
    }
}

void PluginCollections::removePluginIfEmpty(juce::ValueTree plugin)
{
    if (!plugin.isValid() || pluginNodeHasCollections(plugin))
        return;

    m_pluginIndex.erase(identityKey(identityFromNode(plugin)));
    auto plugins = m_state.getChildWithName(pluginsType);
    plugins.removeChild(plugin, nullptr);
}

void PluginCollections::initialiseDefaults()
{
    m_state.setProperty(versionProperty, schemaVersion, nullptr);
    auto collections = m_state.getOrCreateChildWithName(collectionsType, nullptr);
    m_state.getOrCreateChildWithName(pluginsType, nullptr);

    const std::pair<const char *, const char *> defaults[]{{"default-mixing", "Mixing"}, {"default-mastering", "Mastering"}, {"default-sound-design", "Sound Design"}, {"default-creative", "Creative"}, {"default-utilities", "Utilities"}};

    for (const auto &[id, name] : defaults)
    {
        juce::ValueTree collection(collectionType);
        collection.setProperty(idProperty, id, nullptr);
        collection.setProperty(nameProperty, name, nullptr);
        collections.addChild(collection, -1, nullptr);
    }
}

void PluginCollections::changed()
{
    if (m_persist)
        m_persist();
    sendSynchronousChangeMessage();
}

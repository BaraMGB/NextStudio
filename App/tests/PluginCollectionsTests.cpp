#include "PluginCollections.h"

#include <iostream>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; \
            ++failures; \
        } \
    } while (false)

juce::PluginDescription makeExternal(const juce::String &format, const juce::String &file, int uid, const juce::String &name)
{
    juce::PluginDescription description;
    description.pluginFormatName = format;
    description.fileOrIdentifier = file;
    description.uniqueId = uid;
    description.name = name;
    return description;
}

juce::PluginDescription makeInternal(const juce::String &xmlType, const juce::String &name)
{
    juce::PluginDescription description;
    description.pluginFormatName = "TracktionInternal";
    description.category = xmlType;
    description.name = name;
    return description;
}

juce::String findCollectionId(const PluginCollections &collections, const juce::String &name)
{
    for (const auto &collection : collections.getCollections())
        if (collection.name == name)
            return collection.id;
    return {};
}

void testDefaultsAndCollectionLifecycle()
{
    juce::ValueTree root("AppSettings");
    int persistCount = 0;
    PluginCollections collections(root, [&persistCount] { ++persistCount; });

    const auto defaults = collections.getCollections();
    REQUIRE(defaults.size() == 5);
    REQUIRE(defaults[0].name == "Creative");
    REQUIRE(defaults[1].name == "Mastering");
    REQUIRE(defaults[2].name == "Mixing");
    REQUIRE(defaults[3].name == "Sound Design");
    REQUIRE(defaults[4].name == "Utilities");

    juce::String collectionId;
    REQUIRE(collections.addCollection("  Live Set  ", &collectionId));
    REQUIRE(collectionId.isNotEmpty());
    REQUIRE(collections.getCollectionName(collectionId) == "Live Set");
    REQUIRE(!collections.addCollection("live set"));
    REQUIRE(!collections.addCollection("Favorites"));
    REQUIRE(!collections.addCollection("   "));
    REQUIRE(!collections.addCollection(juce::String::repeatedString("x", 65)));
    REQUIRE(collections.renameCollection(collectionId, "Performance"));
    REQUIRE(collections.getCollectionName(collectionId) == "Performance");
    REQUIRE(collections.removeCollection(collectionId));
    REQUIRE(collections.getCollectionName(collectionId).isEmpty());
    REQUIRE(persistCount == 3);
}

void testIdentityAndMembership()
{
    juce::ValueTree root("AppSettings");
    PluginCollections collections(root);

    const auto firstDescription = makeExternal("VST3", "/plugins/example.vst3", 0x1234, "Compressor");
    auto renamedDescription = firstDescription;
    renamedDescription.name = "Compressor 2";
    const auto otherFormat = makeExternal("AudioUnit", "/plugins/example.vst3", 0x1234, "Compressor");
    const auto first = PluginIdentity::fromDescription(firstDescription);
    const auto renamed = PluginIdentity::fromDescription(renamedDescription);
    const auto different = PluginIdentity::fromDescription(otherFormat);

    REQUIRE(first == renamed);
    REQUIRE(first != different);
    REQUIRE(collections.setFavorite(first, firstDescription.name, true));
    REQUIRE(collections.isFavorite(renamed));
    REQUIRE(!collections.isFavorite(different));
    REQUIRE(!collections.setFavorite(first, firstDescription.name, true));

    const auto mixingId = findCollectionId(collections, "Mixing");
    const auto masteringId = findCollectionId(collections, "Mastering");
    REQUIRE(mixingId.isNotEmpty());
    REQUIRE(masteringId.isNotEmpty());
    REQUIRE(collections.setCollectionMembership(first, firstDescription.name, mixingId, true));
    REQUIRE(collections.setCollectionMembership(first, firstDescription.name, masteringId, true));
    REQUIRE(collections.isInCollection(first, mixingId));
    REQUIRE(collections.isInCollection(first, masteringId));
    REQUIRE(collections.removeCollection(mixingId));
    REQUIRE(!collections.isInCollection(first, mixingId));
    REQUIRE(collections.isInCollection(first, masteringId));

    const auto internal = PluginIdentity::fromDescription(makeInternal("nextDelay", "Delay"));
    REQUIRE(internal.kind == PluginIdentity::Kind::internal);
    REQUIRE(internal.xmlType == "nextDelay");
    REQUIRE(collections.setFavorite(internal, "Delay", true));
    REQUIRE(collections.isFavorite(PluginIdentity::fromDescription(makeInternal("nextDelay", "Delay Renamed"))));

    REQUIRE(collections.setFavorite(first, firstDescription.name, false));
    REQUIRE(!collections.isFavorite(first));
    REQUIRE(collections.isInCollection(first, masteringId));
    REQUIRE(collections.setCollectionMembership(first, firstDescription.name, masteringId, false));
    REQUIRE(!collections.isInCollection(first, masteringId));
}

void testFavoriteOrdering()
{
    juce::ValueTree root("AppSettings");
    PluginCollections collections(root);
    const auto favoriteZ = makeExternal("VST3", "/plugins/z.vst3", 1, "Zulu");
    const auto regular = makeExternal("VST3", "/plugins/b.vst3", 2, "Beta");
    const auto favoriteA = makeExternal("VST3", "/plugins/a.vst3", 3, "alpha");
    REQUIRE(collections.setFavorite(PluginIdentity::fromDescription(favoriteZ), favoriteZ.name, true));
    REQUIRE(collections.setFavorite(PluginIdentity::fromDescription(favoriteA), favoriteA.name, true));

    juce::Array<juce::PluginDescription> plugins{favoriteZ, regular, favoriteA};
    collections.sortByFavorite(plugins, true);
    REQUIRE(plugins[0].name == "alpha");
    REQUIRE(plugins[1].name == "Zulu");
    REQUIRE(plugins[2].name == "Beta");

    collections.sortByFavorite(plugins, false);
    REQUIRE(plugins[0].name == "Beta");
    REQUIRE(plugins[1].name == "alpha");
    REQUIRE(plugins[2].name == "Zulu");
}

void testRoundTripAndDeletedDefaultsStayDeleted()
{
    juce::ValueTree root("AppSettings");
    PluginCollections collections(root);
    const auto plugin = PluginIdentity::fromDescription(makeExternal("VST3", "/plugins/synth.vst3", 77, "Synth"));
    const auto soundDesignId = findCollectionId(collections, "Sound Design");
    REQUIRE(collections.setFavorite(plugin, "Synth", true));
    REQUIRE(collections.setCollectionMembership(plugin, "Synth", soundDesignId, true));

    const auto xml = root.createXml();
    REQUIRE(xml != nullptr);
    auto restoredRoot = juce::ValueTree::fromXml(*xml);
    PluginCollections restored(restoredRoot);
    REQUIRE(restored.getCollections().size() == 5);
    REQUIRE(restored.isFavorite(plugin));
    REQUIRE(restored.isInCollection(plugin, soundDesignId));

    for (const auto &collection : restored.getCollections())
        REQUIRE(restored.removeCollection(collection.id));
    REQUIRE(restored.getCollections().isEmpty());

    PluginCollections reopened(restoredRoot);
    REQUIRE(reopened.getCollections().isEmpty());
    REQUIRE(reopened.isFavorite(plugin));
}

void testLegacyCategoryMigration()
{
    juce::ValueTree root("AppSettings");
    juce::ValueTree state("PluginCollections");
    juce::ValueTree categories("Categories");
    juce::ValueTree category("Category");
    category.setProperty("id", "legacy-mixing", nullptr);
    category.setProperty("name", "Mixing", nullptr);
    categories.addChild(category, -1, nullptr);

    juce::ValueTree plugins("Plugins");
    juce::ValueTree plugin("Plugin");
    plugin.setProperty("kind", "external", nullptr);
    plugin.setProperty("name", "Compressor", nullptr);
    plugin.setProperty("format", "VST3", nullptr);
    plugin.setProperty("file", "/plugins/example.vst3", nullptr);
    plugin.setProperty("uniqueId", 0x1234, nullptr);
    plugin.setProperty("deprecatedUid", 0, nullptr);
    juce::ValueTree categoryRef("CategoryRef");
    categoryRef.setProperty("categoryId", "legacy-mixing", nullptr);
    plugin.addChild(categoryRef, -1, nullptr);
    plugins.addChild(plugin, -1, nullptr);

    state.addChild(categories, -1, nullptr);
    state.addChild(plugins, -1, nullptr);
    root.addChild(state, -1, nullptr);

    PluginCollections collections(root);
    const auto identity = PluginIdentity::fromDescription(makeExternal("VST3", "/plugins/example.vst3", 0x1234, "Compressor"));
    REQUIRE(collections.getCollections().size() == 1);
    REQUIRE(collections.getCollectionName("legacy-mixing") == "Mixing");
    REQUIRE(collections.isInCollection(identity, "legacy-mixing"));
    REQUIRE(!collections.getState().getChildWithName("Categories").isValid());
    REQUIRE(collections.getState().getChildWithName("Collections").isValid());
    REQUIRE((int)collections.getState().getProperty("version") == 2);
}
} // namespace

int main()
{
    testDefaultsAndCollectionLifecycle();
    testIdentityAndMembership();
    testFavoriteOrdering();
    testRoundTripAndDeletedDefaultsStayDeleted();
    testLegacyCategoryMigration();

    if (failures != 0)
        std::cerr << failures << " plugin collection test(s) failed\n";

    return failures == 0 ? 0 : 1;
}

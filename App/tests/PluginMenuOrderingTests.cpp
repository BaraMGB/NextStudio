#include "PluginMenu.h"
#include "Utilities.h"

#include <iostream>
#include <vector>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

enum class TestKind
{
    group,
    entry
};

enum class TestRole
{
    midi,
    instrument,
    audio
};

class TestNode final : public PluginTreeBase
{
public:
    TestNode(juce::String displayName, TestKind kind, TestRole role = TestRole::audio)
        : m_displayName(std::move(displayName)), m_kind(kind), m_role(role)
    {
    }

    [[nodiscard]] juce::String getUniqueName() const override { return m_displayName; }
    [[nodiscard]] juce::String getDisplayName() const override { return m_displayName; }
    [[nodiscard]] TestKind getKind() const { return m_kind; }
    [[nodiscard]] TestRole getRole() const { return m_role; }

    TestNode *add(juce::String name, TestKind kind, TestRole role = TestRole::audio)
    {
        auto *node = new TestNode(std::move(name), kind, role);
        addSubItem(node);
        return node;
    }

private:
    juce::String m_displayName;
    TestKind m_kind;
    TestRole m_role;
};

std::vector<juce::String> childNames(const TestNode &parent, TestKind kind)
{
    std::vector<juce::String> result;
    for (int i = 0; i < parent.getNumSubItems(); ++i)
    {
        auto *child = dynamic_cast<TestNode *>(parent.getSubItem(i));
        if (child != nullptr && child->getKind() == kind)
            result.push_back(child->getDisplayName());
    }
    return result;
}

void appendMenuEntryNames(const TestNode &parent, std::vector<juce::String> &result, const TestRole *role)
{
    for (int i = 0; i < parent.getNumSubItems(); ++i)
    {
        auto *child = dynamic_cast<TestNode *>(parent.getSubItem(i));
        if (child != nullptr && child->getKind() == TestKind::group)
            appendMenuEntryNames(*child, result, role);
    }

    for (int i = 0; i < parent.getNumSubItems(); ++i)
    {
        auto *child = dynamic_cast<TestNode *>(parent.getSubItem(i));
        if (child != nullptr && child->getKind() == TestKind::entry && (role == nullptr || child->getRole() == *role))
            result.push_back(child->getDisplayName());
    }
}

void testCaseInsensitiveNaturalStableOrdering()
{
    TestNode root("Root", TestKind::group);
    auto *firstAlpha = root.add("Alpha", TestKind::entry);
    root.add("effect 10", TestKind::entry);
    root.add("zeta", TestKind::entry);
    auto *secondAlpha = root.add("alpha", TestKind::entry);
    root.add("Effect 2", TestKind::entry);
    root.add("beta", TestKind::entry);

    root.sortSubItemsRecursively();

    const auto names = childNames(root, TestKind::entry);
    REQUIRE(names == std::vector<juce::String>({"Alpha", "alpha", "beta", "Effect 2", "effect 10", "zeta"}));
    REQUIRE(root.getSubItem(0) == firstAlpha);
    REQUIRE(root.getSubItem(1) == secondAlpha);
}

void testCategoriesAndEntriesRemainNestedAndSorted()
{
    TestNode root("Root", TestKind::group);
    auto *vendorZ = root.add("vendor Z", TestKind::group);
    vendorZ->add("Compressor", TestKind::entry);
    vendorZ->add("chorus", TestKind::entry);
    auto *vendorA = root.add("Vendor A", TestKind::group);
    auto *category = vendorA->add("Dynamics", TestKind::group);
    category->add("Gate", TestKind::entry);
    category->add("compressor", TestKind::entry);
    root.add("Zebra", TestKind::entry);
    root.add("alpha", TestKind::entry);

    root.sortSubItemsRecursively();

    REQUIRE(childNames(root, TestKind::group) == std::vector<juce::String>({"Vendor A", "vendor Z"}));
    REQUIRE(childNames(root, TestKind::entry) == std::vector<juce::String>({"alpha", "Zebra"}));
    REQUIRE(vendorA->getNumSubItems() == 1);
    REQUIRE(vendorA->getSubItem(0) == category);
    REQUIRE(childNames(*category, TestKind::entry) == std::vector<juce::String>({"compressor", "Gate"}));
    REQUIRE(childNames(*vendorZ, TestKind::entry) == std::vector<juce::String>({"chorus", "Compressor"}));
}

void testFilteredMenusRetainUnfilteredOrder()
{
    TestNode root("Root", TestKind::group);
    auto *external = root.add("External", TestKind::group);
    external->add("Zulu MIDI", TestKind::entry, TestRole::midi);
    external->add("alpha audio", TestKind::entry, TestRole::audio);
    external->add("Beta MIDI", TestKind::entry, TestRole::midi);
    auto *builtin = root.add("builtin", TestKind::group);
    builtin->add("Synth", TestKind::entry, TestRole::instrument);
    builtin->add("Arpeggiator", TestKind::entry, TestRole::midi);

    root.sortSubItemsRecursively();

    std::vector<juce::String> unfiltered;
    appendMenuEntryNames(root, unfiltered, nullptr);
    const auto midiRole = TestRole::midi;
    std::vector<juce::String> filtered;
    appendMenuEntryNames(root, filtered, &midiRole);

    REQUIRE(unfiltered == std::vector<juce::String>({"Arpeggiator", "Synth", "alpha audio", "Beta MIDI", "Zulu MIDI"}));
    REQUIRE(filtered == std::vector<juce::String>({"Arpeggiator", "Beta MIDI", "Zulu MIDI"}));

    std::vector<juce::String> midiSubsequence;
    for (const auto &name : unfiltered)
        if (name == "Arpeggiator" || name == "Beta MIDI" || name == "Zulu MIDI")
            midiSubsequence.push_back(name);
    REQUIRE(filtered == midiSubsequence);
}

juce::PluginDescription makeDescription(const juce::String &format, const juce::String &name, const juce::String &id)
{
    juce::PluginDescription description;
    description.pluginFormatName = format;
    description.name = name;
    description.fileOrIdentifier = id;
    return description;
}

void testSidebarFormatThenNameOrdering()
{
    juce::Array<juce::PluginDescription> descriptions{
        makeDescription("VST 10", "zeta", "vst-z"),
        makeDescription("audioUnit", "Beta", "au-b"),
        makeDescription("VST 2", "alpha", "vst-a-first"),
        makeDescription("AudioUnit", "alpha", "au-a"),
        makeDescription("vst 2", "Alpha", "vst-a-second")};

    EngineHelpers::CompareFormatForward forward;
    descriptions.sort(forward, true);
    REQUIRE(descriptions[0].fileOrIdentifier == "au-a");
    REQUIRE(descriptions[1].fileOrIdentifier == "au-b");
    REQUIRE(descriptions[2].fileOrIdentifier == "vst-a-first");
    REQUIRE(descriptions[3].fileOrIdentifier == "vst-a-second");
    REQUIRE(descriptions[4].fileOrIdentifier == "vst-z");

    EngineHelpers::CompareFormatBackward backward;
    descriptions.sort(backward, true);
    REQUIRE(descriptions[0].fileOrIdentifier == "vst-z");
    REQUIRE(descriptions[1].fileOrIdentifier == "vst-a-first");
    REQUIRE(descriptions[2].fileOrIdentifier == "vst-a-second");
    REQUIRE(descriptions[3].fileOrIdentifier == "au-b");
    REQUIRE(descriptions[4].fileOrIdentifier == "au-a");
}

} // namespace

int main()
{
    testCaseInsensitiveNaturalStableOrdering();
    testCategoriesAndEntriesRemainNestedAndSorted();
    testFilteredMenusRetainUnfilteredOrder();
    testSidebarFormatThenNameOrdering();

    if (failures != 0)
        std::cerr << failures << " plugin menu ordering test(s) failed\n";

    return failures == 0 ? 0 : 1;
}

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <functional>
#include <unordered_set>
#include <unordered_map>

// Identity remains operator==. These indices are local to a read-only update,
// never retained across property/model mutations.
template <typename Item>
struct SelectionIdentityHash : std::hash<Item> {};

template <>
struct SelectionIdentityHash<juce::ValueTree>
{
    size_t operator()(const juce::ValueTree& tree) const
    {
        // JUCE exposes no SharedObject hash. A property storage address is a
        // shared, unique anchor during a read-only pass, even for 20,000 notes
        // with identical contents. Rebuild after mutation: property arrays can
        // relocate. The pointer is hashed, never dereferenced or persisted.
        if (tree.getNumProperties() != 0)
            return std::hash<const juce::var*>{}(tree.getPropertyPointer(tree.getPropertyName(0)));
        // Propertyless utility trees have no such anchor. Equality still keeps
        // them distinct, although they share a bucket; selectable model leaves
        // (notes, curve points, clips, tracks) all have properties.
        return size_t(tree.getType().toString().hashCode64());
    }
};

template <typename Item>
using SelectionIdentitySet = std::unordered_set<Item, SelectionIdentityHash<Item>>;
using SelectionTreeSet = SelectionIdentitySet<juce::ValueTree>;
using SelectionTreeIndex = std::unordered_map<juce::ValueTree, int, SelectionIdentityHash<juce::ValueTree>>;
inline SelectionTreeIndex indexSelectionChildren(const juce::ValueTree& parent)
{
    SelectionTreeIndex result;
    result.reserve(size_t(parent.getNumChildren()));
    int index = 0;
    for (const auto& child : parent) result.emplace(child, index++);
    return result;
}

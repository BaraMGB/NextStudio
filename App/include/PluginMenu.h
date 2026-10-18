
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginCollections.h"

#include <functional>
#include <map>
#include <vector>

namespace te = tracktion_engine;

static inline const char *getInternalPluginFormatName() { return "TracktionInternal"; }

//==============================================================================
class PluginTreeBase
{
public:
    virtual ~PluginTreeBase() = default;
    [[nodiscard]] virtual juce::String getUniqueName() const = 0;
    [[nodiscard]] virtual juce::String getDisplayName() const = 0;

    void addSubItem(PluginTreeBase *itm) { subitems.add(itm); }
    int getNumSubItems() const { return subitems.size(); }
    PluginTreeBase *getSubItem(int idx) const { return subitems[idx]; }

    void sortSubItemsRecursively()
    {
        for (auto *item : subitems)
            item->sortSubItemsRecursively();

        struct DisplayNameComparator
        {
            static int compareElements(const PluginTreeBase *first, const PluginTreeBase *second) { return first->getDisplayName().compareNatural(second->getDisplayName(), false); }
        } comparator;

        subitems.sort(comparator, true);
    }

private:
    juce::OwnedArray<PluginTreeBase> subitems;
};

//==============================================================================
class PluginTreeItem : public PluginTreeBase
{
public:
    explicit PluginTreeItem(juce::PluginDescription);
    PluginTreeItem(const juce::String &uniqueId, const juce::String &name, juce::String xmlType, bool isSynth, bool isPlugin);

    te::Plugin::Ptr create(te::Edit &) const;

    [[nodiscard]] juce::String getUniqueName() const override
    {
        if (desc.fileOrIdentifier.startsWith(te::RackType::getRackPresetPrefix()))
            return desc.fileOrIdentifier;

        return desc.createIdentifierString();
    }

    [[nodiscard]] juce::String getDisplayName() const override { return desc.name; }

    juce::PluginDescription desc;
    juce::String xmlType;
    bool isPlugin = true;

    JUCE_LEAK_DETECTOR(PluginTreeItem)
};

//==============================================================================
class PluginTreeGroup : public PluginTreeBase
{
public:
    PluginTreeGroup(te::Edit &, juce::KnownPluginList::PluginTree &, te::Plugin::Type);
    explicit PluginTreeGroup(juce::String);

    [[nodiscard]] juce::String getUniqueName() const override { return name; }
    [[nodiscard]] juce::String getDisplayName() const override { return name; }

    juce::String name;

private:
    void populateFrom(juce::KnownPluginList::PluginTree &);
    void createBuiltInItems(int &num, te::Plugin::Type);

    JUCE_LEAK_DETECTOR(PluginTreeGroup)
};

template <class FilterClass> void addInternalPlugin(PluginTreeBase &item, int &num, bool synth = false) { item.addSubItem(new PluginTreeItem(juce::String(num++) + "_trkbuiltin", TRANS(FilterClass::getPluginName()), FilterClass::xmlTypeName, synth, false)); }

//==============================================================================
class PluginMenu : public juce::PopupMenu
{
public:
    using Filter = std::function<bool(const PluginTreeItem &)>;

    PluginMenu() = default;
    explicit PluginMenu(PluginTreeGroup &, PluginCollections *collections = nullptr, Filter filter = {});

    PluginTreeItem *runMenu(PluginTreeGroup &, juce::Component *target = nullptr);

private:
    bool appendStandardItems(PluginTreeGroup &, juce::PopupMenu &);
    bool appendCuratedItems(PluginTreeGroup &, PluginCollections &, juce::PopupMenu &);
    void collectMatchingItems(PluginTreeGroup &, std::vector<PluginTreeItem *> &) const;
    void addPluginItem(juce::PopupMenu &, PluginTreeItem &);
    [[nodiscard]] bool matches(const PluginTreeItem &) const;

    Filter m_filter;
    int m_nextItemId{1};
    std::map<int, PluginTreeItem *> m_itemLookup;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginMenu)
};

te::Plugin::Ptr showMenuAndCreatePlugin(te::Edit &);

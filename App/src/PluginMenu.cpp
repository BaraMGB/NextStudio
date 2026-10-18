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

#include "PluginMenu.h"

#include "ArpeggiatorPlugin.h"
#include "NextChorusPlugin.h"
#include "NextDelayPlugin.h"
#include "NextFilterPlugin.h"
#include "NextPhaserPlugin.h"
#include "NextSaturationPlugin.h"
#include "PeakLimiterPlugin.h"
#include "SimpleSynthPlugin.h"
#include "SoundFontPlugin.h"
#include "SpectrumAnalyzerPlugin.h"
#include "Utilities.h"
#include <algorithm>
#include <utility>

//==============================================================================
PluginTreeItem::PluginTreeItem(juce::PluginDescription d)
    : desc(std::move(d)),
      xmlType(te::ExternalPlugin::xmlTypeName),
      isPlugin(true)
{
    jassert(xmlType.isNotEmpty());
}

PluginTreeItem::PluginTreeItem(const juce::String &uniqueId, const juce::String &name, juce::String xmlType_, bool isSynth, bool isPlugin_)
    : xmlType(std::move(xmlType_)),
      isPlugin(isPlugin_)
{
    jassert(xmlType.isNotEmpty());
    desc.name = name;
    desc.fileOrIdentifier = uniqueId;
    desc.pluginFormatName = (uniqueId.endsWith("_trkbuiltin") || xmlType == te::RackInstance::xmlTypeName) ? getInternalPluginFormatName() : juce::String();
    desc.category = xmlType;
    desc.isInstrument = isSynth;
}

te::Plugin::Ptr PluginTreeItem::create(te::Edit &ed) const { return ed.getPluginCache().createNewPlugin(xmlType, desc); }

//==============================================================================
PluginTreeGroup::PluginTreeGroup(te::Edit &edit, juce::KnownPluginList::PluginTree &tree, te::Plugin::Type types)
    : name("Plugins")
{
    {
        int num = 1;

        auto builtinFolder = new PluginTreeGroup(TRANS("Builtin Plugins"));
        addSubItem(builtinFolder);
        builtinFolder->createBuiltInItems(num, types);
    }

    {
        /*        auto racksFolder = new PluginTreeGroup (TRANS("Plugin Racks"));
                addSubItem (racksFolder);

                racksFolder->addSubItem (new PluginTreeItem (juce::String (te::RackType::getRackPresetPrefix()) + "-1",
                                                             TRANS("Create New Empty Rack"),
                                                             te::RackInstance::xmlTypeName, false, false));

                int i = 0;
                for (auto rf : edit.getRackList().getTypes())
                    racksFolder->addSubItem (new PluginTreeItem ("RACK__" + juce::String (i++), rf->rackName,
                                                                 te::RackInstance::xmlTypeName, false, false));
        */
    }

    populateFrom(tree);
    sortSubItemsRecursively();
}

PluginTreeGroup::PluginTreeGroup(juce::String s)
    : name(std::move(s))
{
    jassert(name.isNotEmpty());
}

void PluginTreeGroup::populateFrom(juce::KnownPluginList::PluginTree &tree)
{
    for (auto subTree : tree.subFolders)
    {
        if (subTree->plugins.size() > 0 || subTree->subFolders.size() > 0)
        {
            auto fs = new PluginTreeGroup(subTree->folder);
            addSubItem(fs);

            fs->populateFrom(*subTree);
        }
    }

    for (const auto &pd : tree.plugins)
        addSubItem(new PluginTreeItem(pd));
}

void PluginTreeGroup::createBuiltInItems(int &num, te::Plugin::Type types)
{
    addInternalPlugin<te::VolumeAndPanPlugin>(*this, num);
    //    addInternalPlugin<te::LevelMeterPlugin> (*this, num);
    addInternalPlugin<te::EqualiserPlugin>(*this, num);
    addInternalPlugin<te::ReverbPlugin>(*this, num);
    addInternalPlugin<PeakLimiterPlugin>(*this, num);
    addInternalPlugin<NextDelayPlugin>(*this, num);
    addInternalPlugin<NextChorusPlugin>(*this, num);
    addInternalPlugin<NextPhaserPlugin>(*this, num);
    addInternalPlugin<NextSaturationPlugin>(*this, num);
    addInternalPlugin<te::CompressorPlugin>(*this, num);
    addInternalPlugin<te::PitchShiftPlugin>(*this, num);
    addInternalPlugin<ArpeggiatorPlugin>(*this, num, false);
    addInternalPlugin<SpectrumAnalyzerPlugin>(*this, num, false);
    addInternalPlugin<NextFilterPlugin>(*this, num);
    //    addInternalPlugin<te::MidiModifierPlugin> (*this, num);
    //    addInternalPlugin<te::MidiPatchBayPlugin> (*this, num);
    //    addInternalPlugin<te::PatchBayPlugin> (*this, num);
    //    addInternalPlugin<te::AuxSendPlugin> (*this, num);
    //    addInternalPlugin<te::AuxReturnPlugin> (*this, num);
    //    addInternalPlugin<te::TextPlugin> (*this, num);
    //    addInternalPlugin<te::FreezePointPlugin> (*this, num);

#if TRACKTION_ENABLE_REWIRE
    addInternalPlugin<te::ReWirePlugin>(*this, num, true);
#endif

    if (types == te::Plugin::Type::allPlugins)
    {
        addInternalPlugin<te::SamplerPlugin>(*this, num, true);
        addInternalPlugin<SoundFontPlugin>(*this, num, true);
        addInternalPlugin<SimpleSynthPlugin>(*this, num, true);
    }

    //    addInternalPlugin<te::InsertPlugin> (*this, num);

#if ENABLE_INTERNAL_PLUGINS
    for (auto &d : PluginTypeBase::getAllPluginDescriptions())
        if (isPluginAuthorised(d))
            addSubItem(new PluginTreeItem(d));
#endif
}

PluginMenu::PluginMenu(PluginTreeGroup &root, PluginCollections *collections, Filter filter)
    : m_filter(std::move(filter))
{
    const bool hasCuratedItems = collections != nullptr && appendCuratedItems(root, *collections, *this);
    if (hasCuratedItems)
        addSeparator();
    appendStandardItems(root, *this);
}

bool PluginMenu::matches(const PluginTreeItem &item) const { return !m_filter || m_filter(item); }

void PluginMenu::addPluginItem(juce::PopupMenu &menu, PluginTreeItem &item)
{
    menu.addItem(m_nextItemId, item.desc.name, true, false);
    m_itemLookup[m_nextItemId] = &item;
    ++m_nextItemId;
}

bool PluginMenu::appendStandardItems(PluginTreeGroup &group, juce::PopupMenu &menu)
{
    bool hasAny = false;
    for (int i = 0; i < group.getNumSubItems(); ++i)
    {
        if (auto *subGroup = dynamic_cast<PluginTreeGroup *>(group.getSubItem(i)))
        {
            juce::PopupMenu subMenu;
            if (appendStandardItems(*subGroup, subMenu))
            {
                menu.addSubMenu(subGroup->name, subMenu, true);
                hasAny = true;
            }
        }
    }

    for (int i = 0; i < group.getNumSubItems(); ++i)
    {
        if (auto *item = dynamic_cast<PluginTreeItem *>(group.getSubItem(i)); item != nullptr && matches(*item))
        {
            addPluginItem(menu, *item);
            hasAny = true;
        }
    }
    return hasAny;
}

void PluginMenu::collectMatchingItems(PluginTreeGroup &group, std::vector<PluginTreeItem *> &items) const
{
    for (int i = 0; i < group.getNumSubItems(); ++i)
        if (auto *subGroup = dynamic_cast<PluginTreeGroup *>(group.getSubItem(i)))
            collectMatchingItems(*subGroup, items);

    for (int i = 0; i < group.getNumSubItems(); ++i)
        if (auto *item = dynamic_cast<PluginTreeItem *>(group.getSubItem(i)); item != nullptr && matches(*item))
            items.push_back(item);
}

bool PluginMenu::appendCuratedItems(PluginTreeGroup &root, PluginCollections &collections, juce::PopupMenu &menu)
{
    std::vector<PluginTreeItem *> items;
    collectMatchingItems(root, items);
    std::stable_sort(items.begin(), items.end(), [](const auto *first, const auto *second) { return first->desc.name.compareNatural(second->desc.name, false) < 0; });

    bool hasAny = false;
    juce::PopupMenu favorites;
    for (auto *item : items)
        if (collections.isFavorite(PluginIdentity::fromDescription(item->desc, item->xmlType)))
            addPluginItem(favorites, *item);

    if (favorites.getNumItems() > 0)
    {
        menu.addSubMenu(TRANS("Favorites"), favorites, true);
        hasAny = true;
    }

    juce::PopupMenu collectionRoot;
    for (const auto &collection : collections.getCollections())
    {
        juce::PopupMenu collectionMenu;
        for (auto *item : items)
            if (collections.isInCollection(PluginIdentity::fromDescription(item->desc, item->xmlType), collection.id))
                addPluginItem(collectionMenu, *item);

        if (collectionMenu.getNumItems() > 0)
            collectionRoot.addSubMenu(collection.name, collectionMenu, true);
    }

    if (collectionRoot.getNumItems() > 0)
    {
        menu.addSubMenu(TRANS("My Collections"), collectionRoot, true);
        hasAny = true;
    }

    return hasAny;
}

PluginTreeItem *PluginMenu::runMenu(PluginTreeGroup &, juce::Component *target)
{
    const int result = target != nullptr ? showAt(target) : show();
    const auto found = m_itemLookup.find(result);
    return found != m_itemLookup.end() ? found->second : nullptr;
}

te::Plugin::Ptr showMenuAndCreatePlugin(te::Edit &edit)
{
    if (auto tree = EngineHelpers::createPluginTree(edit.engine))
    {
        PluginTreeGroup root(edit, *tree, te::Plugin::Type::allPlugins);
        PluginMenu m(root);

        if (auto type = m.runMenu(root))
            return type->create(edit);
    }

    return {};
}

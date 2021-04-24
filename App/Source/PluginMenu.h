#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

static inline const char* getInternalPluginFormatName()     { return "TracktionInternal"; }

//==============================================================================
class PluginTreeBase
{
public:
    virtual ~PluginTreeBase() = default;
    virtual juce::String getUniqueName() const = 0;

    void addSubItem (PluginTreeBase* itm)   { subitems.add (itm);       }
    int getNumSubItems()                    { return subitems.size();   }
    PluginTreeBase* getSubItem (int idx)    { return subitems[idx];     }

private:
    juce::OwnedArray<PluginTreeBase> subitems;
};

//==============================================================================
class PluginTreeItem : public PluginTreeBase
{
public:
    PluginTreeItem (const juce::PluginDescription&);
    PluginTreeItem (const juce::String& uniqueId
                    , const juce::String& name
                    , const juce::String& xmlType
                    , bool isSynth
                    , bool isPlugin);

    te::Plugin::Ptr create (te::Edit&);

    juce::String getUniqueName() const override
    {
        if (desc.fileOrIdentifier.startsWith (te::RackType::getRackPresetPrefix()))
            return desc.fileOrIdentifier;

        return desc.createIdentifierString();
    }

    juce::PluginDescription desc;
    juce::String xmlType;
    bool isPlugin = true;

    JUCE_LEAK_DETECTOR (PluginTreeItem)
};

//==============================================================================
class PluginTreeGroup : public PluginTreeBase
{
public:
    PluginTreeGroup (te::Edit&, juce::KnownPluginList::PluginTree&, te::Plugin::Type);
    PluginTreeGroup (const juce::String&);

    juce::String getUniqueName() const override           { return name; }

    juce::String name;

private:
    void populateFrom (juce::KnownPluginList::PluginTree&);
    void createBuiltInItems (int& num, te::Plugin::Type);

    JUCE_LEAK_DETECTOR (PluginTreeGroup)
};


template<class FilterClass>
void addInternalPlugin (PluginTreeBase& item, int& num, bool synth = false)
{
    item.addSubItem (new PluginTreeItem (juce::String (num++) + "_trkbuiltin",
                                         TRANS (FilterClass::getPluginName()),
                                         FilterClass::xmlTypeName, synth, false));
}




//==============================================================================
class PluginMenu : public juce::PopupMenu
{
public:
    PluginMenu() = default;

    PluginMenu (PluginTreeGroup& node)
    {
        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subNode = dynamic_cast<PluginTreeGroup*> (node.getSubItem (i)))
                addSubMenu (subNode->name, PluginMenu (*subNode), true);

        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subType = dynamic_cast<PluginTreeItem*> (node.getSubItem (i)))
                addItem (subType->getUniqueName().hashCode(), subType->desc.name, true, false);
    }

    static PluginTreeItem* findType (PluginTreeGroup& node, int hash)
    {
        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto subNode = dynamic_cast<PluginTreeGroup*> (node.getSubItem (i)))
                if (auto* t = findType (*subNode, hash))
                    return t;

        for (int i = 0; i < node.getNumSubItems(); ++i)
            if (auto t = dynamic_cast<PluginTreeItem*> (node.getSubItem (i)))
                if (t->getUniqueName().hashCode() == hash)
                    return t;

        return nullptr;
    }

    PluginTreeItem* runMenu (PluginTreeGroup& node)
    {
        int res = show();

        if (res == 0)
            return nullptr;

        return findType (node, res);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginMenu)
};

te::Plugin::Ptr showMenuAndCreatePlugin (te::Edit&);




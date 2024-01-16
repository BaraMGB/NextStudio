/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "PluginBrowser.h"
#include "SearchFieldComponent.h"


class InstrumentEffectListModel : public juce::TableListBoxModel
                             , public juce::ChangeBroadcaster
{
public:
    enum column
    {
        typeCol = 1,
        nameCol = 2,
    };

    InstrumentEffectListModel(te::Engine& engine, bool isInstrumentList);

    void paintRowBackground(juce::Graphics& g, int row, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int row, int col, int width, int height, bool rowIsSelected) override;
    int  getNumRows() override;
    void sortOrderChanged (int newSortColumnId, bool isForwards) override;

    void updatePluginLists();
    juce::Array<juce::PluginDescription>& getPluginList() 
    {
        if (m_isInstrumentList)
            return m_instruments;
        else
            return m_effects;
    }

    void changeSearchTerm(juce::String searchTerm)
    {
        m_searchTerm = searchTerm;
        updatePluginLists();
    }

private:

    void filterList()
    {
        juce::Array<juce::PluginDescription>& list = m_isInstrumentList ? m_instruments : m_effects;
        juce::Array<juce::PluginDescription> filteredList;

        for (const auto& plugin : list)
            if (plugin.name.containsIgnoreCase(m_searchTerm))
                filteredList.add(plugin);

        list = filteredList;
    }

    juce::KnownPluginList& m_knownPlugins;
    te::Engine& m_engine;
    juce::Array<juce::PluginDescription> m_instruments;
    juce::Array<juce::PluginDescription> m_effects;
    bool m_isInstrumentList;
    std::tuple<column, bool> m_order;

    juce::String m_searchTerm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentEffectListModel)
};

//----------------------------------------------------------------------------------------------------

class InstrumentEffectTable : public juce::TableListBox
{
public:
    InstrumentEffectTable(te::Engine& engine, InstrumentEffectListModel& model);

    juce::var getDragSourceDescription(const juce::SparseSet<int>& /*rowsToDescribe*/) override;

    te::Plugin::Ptr getSelectedPlugin(te::Edit& edit);
    juce::Array<juce::PluginDescription>& getPluginList()
    {
        return m_model.getPluginList();
    }

private:
    te::Engine& m_engine;
    InstrumentEffectListModel& m_model;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentEffectTable)
};

//----------------------------------------------------------------------------------------------------


class InstrumentEffectChooser : public juce::Component 
                     , public juce::ChangeListener
{
public:
    InstrumentEffectChooser(te::Engine& engine, bool isInstrumentList);
    ~InstrumentEffectChooser() override
    {
        m_searchField.removeChangeListener(this);
        m_model.removeChangeListener(this);
    }

    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;


private:
    InstrumentEffectListModel m_model;
    InstrumentEffectTable m_listbox;
    SearchFieldComponent m_searchField;
    te::Engine&                           m_engine;
    bool m_isInstrumentList;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentEffectChooser)
};

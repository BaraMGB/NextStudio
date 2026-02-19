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
#include "SideBrowser/PluginBrowser.h"
#include "SideBrowser/SearchFieldComponent.h"
#include "Utilities/ApplicationViewState.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

class InstrumentEffectListModel
    : public juce::TableListBoxModel
    , public juce::ChangeBroadcaster
{
public:
    enum column
    {
        typeCol = 1,
        nameCol = 2,
    };

    InstrumentEffectListModel(te::Engine &engine, bool isInstrumentList, ApplicationViewState &appState);

    void paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics &g, int row, int col, int width, int height, bool rowIsSelected) override;
    int getNumRows() override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;

    juce::var getDragSourceDescription(const juce::SparseSet<int> & /*rowsToDescribe*/) override;
    void updatePluginLists();
    juce::Array<juce::PluginDescription> &getPluginList()
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
        juce::Array<juce::PluginDescription> &list = m_isInstrumentList ? m_instruments : m_effects;
        juce::Array<juce::PluginDescription> filteredList;

        for (const auto &plugin : list)
            if (plugin.name.containsIgnoreCase(m_searchTerm))
                filteredList.add(plugin);

        list = filteredList;
    }

    juce::KnownPluginList &m_knownPlugins;
    te::Engine &m_engine;
    ApplicationViewState &m_appState;
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
    InstrumentEffectTable(te::Engine &engine, InstrumentEffectListModel &model, ApplicationViewState &appState);

    te::Plugin::Ptr getSelectedPlugin(te::Edit &edit);
    juce::Array<juce::PluginDescription> &getPluginList() { return m_model.getPluginList(); }

private:
    te::Engine &m_engine;
    InstrumentEffectListModel &m_model;
    ApplicationViewState &m_appState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentEffectTable)
};

//----------------------------------------------------------------------------------------------------

class InstrumentEffectChooser
    : public juce::Component
    , public juce::ChangeListener
{
public:
    InstrumentEffectChooser(te::Engine &engine, bool isInstrumentList, ApplicationViewState &appState);
    ~InstrumentEffectChooser() override
    {
        m_searchField.removeChangeListener(this);
        m_model.removeChangeListener(this);
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(m_appState.getBackgroundColour2());
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(m_searchField.getY(), 0, getWidth());
    }

    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
    InstrumentEffectListModel m_model;
    InstrumentEffectTable m_listbox;
    SearchFieldComponent m_searchField;
    te::Engine &m_engine;
    bool m_isInstrumentList;
    ApplicationViewState &m_appState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentEffectChooser)
};

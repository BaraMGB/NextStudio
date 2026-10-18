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
#include "ApplicationViewState.h"
#include "EditViewState.h"
#include "PluginBrowser.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

#include <map>

class InstrumentEffectListModel
    : public juce::TableListBoxModel
    , public juce::ChangeBroadcaster
    , public juce::ChangeListener
{
public:
    enum column
    {
        typeCol = 1,
        nameCol = 2,
        favoriteCol = 3,
    };

    InstrumentEffectListModel(te::Engine &engine, bool isInstrumentList, ApplicationViewState &appState);
    ~InstrumentEffectListModel() override;

    void paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics &g, int row, int col, int width, int height, bool rowIsSelected) override;
    int getNumRows() override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent &) override;
    void selectedRowsChanged(int lastRowSelected) override;

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

    void changeCollectionFilter(juce::String collectionId)
    {
        m_collectionFilter = std::move(collectionId);
        updatePluginLists();
    }

    [[nodiscard]] const juce::String &getCollectionFilter() const { return m_collectionFilter; }
    [[nodiscard]] const juce::PluginDescription *getPluginAtRow(int row) const;
    [[nodiscard]] const juce::PluginDescription *getSelectedPlugin() const { return m_hasSelectedPlugin ? &m_selectedPlugin : nullptr; }
    int refreshSelectedPluginRow();
    std::function<void(const juce::PluginDescription &, juce::Component *)> onPluginContextRequested;

    static juce::String favoritesFilterId() { return "__favorites"; }

private:
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void filterList()
    {
        juce::Array<juce::PluginDescription> &list = m_isInstrumentList ? m_instruments : m_effects;
        juce::Array<juce::PluginDescription> filteredList;
        auto &collections = m_appState.getPluginCollections();

        for (const auto &plugin : list)
        {
            const auto identity = PluginIdentity::fromDescription(plugin, plugin.category);
            const bool matchesCollection = m_collectionFilter.isEmpty() || (m_collectionFilter == favoritesFilterId() && collections.isFavorite(identity)) || (m_collectionFilter != favoritesFilterId() && collections.isInCollection(identity, m_collectionFilter));

            if (matchesCollection && plugin.name.containsIgnoreCase(m_searchTerm))
                filteredList.add(plugin);
        }

        list = filteredList;
    }

    juce::KnownPluginList &m_knownPlugins;
    te::Engine &m_engine;
    ApplicationViewState &m_appState;
    juce::Array<juce::PluginDescription> m_instruments;
    juce::Array<juce::PluginDescription> m_effects;
    bool m_isInstrumentList;
    std::tuple<column, bool> m_order{typeCol, true};

    juce::String m_searchTerm;
    juce::String m_collectionFilter;
    juce::PluginDescription m_selectedPlugin;
    bool m_hasSelectedPlugin{false};

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
        g.drawHorizontalLine(m_allPluginsButton.getY() - 2, 0, getWidth());
        g.drawHorizontalLine(m_searchField.getY(), 0, getWidth());
    }

    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
    void refreshCollectionControls();
    void showPluginContextMenu(const juce::PluginDescription &, juce::Component *target);
    void showCollectionManagementMenu();
    void showCollectionNameDialog(const juce::String &title, const juce::String &initialName, std::function<void(const juce::String &)> completion, const juce::String &exceptCollectionId = {});
    void confirmCollectionDeletion(const juce::String &collectionId);

    InstrumentEffectListModel m_model;
    InstrumentEffectTable m_listbox;
    SearchFieldComponent m_searchField;
    juce::TextButton m_allPluginsButton{"All"};
    juce::TextButton m_favoritesButton{juce::String::fromUTF8("\xe2\x98\x85")};
    juce::ComboBox m_collectionSelector;
    juce::TextButton m_manageCollectionsButton{"..."};
    std::map<int, juce::String> m_collectionIds;
    te::Engine &m_engine;
    bool m_isInstrumentList;
    ApplicationViewState &m_appState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentEffectChooser)
};

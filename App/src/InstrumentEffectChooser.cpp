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

#include "InstrumentEffectChooser.h"
#include "ApplicationViewState.h"
#include "PluginMenu.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

InstrumentEffectListModel::InstrumentEffectListModel(tracktion::Engine &engine, bool isInstrumentList, ApplicationViewState &appState)
    : m_engine(engine),
      m_knownPlugins(engine.getPluginManager().knownPluginList),
      m_isInstrumentList(isInstrumentList),
      m_appState(appState)
{
    m_knownPlugins.addChangeListener(this);
    m_appState.getPluginCollections().addChangeListener(this);
    updatePluginLists();
}

InstrumentEffectListModel::~InstrumentEffectListModel()
{
    m_appState.getPluginCollections().removeChangeListener(this);
    m_knownPlugins.removeChangeListener(this);
}

void InstrumentEffectListModel::updatePluginLists()
{
    m_instruments.clear();
    m_effects.clear();

    for (auto &desc : m_knownPlugins.getTypes())
    {
        if (desc.isInstrument)
            m_instruments.add(desc);
        else
            m_effects.add(desc);
    }

    for (auto &desc : EngineHelpers::getInternalPlugins())
    {
        if (desc.isInstrument)
            m_instruments.add(desc);
        else
            m_effects.add(desc);
    }

    filterList();

    if (std::get<column>(m_order) == nameCol)
    {
        EngineHelpers::sortByName(m_instruments, std::get<bool>(m_order));
        EngineHelpers::sortByName(m_effects, std::get<bool>(m_order));
    }
    else if (std::get<column>(m_order) == typeCol)
    {
        EngineHelpers::sortByFormatName(m_instruments, std::get<bool>(m_order));
        EngineHelpers::sortByFormatName(m_effects, std::get<bool>(m_order));
    }
    else if (std::get<column>(m_order) == favoriteCol)
    {
        m_appState.getPluginCollections().sortByFavorite(m_instruments, std::get<bool>(m_order));
        m_appState.getPluginCollections().sortByFavorite(m_effects, std::get<bool>(m_order));
    }

    sendChangeMessage();
}

void InstrumentEffectListModel::paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected)
{
    if (row < 0 || row >= getNumRows())
        return;
    auto bgColour = row % 2 == 0 ? m_appState.getBackgroundColour2() : m_appState.getBackgroundColour2().brighter(0.05f);
    juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(bgColour);
    g.fillRect(bounds);

    if (rowIsSelected)
    {
        g.setColour(m_appState.getPrimeColour());
        g.fillRect(bounds);
    }
}

void InstrumentEffectListModel::paintCell(juce::Graphics &g, int row, int col, int width, int height, bool rowIsSelected)
{
    if (row < 0 || row >= getNumRows())
        return;

    juce::PluginDescription desc = m_isInstrumentList ? m_instruments[row] : m_effects[row];
    juce::String text;
    bool isBlacklisted = m_knownPlugins.getBlacklistedFiles().contains(desc.fileOrIdentifier);

    if (col == favoriteCol)
    {
        const auto identity = PluginIdentity::fromDescription(desc, desc.category);
        const bool favorite = m_appState.getPluginCollections().isFavorite(identity);
        g.setColour(rowIsSelected ? m_appState.getPrimeColour().contrasting(.7f) : favorite ? m_appState.getPrimeColour() : m_appState.getTextColour().withAlpha(0.45f));
        g.setFont(juce::Font((float)height * 0.75f));
        g.drawText(favorite ? juce::String::fromUTF8("\xe2\x98\x85") : juce::String::fromUTF8("\xe2\x98\x86"), 0, 0, width, height, juce::Justification::centred);
        g.setColour(m_appState.getBorderColour().withAlpha(0.3f));
        g.drawHorizontalLine(height - 1, 0, width);
        g.drawVerticalLine(width - 1, 0, height);
        return;
    }

    if (isBlacklisted)
    {
        if (col == nameCol)
            text = desc.fileOrIdentifier;
        else if (col == typeCol)
            text = TRANS("Deactivated after failing to initialise correctly");
    }
    else
    {
        switch (col)
        {
        case typeCol:
            text = desc.pluginFormatName;
            break;
        case nameCol:
            text = desc.name;
            break;
        default:
            jassertfalse;
            break;
        }
    }

    if (text.isNotEmpty())
    {
        if (col == typeCol)
        {
            juce::Drawable *icon = nullptr;
            if (desc.pluginFormatName == "VST3")
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::vst3Icon_svg, juce::Colours::yellowgreen).release();
            else if (desc.pluginFormatName == "LADSPA")
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::ladspaIcon_svg, juce::Colours::coral).release();
            else if (desc.pluginFormatName == "LA2")
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::la2Icon_svg, juce::Colours::cornflowerblue).release();
            else if (desc.pluginFormatName == "AudioUnit")
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::AUIcon_svg, juce::Colours::wheat).release();
            else if (desc.pluginFormatName == getInternalPluginFormatName())
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::INTIcon_svg, juce::Colours::gold).release();

            if (icon != nullptr)
            {
                auto padding = 5.f;
                icon->setTransformToFit({static_cast<float>(width) / 4 - padding, padding, (static_cast<float>(width) / 2.f) + padding, static_cast<float>(height - (padding * 2))}, juce::RectanglePlacement::centred);
                icon->draw(g, 1.0f);
                delete icon;
            }
        }
        else
        {
            juce::String preTerm, postTerm;
            int termStartIndex = text.indexOfIgnoreCase(m_searchTerm);
            juce::String searchTerm = text.substring(termStartIndex, termStartIndex + m_searchTerm.length());

            if (termStartIndex != -1 && m_searchTerm.length() > 0)
            {
                preTerm = text.substring(0, termStartIndex);
                postTerm = text.substring(termStartIndex + m_searchTerm.length());
                auto colour = rowIsSelected ? m_appState.getPrimeColour().contrasting(.7f) : m_appState.getTextColour();

                g.setColour(colour);
                g.setFont(juce::Font((float)height * 0.7f, juce::Font::bold));
                g.drawFittedText(preTerm, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);

                int preTermWidth = g.getCurrentFont().getStringWidth(preTerm);

                g.setColour(juce::Colours::coral);
                g.drawFittedText(searchTerm, 4 + preTermWidth, 0, width - 6 - preTermWidth, height, juce::Justification::centredLeft, 1, 0.9f);

                int termWidth = g.getCurrentFont().getStringWidth(searchTerm);

                g.setColour(colour);
                g.drawFittedText(postTerm, 4 + preTermWidth + termWidth, 0, width - 6 - preTermWidth - termWidth, height, juce::Justification::centredLeft, 1, 0.9f);
            }
            else
            {
                auto colour = rowIsSelected ? m_appState.getPrimeColour().contrasting(.7f) : m_appState.getTextColour();

                g.setColour(colour);
                g.setFont(juce::Font((float)height * 0.7f, juce::Font::bold));
                g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
            }
        }
    }

    g.setColour(m_appState.getBorderColour().withAlpha(0.3f));
    g.drawHorizontalLine(height - 1, 0, width);
    g.drawVerticalLine(width - 1, 0, height);
}

int InstrumentEffectListModel::getNumRows() { return m_isInstrumentList ? m_instruments.size() : m_effects.size(); }

void InstrumentEffectListModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    auto &desc = m_isInstrumentList ? m_instruments : m_effects;
    switch (newSortColumnId)
    {
    case typeCol:
        EngineHelpers::sortByFormatName(desc, isForwards);
        m_order = {typeCol, isForwards};
        sendChangeMessage();
        break;

    case nameCol:
        EngineHelpers::sortByName(desc, isForwards);
        m_order = {nameCol, isForwards};
        sendChangeMessage();
        break;

    case favoriteCol:
        m_appState.getPluginCollections().sortByFavorite(desc, isForwards);
        m_order = {favoriteCol, isForwards};
        sendChangeMessage();
        break;

    default:
        jassertfalse;
        break;
    }
}

void InstrumentEffectListModel::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_knownPlugins || source == &m_appState.getPluginCollections())
        updatePluginLists();
}

const juce::PluginDescription *InstrumentEffectListModel::getPluginAtRow(int row) const
{
    const auto &list = m_isInstrumentList ? m_instruments : m_effects;
    return juce::isPositiveAndBelow(row, list.size()) ? &list.getReference(row) : nullptr;
}

void InstrumentEffectListModel::selectedRowsChanged(int lastRowSelected)
{
    if (const auto *plugin = getPluginAtRow(lastRowSelected))
    {
        m_selectedPlugin = *plugin;
        m_hasSelectedPlugin = true;
    }
    else
    {
        m_hasSelectedPlugin = false;
    }
}

int InstrumentEffectListModel::refreshSelectedPluginRow()
{
    if (!m_hasSelectedPlugin)
        return -1;

    const auto selectedIdentity = PluginIdentity::fromDescription(m_selectedPlugin, m_selectedPlugin.category);
    const auto &list = m_isInstrumentList ? m_instruments : m_effects;
    for (int row = 0; row < list.size(); ++row)
    {
        const auto &plugin = list.getReference(row);
        if (PluginIdentity::fromDescription(plugin, plugin.category) == selectedIdentity)
        {
            m_selectedPlugin = plugin;
            return row;
        }
    }

    return -1;
}

void InstrumentEffectListModel::cellClicked(int rowNumber, int columnId, const juce::MouseEvent &event)
{
    const auto *plugin = getPluginAtRow(rowNumber);
    if (plugin == nullptr)
        return;

    if (event.mods.isPopupMenu())
    {
        if (onPluginContextRequested)
            onPluginContextRequested(*plugin, event.eventComponent);
        return;
    }

    if (columnId == favoriteCol)
    {
        auto &collections = m_appState.getPluginCollections();
        const auto identity = PluginIdentity::fromDescription(*plugin, plugin->category);
        collections.setFavorite(identity, plugin->name, !collections.isFavorite(identity));
    }
}

juce::var InstrumentEffectListModel::getDragSourceDescription(const juce::SparseSet<int> & /*rowsToDescribe*/) { return {"Instrument or Effect"}; }

// ----------------------------------------------------------------------------------------------

InstrumentEffectTable::InstrumentEffectTable(tracktion::Engine &engine, InstrumentEffectListModel &model, ApplicationViewState &appState)
    : m_engine(engine),
      m_model(model),
      m_appState(appState)
{
}

tracktion::Plugin::Ptr InstrumentEffectTable::getSelectedPlugin(tracktion::Edit &edit)
{
    const auto *plugin = m_model.getSelectedPlugin();
    if (plugin == nullptr)
        return nullptr;

    if (plugin->pluginFormatName == getInternalPluginFormatName())
        return edit.getPluginCache().createNewPlugin(plugin->category, *plugin);

    return edit.getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, *plugin);
}

//----------------------------------------------------------------------------------------------------

const auto formatWidth = 60;
InstrumentEffectChooser::InstrumentEffectChooser(tracktion::Engine &engine, bool isInstrumentList, ApplicationViewState &appState)
    : m_engine(engine),
      m_isInstrumentList(isInstrumentList),
      m_model(engine, isInstrumentList, appState),
      m_listbox(engine, m_model, appState),
      m_searchField(appState),
      m_appState(appState)
{
    addAndMakeVisible(m_listbox);
    m_model.addChangeListener(this);
    m_listbox.setModel(&m_model);
    m_listbox.setRowHeight(20);
    juce::TableHeaderComponent &header = m_listbox.getHeader();

    header.addColumn(TRANS("Format"), 1, formatWidth, formatWidth, formatWidth, juce::TableHeaderComponent::notResizable | juce::TableHeaderComponent::sortedForwards);
    header.addColumn(TRANS("Name"), 2, juce::jmax(1, getWidth() - formatWidth + 1), 80, 30000, juce::TableHeaderComponent::notResizable);

    header.addColumn(juce::String::fromUTF8("\xe2\x98\x85"), InstrumentEffectListModel::favoriteCol, 26, 26, 26, juce::TableHeaderComponent::notResizable, 0);

    addAndMakeVisible(m_allPluginsButton);
    addAndMakeVisible(m_favoritesButton);
    addAndMakeVisible(m_collectionSelector);
    addAndMakeVisible(m_manageCollectionsButton);

    m_allPluginsButton.setTooltip(TRANS("Show all plugins"));
    m_favoritesButton.setTooltip(TRANS("Show favorites"));
    m_collectionSelector.setTextWhenNothingSelected(TRANS("Collection"));
    m_manageCollectionsButton.setTooltip(TRANS("Manage collections"));

    m_allPluginsButton.onClick = [this] { m_model.changeCollectionFilter({}); };
    m_favoritesButton.onClick = [this] { m_model.changeCollectionFilter(InstrumentEffectListModel::favoritesFilterId()); };
    m_collectionSelector.onChange = [this]
    {
        const auto found = m_collectionIds.find(m_collectionSelector.getSelectedId());
        if (found != m_collectionIds.end())
            m_model.changeCollectionFilter(found->second);
    };
    m_manageCollectionsButton.onClick = [this] { showCollectionManagementMenu(); };
    m_model.onPluginContextRequested = [this](const juce::PluginDescription &plugin, juce::Component *target) { showPluginContextMenu(plugin, target); };
    refreshCollectionControls();

    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);
}

void InstrumentEffectChooser::resized()
{
    auto area = getLocalBounds();
    auto searchField = area.removeFromBottom(30);
    auto collectionBar = area.removeFromBottom(30).reduced(3, 2);
    auto allButton = collectionBar.removeFromLeft(42);
    collectionBar.removeFromLeft(2);
    auto favoritesButton = collectionBar.removeFromLeft(30);
    collectionBar.removeFromLeft(2);
    auto manageButton = collectionBar.removeFromRight(30);
    collectionBar.removeFromRight(2);

    m_listbox.setBounds(area);
    m_allPluginsButton.setBounds(allButton);
    m_favoritesButton.setBounds(favoritesButton);
    m_collectionSelector.setBounds(collectionBar);
    m_manageCollectionsButton.setBounds(manageButton);
    m_searchField.setBounds(searchField);

    juce::TableHeaderComponent &header = m_listbox.getHeader();
    header.setColumnWidth(InstrumentEffectListModel::nameCol, juce::jmax(1, getWidth() - formatWidth - 26 + 1));
}

void InstrumentEffectChooser::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto searchbox = dynamic_cast<SearchFieldComponent *>(source))
    {
        m_model.changeSearchTerm(searchbox->getText());
        repaint();
    }

    if (source == &m_model)
        refreshCollectionControls();

    m_listbox.updateContent();
    if (const int selectedRow = m_model.refreshSelectedPluginRow(); selectedRow >= 0)
        m_listbox.selectRow(selectedRow);
    else
        m_listbox.deselectAllRows();

    if (isShowing())
        if (auto *parent = getParentComponent())
            parent->resized();
}

void InstrumentEffectChooser::refreshCollectionControls()
{
    const auto selectedCollection = m_model.getCollectionFilter();
    const bool showingAll = selectedCollection.isEmpty();
    const bool showingFavorites = selectedCollection == InstrumentEffectListModel::favoritesFilterId();

    m_allPluginsButton.setToggleState(showingAll, juce::dontSendNotification);
    m_favoritesButton.setToggleState(showingFavorites, juce::dontSendNotification);

    m_collectionSelector.clear(juce::dontSendNotification);
    m_collectionIds.clear();

    int itemId = 100;
    int selectedItemId = 0;
    for (const auto &collection : m_appState.getPluginCollections().getCollections())
    {
        m_collectionSelector.addItem(collection.name, itemId);
        m_collectionIds[itemId] = collection.id;
        if (collection.id == selectedCollection)
            selectedItemId = itemId;
        ++itemId;
    }

    if (!showingAll && !showingFavorites && selectedItemId == 0)
    {
        m_model.changeCollectionFilter({});
        return;
    }

    m_collectionSelector.setSelectedId(selectedItemId, juce::dontSendNotification);
    m_collectionSelector.setEnabled(!m_collectionIds.empty());
}

void InstrumentEffectChooser::showPluginContextMenu(const juce::PluginDescription &plugin, juce::Component *target)
{
    auto &collections = m_appState.getPluginCollections();
    const auto identity = PluginIdentity::fromDescription(plugin, plugin.category);
    juce::PopupMenu menu;
    const bool favorite = collections.isFavorite(identity);
    menu.addItem(juce::PopupMenu::Item(favorite ? TRANS("Remove from Favorites") : TRANS("Add to Favorites"))
                     .setTicked(favorite)
                     .setAction(
                         [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), identity, name = plugin.name, favorite]
                         {
                             if (safeThis != nullptr)
                                 safeThis->m_appState.getPluginCollections().setFavorite(identity, name, !favorite);
                         }));

    juce::PopupMenu collectionsMenu;
    for (const auto &collection : collections.getCollections())
    {
        const bool member = collections.isInCollection(identity, collection.id);
        collectionsMenu.addItem(juce::PopupMenu::Item(collection.name)
                                    .setTicked(member)
                                    .setAction(
                                        [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), identity, name = plugin.name, collectionId = collection.id, member]
                                        {
                                            if (safeThis != nullptr)
                                                safeThis->m_appState.getPluginCollections().setCollectionMembership(identity, name, collectionId, !member);
                                        }));
    }
    collectionsMenu.addSeparator();
    collectionsMenu.addItem(juce::PopupMenu::Item(TRANS("New Collection..."))
                                .setAction(
                                    [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), identity, name = plugin.name]
                                    {
                                        if (safeThis == nullptr)
                                            return;
                                        safeThis->showCollectionNameDialog(TRANS("New Plugin Collection"), {},
                                                                           [safeThis, identity, name](const juce::String &collectionName)
                                                                           {
                                                                               if (safeThis == nullptr)
                                                                                   return;
                                                                               juce::String collectionId;
                                                                               auto &model = safeThis->m_appState.getPluginCollections();
                                                                               if (model.addCollection(collectionName, &collectionId))
                                                                                   model.setCollectionMembership(identity, name, collectionId, true);
                                                                           });
                                    }));
    menu.addSubMenu(TRANS("Collections"), collectionsMenu, true);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target != nullptr ? target : this));
}

void InstrumentEffectChooser::showCollectionManagementMenu()
{
    juce::PopupMenu menu;
    menu.addItem(juce::PopupMenu::Item(TRANS("New Collection..."))
                     .setAction(
                         [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this)]
                         {
                             if (safeThis != nullptr)
                                 safeThis->showCollectionNameDialog(TRANS("New Plugin Collection"), {},
                                                                    [safeThis](const juce::String &name)
                                                                    {
                                                                        if (safeThis != nullptr)
                                                                            safeThis->m_appState.getPluginCollections().addCollection(name);
                                                                    });
                         }));

    const auto selected = m_collectionIds.find(m_collectionSelector.getSelectedId());
    const juce::String collectionId = selected != m_collectionIds.end() ? selected->second : juce::String();
    const bool customCollectionSelected = collectionId.isNotEmpty() && collectionId != InstrumentEffectListModel::favoritesFilterId();
    const auto collectionName = m_appState.getPluginCollections().getCollectionName(collectionId);
    menu.addItem(juce::PopupMenu::Item(TRANS("Rename Collection..."))
                     .setEnabled(customCollectionSelected)
                     .setAction(
                         [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), collectionId, collectionName]
                         {
                             if (safeThis != nullptr)
                                 safeThis->showCollectionNameDialog(
                                     TRANS("Rename Plugin Collection"), collectionName,
                                     [safeThis, collectionId](const juce::String &name)
                                     {
                                         if (safeThis != nullptr)
                                             safeThis->m_appState.getPluginCollections().renameCollection(collectionId, name);
                                     },
                                     collectionId);
                         }));
    menu.addItem(juce::PopupMenu::Item(TRANS("Delete Collection..."))
                     .setEnabled(customCollectionSelected)
                     .setAction(
                         [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), collectionId]
                         {
                             if (safeThis != nullptr)
                                 safeThis->confirmCollectionDeletion(collectionId);
                         }));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_manageCollectionsButton));
}

void InstrumentEffectChooser::showCollectionNameDialog(const juce::String &title, const juce::String &initialName, std::function<void(const juce::String &)> completion, const juce::String &exceptCollectionId)
{
    auto *dialog = new juce::AlertWindow(title, TRANS("Enter a collection name:"), juce::AlertWindow::NoIcon);
    dialog->addTextEditor("name", initialName);
    dialog->addButton(TRANS("Save"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton(TRANS("Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<juce::AlertWindow> safeDialog(dialog);
    const auto safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this);
    dialog->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [safeThis, safeDialog, completion = std::move(completion), exceptCollectionId](int result)
                                {
                                    if (result != 1 || safeThis == nullptr || safeDialog == nullptr)
                                        return;

                                    const auto name = safeDialog->getTextEditorContents("name");
                                    if (!safeThis->m_appState.getPluginCollections().isCollectionNameAvailable(name, exceptCollectionId))
                                    {
                                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, TRANS("Invalid Collection"), TRANS("Collection names must be unique, non-empty, and may contain at most 64 characters."));
                                        return;
                                    }
                                    completion(name);
                                }),
                            true);

    if (auto *editor = dialog->getTextEditor("name"))
    {
        editor->grabKeyboardFocus();
        editor->selectAll();
    }
}

void InstrumentEffectChooser::confirmCollectionDeletion(const juce::String &collectionId)
{
    const auto name = m_appState.getPluginCollections().getCollectionName(collectionId);
    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon, TRANS("Delete Plugin Collection"), TRANS("Delete the collection '") + name + "'? Plugin favorites and the plugins themselves are not removed.", TRANS("Delete"), TRANS("Cancel"), this,
                                       juce::ModalCallbackFunction::create(
                                           [safeThis = juce::Component::SafePointer<InstrumentEffectChooser>(this), collectionId](int result)
                                           {
                                               if (result == 1 && safeThis != nullptr)
                                                   safeThis->m_appState.getPluginCollections().removeCollection(collectionId);
                                           }));
}

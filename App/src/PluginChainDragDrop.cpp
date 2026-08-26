/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "PluginChainView.h"

#include "Browser_Base.h"
#include "InstrumentEffectChooser.h"
#include "PluginBrowser.h"
#include "PluginChainItemView.h"
#include "PluginInsertFeedback.h"

namespace
{
enum class PluginChainDragKind
{
    none,
    pluginComponent,
    pluginListEntry,
    instrumentOrEffect,
    fileBrowser,
    automationParameter
};

PluginChainDragKind getPluginChainDragKind(const juce::var &description)
{
    if (description == "PluginComp")
        return PluginChainDragKind::pluginComponent;
    if (description == "PluginListEntry")
        return PluginChainDragKind::pluginListEntry;
    if (description == "Instrument or Effect")
        return PluginChainDragKind::instrumentOrEffect;
    if (description == "FileBrowser")
        return PluginChainDragKind::fileBrowser;
    if (description == te::AutomationDragDropTarget::automatableDragString)
        return PluginChainDragKind::automationParameter;
    return PluginChainDragKind::none;
}

juce::File getDraggedBrowserFile(const juce::DragAndDropTarget::SourceDetails &details)
{
    if (auto *browser = dynamic_cast<BrowserListBox *>(details.sourceComponent.get()))
        return browser->getSelectedFile();

    return {};
}

te::Plugin::Ptr createPluginFromDrag(PluginChainDragKind kind, juce::Component *source, te::Edit &edit)
{
    if (kind == PluginChainDragKind::pluginListEntry)
    {
        auto *list = dynamic_cast<PluginListbox *>(source);
        if (list == nullptr && source != nullptr)
            list = source->findParentComponentOfClass<PluginListbox>();
        return list != nullptr ? list->getSelectedPlugin(edit) : te::Plugin::Ptr{};
    }

    if (kind == PluginChainDragKind::instrumentOrEffect)
    {
        auto *table = dynamic_cast<InstrumentEffectTable *>(source);
        if (table == nullptr && source != nullptr)
            table = source->findParentComponentOfClass<InstrumentEffectTable>();
        return table != nullptr ? table->getSelectedPlugin(edit) : te::Plugin::Ptr{};
    }

    return {};
}
} // namespace

void PluginChainView::insertPluginAtEnd(te::Plugin::Ptr plugin, te::Track::Ptr targetTrack)
{
    if (plugin == nullptr || targetTrack == nullptr)
        return;

    if (targetTrack == m_track)
    {
        insertPluginAtVisualIndex(plugin, getRackOrder().size(), true);
        return;
    }

    m_selectedRackItemID = plugin->itemID;
    m_scrollToSelectedAfterRebuild = true;
    const auto insertResult = EngineHelpers::insertPluginWithPreset(m_evs, targetTrack, plugin);
    if (insertResult != EngineHelpers::PluginInsertResult::inserted)
        UIHelpers::showPluginInsertBlockedDialog(insertResult);
}

bool PluginChainView::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &details)
{
    const auto kind = getPluginChainDragKind(details.description);
    if (kind == PluginChainDragKind::fileBrowser)
        return EngineHelpers::isSoundFontFile(getDraggedBrowserFile(details));

    return kind == PluginChainDragKind::pluginListEntry || kind == PluginChainDragKind::instrumentOrEffect || kind == PluginChainDragKind::automationParameter;
}

void PluginChainView::itemDragMove(const SourceDetails &details)
{
    const auto kind = getPluginChainDragKind(details.description);
    m_isOver = isInterestedInDragSource(details);
    if (kind == PluginChainDragKind::automationParameter)
        m_dragSource = details.sourceComponent.get();
    repaint();
}

void PluginChainView::itemDragExit(const SourceDetails &details)
{
    m_isOver = false;
    if (getPluginChainDragKind(details.description) != PluginChainDragKind::automationParameter)
        m_dragSource = nullptr;
    repaint();
}

void PluginChainView::itemDropped(const juce::DragAndDropTarget::SourceDetails &details)
{
    m_dragSource = nullptr;
    const auto kind = getPluginChainDragKind(details.description);

    if (kind == PluginChainDragKind::fileBrowser)
    {
        const auto draggedFile = getDraggedBrowserFile(details);
        if (EngineHelpers::isSoundFontFile(draggedFile))
        {
            if (m_track != nullptr)
                insertSoundFontAtVisualIndex(draggedFile, getRackOrder().size(), true);
            else
                EngineHelpers::addSoundFontTrack(m_evs, draggedFile, m_evs.m_applicationState.getRandomTrackColour());
        }
    }
    else if (kind == PluginChainDragKind::pluginListEntry || kind == PluginChainDragKind::instrumentOrEffect)
    {
        auto targetTrack = m_track;
        if (targetTrack == nullptr)
            targetTrack = EngineHelpers::addAudioTrack(true, m_evs.m_applicationState.getRandomTrackColour(), m_evs);

        insertPluginAtEnd(createPluginFromDrag(kind, details.sourceComponent.get(), m_evs.m_edit), targetTrack);
    }

    m_isOver = false;
    repaint();
}

bool AddButton::isInterestedInDragSource(const SourceDetails &details)
{
    const auto kind = getPluginChainDragKind(details.description);
    if (kind == PluginChainDragKind::fileBrowser)
        return EngineHelpers::isSoundFontFile(getDraggedBrowserFile(details));

    return kind == PluginChainDragKind::pluginComponent || kind == PluginChainDragKind::pluginListEntry || kind == PluginChainDragKind::instrumentOrEffect || kind == PluginChainDragKind::automationParameter;
}

void AddButton::itemDragMove(const SourceDetails &details)
{
    isOver = isInterestedInDragSource(details);
    repaint();
}

void AddButton::itemDropped(const SourceDetails &details)
{
    auto *rack = findParentComponentOfClass<PluginChainView>();
    if (rack == nullptr || m_track == nullptr)
        return;

    const auto kind = getPluginChainDragKind(details.description);
    const auto getTargetVisualIndex = [this, rack] { return rack->getVisualIndexForPluginOrdinal(getTargetPluginOrdinal()); };

    if (kind == PluginChainDragKind::pluginListEntry || kind == PluginChainDragKind::instrumentOrEffect)
    {
        if (auto pluginToInsert = createPluginFromDrag(kind, details.sourceComponent.get(), m_track->edit))
            rack->insertPluginAtVisualIndex(pluginToInsert, getTargetVisualIndex(), true);
    }
    else if (kind == PluginChainDragKind::fileBrowser)
    {
        const auto draggedFile = getDraggedBrowserFile(details);
        if (EngineHelpers::isSoundFontFile(draggedFile))
            rack->insertSoundFontAtVisualIndex(draggedFile, getTargetVisualIndex(), true);
    }
    else if (kind == PluginChainDragKind::pluginComponent || kind == PluginChainDragKind::automationParameter)
    {
        if (auto *view = dynamic_cast<PluginChainItemView *>(details.sourceComponent.get()))
            rack->moveItem(view, getTargetVisualIndex());
    }

    isOver = false;
    repaint();
}

/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "EditViewState.h"
#include "Utilities.h"

class RackPluginListItem
    : public juce::Component
    , public juce::DragAndDropTarget
{
public:
    explicit RackPluginListItem(EditViewState &evs, te::Track::Ptr track, juce::String sectionLabel, EngineHelpers::PluginChainRole sectionRole, bool showAddButton);
    RackPluginListItem(EditViewState &evs, te::Track::Ptr track, te::Plugin::Ptr plugin, te::EditItemID id, juce::String labelText);

    bool isInterestedInDragSource(const SourceDetails &details) override;
    void itemDragMove(const SourceDetails &details) override;
    void itemDragExit(const SourceDetails &) override;
    void itemDropped(const SourceDetails &details) override;

    void setSelected(bool shouldBeSelected);
    te::EditItemID getItemID() const { return m_itemID; }

    void resized() override;
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    std::function<void()> onClick;
    std::function<void(te::EditItemID, te::EditItemID, bool)> onReorder;
    std::function<void()> onDelete;
    std::function<void()> onToggleEnabled;
    std::function<void(EngineHelpers::PluginChainRole, juce::Component *)> onAdd;

private:
    static void drawEyeIcon(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour, bool enabled);
    static juce::Image createEyeMenuIcon(juce::Colour colour, bool enabled);
    static juce::Image createTrashMenuIcon(juce::Colour colour);
    void updateIconBounds();

    EditViewState &m_evs;
    te::Track::Ptr m_track;
    te::Plugin::Ptr m_plugin;
    te::EditItemID m_itemID;
    juce::String m_label;
    juce::Rectangle<int> m_eyeBounds;
    juce::Rectangle<int> m_trashBounds;
    std::unique_ptr<juce::Drawable> m_trashIcon;
    bool m_selected{false};
    bool m_dragOver{false};
    bool m_dropAfter{false};
    bool m_didDrag{false};
    bool m_isSectionHeader{false};
    EngineHelpers::PluginChainRole m_sectionRole{EngineHelpers::PluginChainRole::audioEffect};
    juce::TextButton m_headerAddButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackPluginListItem)
};

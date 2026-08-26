/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "PluginChainItemView.h"
#include "PluginChainSections.h"
#include "PluginChainView.h"

#include <vector>

class PluginChainView::RackContentComponent : public juce::Component
{
public:
    explicit RackContentComponent(PluginChainView &owner);

    void paint(juce::Graphics &g) override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;
    void refreshButtonsAndLayout();

private:
    friend class PluginChainView;

    struct RoleBuckets
    {
        std::vector<PluginChainItemView *> midiItems;
        std::vector<PluginChainItemView *> instrumentItems;
        std::vector<PluginChainItemView *> audioItems;

        const std::vector<PluginChainItemView *> &forRole(EngineHelpers::PluginChainRole role) const;
    };

    struct SectionVisual
    {
        juce::String title;
        juce::Rectangle<int> bounds;
    };

    void createAddButton(EngineHelpers::PluginChainRole role, int targetPluginOrdinal, const juce::String &label, int x, int y, int width, int height);
    RoleBuckets collectRoleBuckets() const;
    int calculateSectionWidth(EngineHelpers::PluginChainRole role, const std::vector<PluginChainItemView *> &roleItems, int contentHeight, int sectionMinWidth) const;
    void layoutSection(const PluginChainSectionSpec &sectionSpec, const std::vector<PluginChainItemView *> &roleItems, int sectionStartX, int sectionWidth, int contentTop, int contentHeight, int sectionButtonHeight, int &visiblePluginOrdinal);

    juce::OwnedArray<PluginChainItemView> m_rackItems;
    juce::OwnedArray<AddButton> m_addButtons;
    std::vector<SectionVisual> m_sections;
    PluginChainView &m_owner;
};

class PluginChainView::PluginListPanelComponent : public juce::Component
{
public:
    explicit PluginListPanelComponent(PluginChainView &owner);
    void paint(juce::Graphics &g) override;

private:
    PluginChainView &m_owner;
};

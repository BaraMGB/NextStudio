/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "PluginChainRackContent.h"

const std::vector<PluginChainItemView *> &PluginChainView::RackContentComponent::RoleBuckets::forRole(EngineHelpers::PluginChainRole role) const
{
    if (role == EngineHelpers::PluginChainRole::midiEffect)
        return midiItems;
    if (role == EngineHelpers::PluginChainRole::instrument)
        return instrumentItems;
    return audioItems;
}

PluginChainView::RackContentComponent::RackContentComponent(PluginChainView &owner)
    : m_owner(owner)
{
}

void PluginChainView::RackContentComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_owner.m_evs.m_applicationState.getBackgroundColour2().withAlpha(0.3f));

    g.setFont(juce::FontOptions(13.0f));
    for (const auto &section : m_sections)
    {
        g.setColour(m_owner.m_evs.m_applicationState.getBorderColour().withAlpha(0.45f));
        g.drawRoundedRectangle(section.bounds.toFloat(), 5.0f, 1.0f);

        auto sectionBounds = section.bounds;
        auto titleArea = sectionBounds.removeFromTop(18);
        g.setColour(m_owner.m_evs.m_applicationState.getTextColour().withAlpha(0.9f));
        g.drawText(section.title, titleArea.reduced(6, 0), juce::Justification::centredLeft, false);
    }
}

void PluginChainView::RackContentComponent::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) { m_owner.mouseWheelMove(event, wheel); }

void PluginChainView::RackContentComponent::createAddButton(EngineHelpers::PluginChainRole role, int targetPluginOrdinal, const juce::String &label, int x, int y, int width, int height)
{
    auto adder = std::make_unique<AddButton>(m_owner.m_track, m_owner.m_evs.m_applicationState);
    adder->setSectionRole(role);
    adder->setTargetPluginOrdinal(targetPluginOrdinal);
    adder->setButtonText(label);
    addAndMakeVisible(adder.get());
    adder->addListener(&m_owner);
    adder->setBounds(x, y, width, height);
    m_addButtons.add(std::move(adder));
}

PluginChainView::RackContentComponent::RoleBuckets PluginChainView::RackContentComponent::collectRoleBuckets() const
{
    RoleBuckets buckets;

    for (auto *item : m_rackItems)
    {
        if (item == nullptr)
            continue;

        auto plugin = item->getPlugin();
        if (plugin == nullptr)
            continue;

        switch (EngineHelpers::getPluginChainRole(*plugin))
        {
        case EngineHelpers::PluginChainRole::midiEffect:
            buckets.midiItems.push_back(item);
            break;
        case EngineHelpers::PluginChainRole::instrument:
            buckets.instrumentItems.push_back(item);
            break;
        case EngineHelpers::PluginChainRole::audioEffect:
            buckets.audioItems.push_back(item);
            break;
        }
    }

    return buckets;
}

int PluginChainView::RackContentComponent::calculateSectionWidth(EngineHelpers::PluginChainRole role, const std::vector<PluginChainItemView *> &roleItems, int contentHeight, int sectionMinWidth) const
{
    const int addButtonWidth = role != EngineHelpers::PluginChainRole::instrument ? 18 : 0;
    int requiredWidth = 14;
    if (role != EngineHelpers::PluginChainRole::instrument)
        requiredWidth += addButtonWidth + 8;

    for (auto *rackItem : roleItems)
    {
        const int itemWidth = rackItem->isCollapsed() ? rackItem->getHeaderWidth() : (contentHeight * rackItem->getNeededWidthFactor()) / 2;
        if (role != EngineHelpers::PluginChainRole::instrument)
            requiredWidth += itemWidth + 6 + addButtonWidth + 6;
        else
            requiredWidth += itemWidth + 6;
    }

    if (role == EngineHelpers::PluginChainRole::instrument && roleItems.empty())
        requiredWidth = juce::jmax(requiredWidth, 156);

    return juce::jmax(sectionMinWidth, requiredWidth);
}

void PluginChainView::RackContentComponent::layoutSection(const PluginChainSectionSpec &sectionSpec, const std::vector<PluginChainItemView *> &roleItems, int sectionStartX, int sectionWidth, int contentTop, int contentHeight, int sectionButtonHeight, int &visiblePluginOrdinal)
{
    const auto role = sectionSpec.role;
    const int addButtonWidth = role != EngineHelpers::PluginChainRole::instrument ? 18 : 0;
    int cursorX = sectionStartX + 8;

    if (role != EngineHelpers::PluginChainRole::instrument)
    {
        createAddButton(role, visiblePluginOrdinal, "+", cursorX, contentTop + 6, addButtonWidth, sectionButtonHeight);
        cursorX += addButtonWidth + 8;
    }

    for (auto *rackItem : roleItems)
    {
        const int itemWidth = rackItem->isCollapsed() ? rackItem->getHeaderWidth() : (contentHeight * rackItem->getNeededWidthFactor()) / 2;
        rackItem->setBounds(cursorX, contentTop, itemWidth, contentHeight);
        cursorX += itemWidth + 6;
        ++visiblePluginOrdinal;

        if (role != EngineHelpers::PluginChainRole::instrument)
        {
            createAddButton(role, visiblePluginOrdinal, "+", cursorX, contentTop + 6, addButtonWidth, sectionButtonHeight);
            cursorX += addButtonWidth + 6;
        }
    }

    if (role == EngineHelpers::PluginChainRole::instrument && roleItems.empty())
    {
        const int buttonWidth = juce::jmax(120, sectionWidth - 20);
        const int buttonX = sectionStartX + (sectionWidth - buttonWidth) / 2;
        createAddButton(role, visiblePluginOrdinal, "Add Instrument", buttonX, contentTop + 6, buttonWidth, sectionButtonHeight);
    }
}

void PluginChainView::RackContentComponent::refreshButtonsAndLayout()
{
    m_addButtons.clear();
    m_sections.clear();

    const int height = getHeight();
    if (height <= 0)
        return;

    int sectionX = 8;
    constexpr int sectionGap = 10;
    constexpr int contentTop = 20;
    const int contentHeight = juce::jmax(1, height - contentTop - 4);
    constexpr int sectionMinWidth = 170;

    if (m_owner.m_track != nullptr)
    {
        const auto sectionSpecs = getPluginChainSectionSpecs(m_owner.m_track.get());
        const auto roleBuckets = collectRoleBuckets();

        int visiblePluginOrdinal = 0;
        for (const auto &sectionSpec : sectionSpecs)
        {
            const auto role = sectionSpec.role;
            const int sectionButtonHeight = juce::jmax(18, contentHeight - 12);
            const auto &roleItems = roleBuckets.forRole(role);
            const int sectionWidth = calculateSectionWidth(role, roleItems, contentHeight, sectionMinWidth);
            const int sectionStartX = sectionX;

            layoutSection(sectionSpec, roleItems, sectionStartX, sectionWidth, contentTop, contentHeight, sectionButtonHeight, visiblePluginOrdinal);
            m_sections.push_back({sectionSpec.title, juce::Rectangle<int>(sectionStartX, 0, sectionWidth, height - 2)});
            sectionX += sectionWidth + sectionGap;
        }
    }

    setSize(sectionX, height);
}

PluginChainView::PluginListPanelComponent::PluginListPanelComponent(PluginChainView &owner)
    : m_owner(owner)
{
}

void PluginChainView::PluginListPanelComponent::paint(juce::Graphics &g)
{
    const auto headerColour = m_owner.m_track != nullptr ? m_owner.m_track->getColour() : m_owner.m_evs.m_applicationState.getPrimeColour();
    GUIHelpers::drawHeaderBox(g, getLocalBounds().toFloat(), headerColour, m_owner.m_evs.m_applicationState.getBorderColour(), m_owner.m_evs.m_applicationState.getBackgroundColour2(), 20.0f, GUIHelpers::HeaderPosition::top, "Plugins");
}

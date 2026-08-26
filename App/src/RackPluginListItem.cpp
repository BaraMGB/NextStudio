/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "RackPluginListItem.h"

RackPluginListItem::RackPluginListItem(EditViewState &evs, te::Track::Ptr track, juce::String sectionLabel, EngineHelpers::PluginChainRole sectionRole, bool showAddButton)
    : m_evs(evs),
      m_track(std::move(track)),
      m_label(std::move(sectionLabel)),
      m_isSectionHeader(true),
      m_sectionRole(sectionRole)
{
    if (showAddButton)
    {
        addAndMakeVisible(m_headerAddButton);
        m_headerAddButton.setButtonText("+");
        m_headerAddButton.onClick = [this]
        {
            if (onAdd)
                onAdd(m_sectionRole, &m_headerAddButton);
        };
    }
}

RackPluginListItem::RackPluginListItem(EditViewState &evs, te::Track::Ptr track, te::Plugin::Ptr plugin, te::EditItemID id, juce::String labelText)
    : m_evs(evs),
      m_track(std::move(track)),
      m_plugin(std::move(plugin)),
      m_itemID(id),
      m_label(std::move(labelText)),
      m_trashIcon(GUIHelpers::getDrawableFromSvg(BinaryData::trashcan_svg, juce::Colours::lightgrey))
{
}

void RackPluginListItem::drawEyeIcon(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour, bool enabled)
{
    g.setColour(colour);
    g.drawEllipse(area, 1.4f);
    g.fillEllipse(area.getCentreX() - 1.6f, area.getCentreY() - 1.6f, 3.2f, 3.2f);

    if (!enabled)
        g.drawLine(area.getX(), area.getY(), area.getRight(), area.getBottom(), 1.2f);
}

juce::Image RackPluginListItem::createEyeMenuIcon(juce::Colour colour, bool enabled)
{
    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    juce::Graphics g(icon);
    drawEyeIcon(g, {1.0f, 4.0f, 14.0f, 8.0f}, colour, enabled);
    return icon;
}

juce::Image RackPluginListItem::createTrashMenuIcon(juce::Colour colour)
{
    juce::Image icon(juce::Image::ARGB, 16, 16, true);
    juce::Graphics g(icon);
    GUIHelpers::drawFromSvg(g, BinaryData::trashcan_svg, colour, {1.0f, 1.0f, 14.0f, 14.0f});
    return icon;
}

bool RackPluginListItem::isInterestedInDragSource(const SourceDetails &details) { return !m_isSectionHeader && details.description == "RackPluginListItem"; }

void RackPluginListItem::itemDragMove(const SourceDetails &details)
{
    m_dragOver = true;
    m_dropAfter = details.localPosition.getY() > (float)getHeight() * 0.5f;
    repaint();
}

void RackPluginListItem::itemDragExit(const SourceDetails &)
{
    m_dragOver = false;
    repaint();
}

void RackPluginListItem::itemDropped(const SourceDetails &details)
{
    m_dragOver = false;
    repaint();

    auto *source = dynamic_cast<RackPluginListItem *>(details.sourceComponent.get());
    if (source == nullptr || source == this || !source->m_itemID.isValid() || !m_itemID.isValid() || onReorder == nullptr)
        return;

    const bool placeAfter = details.localPosition.getY() > (float)getHeight() * 0.5f;
    onReorder(source->m_itemID, m_itemID, placeAfter);
}

void RackPluginListItem::setSelected(bool shouldBeSelected)
{
    if (m_isSectionHeader)
        shouldBeSelected = false;

    if (m_selected == shouldBeSelected)
        return;

    m_selected = shouldBeSelected;
    repaint();
}

void RackPluginListItem::updateIconBounds()
{
    auto textArea = getLocalBounds().reduced(6, 0);
    constexpr int iconSize = 14;
    constexpr int iconGap = 4;
    const auto iconArea = textArea.removeFromRight((iconSize * 2) + iconGap);
    const int iconY = (getHeight() - iconSize) / 2;
    m_eyeBounds = juce::Rectangle<int>(iconArea.getX(), iconY, iconSize, iconSize).reduced(1);
    m_trashBounds = juce::Rectangle<int>(iconArea.getX() + iconSize + iconGap, iconY, iconSize, iconSize).reduced(1);
}

void RackPluginListItem::resized()
{
    if (m_isSectionHeader)
    {
        if (m_headerAddButton.isVisible())
        {
            auto area = getLocalBounds().reduced(4, 2);
            m_headerAddButton.setBounds(area.removeFromRight(22));
        }
        return;
    }

    updateIconBounds();
}

void RackPluginListItem::paint(juce::Graphics &g)
{
    if (m_isSectionHeader)
    {
        g.fillAll(m_evs.m_applicationState.getBackgroundColour2().withAlpha(0.55f));
        g.setColour(m_evs.m_applicationState.getTextColour().withAlpha(0.95f));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        auto area = getLocalBounds().reduced(6, 0);
        if (m_headerAddButton.isVisible())
            area.removeFromRight(26);
        g.drawText(m_label, area, juce::Justification::centredLeft, false);
        g.setColour(m_evs.m_applicationState.getBorderColour().withAlpha(0.5f));
        g.drawRect(getLocalBounds(), 1);
        return;
    }

    g.fillAll(m_evs.m_applicationState.getBackgroundColour1());

    if (m_selected)
    {
        const auto trackColour = m_track != nullptr ? m_track->getColour() : m_evs.m_applicationState.getPrimeColour();
        g.fillAll(trackColour.withAlpha(0.2f));
    }

    g.setColour(m_evs.m_applicationState.getTextColour());
    g.setFont(juce::FontOptions(14.0f));
    auto textArea = getLocalBounds().reduced(6, 0);
    textArea.removeFromRight(32);

    const auto iconColour = juce::Colours::lightgrey;
    drawEyeIcon(g, m_eyeBounds.toFloat(), iconColour, m_plugin != nullptr ? m_plugin->isEnabled() : true);
    if (m_trashIcon != nullptr)
        m_trashIcon->drawWithin(g, m_trashBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    g.drawText(m_label, textArea, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);

    if (m_dragOver)
    {
        g.setColour(m_evs.m_applicationState.getPrimeColour().withAlpha(0.9f));
        auto marker = getLocalBounds().reduced(2, 0);
        marker.setHeight(2);
        if (m_dropAfter)
            marker.setY(getHeight() - 2);
        g.fillRect(marker);
    }
}

void RackPluginListItem::mouseDown(const juce::MouseEvent &) { m_didDrag = false; }

void RackPluginListItem::mouseDrag(const juce::MouseEvent &e)
{
    if (m_isSectionHeader || m_didDrag || e.getDistanceFromDragStart() < 4)
        return;

    if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        m_didDrag = true;
        auto dragImage = createComponentSnapshot(getLocalBounds());
        container->startDragging("RackPluginListItem", this, juce::ScaledImage(dragImage), true);
    }
}

void RackPluginListItem::mouseUp(const juce::MouseEvent &e)
{
    if (m_isSectionHeader)
        return;

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        const auto iconColour = juce::Colours::lightgrey;
        const bool pluginEnabled = m_plugin != nullptr ? m_plugin->isEnabled() : true;

        menu.addItem(1, "Delete Plugin", true, false, createTrashMenuIcon(iconColour));
        menu.addItem(2, pluginEnabled ? "Disable Plugin" : "Enable Plugin", true, false, createEyeMenuIcon(iconColour, pluginEnabled));

        const int result = menu.show();
        if (result == 1 && onDelete)
            onDelete();
        else if (result == 2 && onToggleEnabled)
            onToggleEnabled();
        return;
    }

    if (m_eyeBounds.contains(e.getPosition()))
    {
        if (onToggleEnabled)
            onToggleEnabled();
        return;
    }

    if (m_trashBounds.contains(e.getPosition()))
    {
        if (onDelete)
            onDelete();
        return;
    }

    if (!m_didDrag && onClick)
        onClick();
}

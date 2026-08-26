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

#include "PluginChainView.h"
#include "PluginInsertFeedback.h"
#include "PluginChainLayout.h"
#include "PluginChainRackContent.h"
#include "PluginChainSections.h"
#include "PluginMenu.h"
#include "RackPanelToggleButton.h"
#include "RackPluginListItem.h"
#include "Utilities.h"
#include <map>

namespace
{
bool pluginTreeItemMatchesRole(const PluginTreeItem &item, EngineHelpers::PluginChainRole role) { return EngineHelpers::getPluginChainRole(item.desc, item.xmlType) == role; }

bool appendFilteredMenuItems(PluginTreeGroup &group, juce::PopupMenu &menu, EngineHelpers::PluginChainRole role, int &nextItemId, std::map<int, PluginTreeItem *> &itemLookup)
{
    bool hasAny = false;

    for (int i = 0; i < group.getNumSubItems(); ++i)
    {
        if (auto *subGroup = dynamic_cast<PluginTreeGroup *>(group.getSubItem(i)))
        {
            juce::PopupMenu subMenu;
            const bool subHasAny = appendFilteredMenuItems(*subGroup, subMenu, role, nextItemId, itemLookup);
            if (subHasAny)
            {
                menu.addSubMenu(subGroup->name, subMenu, true);
                hasAny = true;
            }
        }
    }

    for (int i = 0; i < group.getNumSubItems(); ++i)
    {
        auto *item = dynamic_cast<PluginTreeItem *>(group.getSubItem(i));
        if (item == nullptr)
            continue;

        if (!pluginTreeItemMatchesRole(*item, role))
            continue;

        menu.addItem(nextItemId, item->desc.name, true, false);
        itemLookup[nextItemId] = item;
        ++nextItemId;
        hasAny = true;
    }

    return hasAny;
}

te::Plugin::Ptr showMenuAndCreatePluginForRole(te::Edit &edit, EngineHelpers::PluginChainRole role, juce::Component *target)
{
    if (auto tree = EngineHelpers::createPluginTree(edit.engine))
    {
        PluginTreeGroup root(edit, *tree, te::Plugin::Type::allPlugins);
        juce::PopupMenu menu;
        std::map<int, PluginTreeItem *> itemLookup;
        int nextItemId = 1;
        appendFilteredMenuItems(root, menu, role, nextItemId, itemLookup);

        if (itemLookup.empty())
            return {};

        const int result = target != nullptr ? menu.showAt(target) : menu.show();
        if (result <= 0)
            return {};

        auto it = itemLookup.find(result);
        if (it == itemLookup.end() || it->second == nullptr)
            return {};

        return it->second->create(edit);
    }

    return {};
}

} // namespace

//==============================================================================
PluginChainView::PluginChainView(EditViewState &evs)
    : m_evs(evs),
      m_modifierSidebar(evs),
      m_modifierDetailPanel(evs)
{
    addAndMakeVisible(m_nameLabel);
    m_nameLabel.setJustificationType(juce::Justification::centred);

    m_contentComp = std::make_unique<RackContentComponent>(*this);
    addAndMakeVisible(m_pluginCanvas);
    m_pluginCanvas.addAndMakeVisible(m_contentComp.get());

    m_pluginPanel = std::make_unique<PluginListPanelComponent>(*this);
    addAndMakeVisible(*m_pluginPanel);

    m_trackPresetPanelToggle = std::make_unique<RackPanelToggleButton>("Track Presets", m_evs.m_applicationState);
    m_modifierPanelToggle = std::make_unique<RackPanelToggleButton>("Modifiers", m_evs.m_applicationState);
    addAndMakeVisible(*m_trackPresetPanelToggle);
    addAndMakeVisible(*m_modifierPanelToggle);

    m_trackPresetPanelToggle->onClick = [this]
    {
        m_evs.m_applicationState.m_trackPresetPanelCollapsed = !(bool)m_evs.m_applicationState.m_trackPresetPanelCollapsed;
        resized();
    };
    m_modifierPanelToggle->onClick = [this]
    {
        m_evs.m_applicationState.m_modifierPanelCollapsed = !(bool)m_evs.m_applicationState.m_modifierPanelCollapsed;
        resized();
    };

    addAndMakeVisible(m_horizontalScrollBar);
    m_horizontalScrollBar.setSingleStepSize(30.0);
    m_horizontalScrollBar.addListener(this);

    m_pluginListViewport.setViewedComponent(&m_pluginListContent, false);
    m_pluginListViewport.setScrollBarThickness(m_evs.m_applicationState.getScrollbarThickness());
    m_pluginListViewport.setScrollBarsShown(true, false, false, false);
    m_pluginPanel->addAndMakeVisible(m_pluginListViewport);

    addAndMakeVisible(m_modifierSidebar);
    addChildComponent(m_modifierDetailPanel);

    m_modifierSidebar.onModifierSelected = [this](te::Modifier::Ptr m)
    {
        m_modifierDetailPanel.setModifier(m);
        markAndUpdate(m_updateLayout);
    };
}

PluginChainView::~PluginChainView()
{
    for (auto &b : m_contentComp->m_addButtons)
    {
        b->removeListener(this);
    }

    detachTrackListeners();
    m_horizontalScrollBar.removeListener(this);
    m_pluginListViewport.setViewedComponent(nullptr, false);
}

void PluginChainView::attachTrackListeners()
{
    detachTrackListeners();

    if (m_track == nullptr)
        return;

    m_observedTrackState = m_track->state;
    m_observedTrackState.addListener(this);

    m_observedPluginListState = m_track->pluginList.state;
    if (m_observedPluginListState.isValid())
        m_observedPluginListState.addListener(this);

    m_observedTrackRackState = m_evs.getTrackPluginChainViewState(m_track->itemID);
    if (m_observedTrackRackState.isValid())
        m_observedTrackRackState.addListener(this);
}

void PluginChainView::detachTrackListeners()
{
    if (m_observedTrackState.isValid())
        m_observedTrackState.removeListener(this);

    if (m_observedPluginListState.isValid())
        m_observedPluginListState.removeListener(this);

    if (m_observedTrackRackState.isValid())
        m_observedTrackRackState.removeListener(this);

    m_observedTrackState = {};
    m_observedPluginListState = {};
    m_observedTrackRackState = {};
}

void PluginChainView::paint(juce::Graphics &g)
{
    auto area = getLocalBounds().reduced(1);
    auto outerArea = area.toFloat();
    auto cornerSize = 10.0f;

    g.setColour(m_evs.m_applicationState.getBackgroundColour1());
    g.fillRoundedRectangle(outerArea, cornerSize);

    g.setColour(m_evs.m_applicationState.getBorderColour().withAlpha(0.9f));
    g.drawRoundedRectangle(outerArea, cornerSize, 1.2f);

    if (m_isOver)
    {
        g.setColour(m_evs.m_applicationState.getPrimeColour());
        g.drawRoundedRectangle(outerArea.reduced(1.0f), cornerSize - 1.0f, 2.0f);
    }

    if (m_track == nullptr)
    {
        g.setColour(m_evs.m_applicationState.getTextColour().withAlpha(0.85f));
        g.drawText("select a track for showing rack", area, juce::Justification::centred);
    }
    else
    {
        auto trackCol = m_track->getColour();
        auto labelingCol = trackCol.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);

        m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

        auto header = area.removeFromLeft(HEADERWIDTH);
        g.setColour(trackCol);
        GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

        if (m_channelStrip != nullptr)
        {
            auto sepX = (float)m_channelStrip->getX() - 1.0f;
            g.setColour(m_evs.m_applicationState.getBorderColour().withAlpha(0.8f));
            g.drawLine(sepX, (float)area.getY(), sepX, (float)area.getBottom(), 1.4f);
        }
    };

    auto viewportBounds = m_pluginCanvas.getBounds().toFloat();
    g.setColour(m_evs.m_applicationState.getBorderColour().withAlpha(0.55f));
    g.drawRoundedRectangle(viewportBounds.expanded(1.0f, 1.0f), 6.0f, 1.0f);
}

void PluginChainView::paintOverChildren(juce::Graphics &g)
{
    auto *dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC || !dragC->isDragAndDropActive())
    {
        m_dragSource = nullptr;
        return;
    }
    if (m_dragSource == nullptr)
        return;

    auto modifier = dynamic_cast<ModifierViewComponent *>(m_dragSource->getParentComponent());
    if (modifier == nullptr)
        return;

    auto mousePos = getMouseXYRelative().toFloat();
    auto sourceBounds = getLocalPoint(m_dragSource, m_dragSource->getLocalBounds().getCentre()).toFloat();

    auto *compUnderMouse = getComponentAt(getMouseXYRelative());

    // Helper to find target inside the viewport
    juce::Component *target = compUnderMouse;
    if (target == &m_pluginCanvas)
    {
        auto pt = m_contentComp->getLocalPoint(this, getMouseXYRelative());
        target = m_contentComp->getComponentAt(pt);

        if (target)
        {
            auto pt2 = target->getLocalPoint(m_contentComp.get(), pt);
            auto deeper = target->getComponentAt(pt2);
            if (deeper)
                target = deeper;
        }
    }

    if (target == this || target == &m_pluginCanvas || target == m_contentComp.get())
        target = nullptr;

    juce::Colour lineColour = m_evs.m_applicationState.getTextColour();

    if (target != nullptr)
    {
        auto *slider = dynamic_cast<AutomatableSliderComponent *>(target);
        if (slider == nullptr)
            slider = target->findParentComponentOfClass<AutomatableSliderComponent>();

        if (slider != nullptr)
        {
            auto param = slider->getAutomatableParameter();
            auto mod = modifier->getModifier();

            if (mod->itemID == param->getOwnerID() || param->getTrack() != te::getTrackContainingModifier(mod->edit, mod))
            {
                lineColour = juce::Colours::grey;
                g.setColour(juce::Colours::grey.withAlpha(0.4f));

                // Need bounds relative to THIS (PluginChainView)
                auto bounds = getLocalPoint(slider, juce::Point<int>(0, 0));
                auto rect = juce::Rectangle<int>(bounds.getX(), bounds.getY(), slider->getWidth(), slider->getHeight());

                int size = std::min(rect.getWidth(), rect.getHeight());
                rect = rect.withSizeKeepingCentre(size, size);

                g.fillRect(rect);
                g.setColour(juce::Colours::black);
                g.drawLine(rect.getX(), rect.getY(), rect.getBottomRight().getX(), rect.getBottomRight().getY(), 2.f);
                g.drawLine(rect.getTopRight().getX(), rect.getTopRight().getY(), rect.getBottomLeft().getX(), rect.getBottomLeft().getY(), 2.f);
            }
            else
            {
                lineColour = m_evs.m_applicationState.getPrimeColour();
                g.setColour(lineColour);

                auto bounds = getLocalPoint(slider, juce::Point<int>(0, 0));
                auto rect = juce::Rectangle<int>(bounds.getX(), bounds.getY(), slider->getWidth(), slider->getHeight());

                int size = std::min(rect.getWidth(), rect.getHeight());
                rect = rect.withSizeKeepingCentre(size, size);

                g.drawRect(rect, 2);
            }
        }
    }

    g.setColour(lineColour);
    g.drawLine(sourceBounds.getX(), sourceBounds.getY(), mousePos.getX(), mousePos.getY(), 2.0f);
}

void PluginChainView::mouseDown(const juce::MouseEvent &)
{
    // editViewState.selectionManager.selectOnly (track.get());
}

void PluginChainView::mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel)
{
    float delta = 0.0f;

    if (std::abs(wheel.deltaX) > 0.0001f)
        delta = wheel.deltaX;
    else if (std::abs(wheel.deltaY) > 0.0001f)
        delta = wheel.deltaY;

    if (std::abs(delta) < 0.0001f)
        return;

    stopTimer();
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), m_contentScrollX - (int)std::round(delta * 280.0f));
    m_targetContentScrollX = (double)m_contentScrollX;
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

void PluginChainView::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX(), area.getHeight() - HEADERWIDTH, area.getHeight(), HEADERWIDTH);
    m_nameLabel.setBounds(nameLabelRect);
    m_nameLabel.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), nameLabelRect.getX() + 10.0, nameLabelRect.getY() + 10.0));
    area.removeFromLeft(HEADERWIDTH);
    area = area.reduced(5);

    layoutSidePanels(area);
    layoutPluginList(area);
    layoutRack(area);
}

void PluginChainView::layoutSidePanels(juce::Rectangle<int> &area)
{
    const auto panelHeaderColour = m_track != nullptr ? m_track->getColour() : m_evs.m_applicationState.getPrimeColour();
    const bool trackPresetPanelCollapsed = m_evs.m_applicationState.m_trackPresetPanelCollapsed;
    const bool modifierPanelCollapsed = m_evs.m_applicationState.m_modifierPanelCollapsed;

    m_trackPresetPanelToggle->setAppearance(trackPresetPanelCollapsed, panelHeaderColour);
    m_modifierPanelToggle->setAppearance(modifierPanelCollapsed, panelHeaderColour);

    if (m_trackPresetManager)
    {
        auto presetArea = area.removeFromLeft(trackPresetPanelCollapsed ? COLLAPSED_PANEL_WIDTH : SIDE_PANEL_WIDTH);
        m_trackPresetManager->setVisible(!trackPresetPanelCollapsed);

        if (trackPresetPanelCollapsed)
        {
            m_trackPresetPanelToggle->setBounds(presetArea.reduced(2));
        }
        else
        {
            const auto presetBounds = presetArea.reduced(2);
            m_trackPresetManager->setBounds(presetBounds);
            m_trackPresetPanelToggle->setBounds(presetBounds.getX() + 2, presetBounds.getY() + 2, 18, 18);
        }
        m_trackPresetPanelToggle->setVisible(true);
    }
    else
    {
        m_trackPresetPanelToggle->setVisible(false);
    }

    auto modifierArea = area.removeFromLeft(modifierPanelCollapsed ? COLLAPSED_PANEL_WIDTH : SIDE_PANEL_WIDTH);
    m_modifierSidebar.setVisible(!modifierPanelCollapsed);
    if (modifierPanelCollapsed)
    {
        m_modifierPanelToggle->setBounds(modifierArea.reduced(2));
    }
    else
    {
        const auto modifierBounds = modifierArea.reduced(2);
        m_modifierSidebar.setBounds(modifierBounds);
        m_modifierPanelToggle->setBounds(modifierBounds.getX() + 2, modifierBounds.getY() + 2, 18, 18);
    }
    m_modifierPanelToggle->setVisible(true);

    const bool shouldShowModifierDetail = !modifierPanelCollapsed && m_track != nullptr && m_evs.getTrackSelectedModifier(m_track->itemID).isValid();
    m_modifierDetailPanel.setVisible(shouldShowModifierDetail);
    if (shouldShowModifierDetail)
        m_modifierDetailPanel.setBounds(area.removeFromLeft(MODIFIER_DETAIL_WIDTH));

    m_trackPresetPanelToggle->toFront(false);
    m_modifierPanelToggle->toFront(false);
    area.removeFromLeft(PANEL_CONTENT_GAP);
}

void PluginChainView::layoutPluginList(juce::Rectangle<int> &area)
{
    if (m_channelStrip != nullptr)
        m_channelStrip->setBounds(area.removeFromRight(CHANNEL_STRIP_WIDTH));

    m_pluginPanel->setBounds(area.removeFromLeft(PLUGIN_LIST_WIDTH).reduced(2));

    auto listContentArea = m_pluginPanel->getLocalBounds();
    listContentArea.removeFromTop(22);
    listContentArea.reduce(2, 2);
    m_pluginListViewport.setBounds(listContentArea);

    const int totalHeight = m_pluginListButtons.size() * PLUGIN_LIST_ROW_HEIGHT;
    const bool needsScrollbar = totalHeight > m_pluginListViewport.getHeight();
    const int scrollbarWidth = needsScrollbar ? m_pluginListViewport.getScrollBarThickness() : 0;
    const int contentWidth = juce::jmax(0, m_pluginListViewport.getWidth() - scrollbarWidth);

    int y = 0;
    for (auto *button : m_pluginListButtons)
    {
        button->setBounds(0, y, contentWidth, PLUGIN_LIST_ROW_HEIGHT);
        y += PLUGIN_LIST_ROW_HEIGHT;
    }
    m_pluginListContent.setSize(contentWidth, juce::jmax(m_pluginListViewport.getHeight(), totalHeight));
}

void PluginChainView::layoutRack(juce::Rectangle<int> area)
{
    m_horizontalScrollBar.setBounds(area.removeFromBottom(m_evs.m_applicationState.getScrollbarThickness()));
    m_pluginCanvas.setBounds(area);

    m_contentComp->setSize(juce::jmax(0, m_contentComp->getWidth()), juce::jmax(0, m_pluginCanvas.getHeight()));
    layoutSelectedRackItem();
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), m_contentScrollX);
    m_targetContentScrollX = (double)m_contentScrollX;
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

juce::StringArray PluginChainView::getRackOrder() const
{
    if (m_track == nullptr)
        return {};

    auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
    juce::String orderString = state.getProperty("rackItemOrder").toString();
    juce::StringArray order;
    order.addTokens(orderString, ",", "");
    return order;
}

void PluginChainView::saveRackOrder(const juce::StringArray &order)
{
    if (m_track == nullptr)
        return;

    auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
    state.setProperty("rackItemOrder", order.joinIntoString(","), nullptr);
}

static te::Plugin::Ptr getPluginFromList(te::PluginList &list, te::EditItemID id)
{
    for (auto p : list)
        if (p->itemID == id)
            return p;
    return {};
}

static bool isPluginHidden(te::Track &t, te::Plugin *p)
{
    const bool isChannelStripPlugin = dynamic_cast<te::VolumeAndPanPlugin *>(p) != nullptr || dynamic_cast<te::LevelMeterPlugin *>(p) != nullptr;

    if (!isChannelStripPlugin)
        return false;

    int hiddenTailCount = 0;
    for (int i = t.pluginList.size() - 1; i >= 0; --i)
    {
        auto *tailPlugin = t.pluginList[i];
        const bool tailIsChannelStripPlugin = dynamic_cast<te::VolumeAndPanPlugin *>(tailPlugin) != nullptr || dynamic_cast<te::LevelMeterPlugin *>(tailPlugin) != nullptr;

        if (!tailIsChannelStripPlugin)
            break;

        if (++hiddenTailCount > 2)
            break;

        if (tailPlugin == p)
            return true;
    }

    return false;
}

int PluginChainView::getPluginIndexForVisualIndex(int visualIndex) const
{
    // Domain mapping:
    // - visualIndex: position in rackItemOrder (plugins + modifiers)
    // - returned track plugin index: insertion index in track.pluginList user-plugin domain
    // Hidden tail plugins are excluded by counting only visible plugin IDs from rack order.
    auto order = getRackOrder();
    int targetTrackPluginIndex = 0;
    for (int i = 0; i < visualIndex && i < order.size(); ++i)
    {
        if (getPluginFromList(m_track->pluginList, te::EditItemID::fromVar(order[i])))
            targetTrackPluginIndex++;
    }
    return targetTrackPluginIndex;
}

static int getVisiblePluginOrdinalForID(te::Track &track, te::EditItemID id)
{
    // Visible plugin ordinal = non-hidden plugin position used by rack ordering.
    // It intentionally excludes hidden tail plugins (e.g. meter/pan) from pluginList.
    int ordinal = 0;
    for (auto *plugin : track.pluginList)
    {
        if (plugin == nullptr || isPluginHidden(track, plugin))
            continue;

        if (plugin->itemID == id)
            return ordinal;

        ++ordinal;
    }

    return -1;
}

static int getVisualIndexForPluginOrdinal(const juce::StringArray &order, te::Track &track, int pluginOrdinal)
{
    // Inverse mapping of the domain above:
    // - pluginOrdinal: visible plugin position (no modifiers, no hidden tail plugins)
    // - returned visual index: rackItemOrder insertion slot
    if (pluginOrdinal < 0)
        return order.size();

    int currentOrdinal = 0;
    for (int i = 0; i < order.size(); ++i)
    {
        const auto id = te::EditItemID::fromVar(order[i]);
        if (auto plugin = getPluginFromList(track.pluginList, id))
        {
            if (isPluginHidden(track, plugin.get()))
                continue;

            if (currentOrdinal == pluginOrdinal)
                return i;

            ++currentOrdinal;
        }
    }

    return order.size();
}

int PluginChainView::getVisualIndexForPluginOrdinal(int pluginOrdinal) const
{
    if (m_track == nullptr)
        return 0;

    return ::getVisualIndexForPluginOrdinal(getRackOrder(), *m_track, pluginOrdinal);
}

void PluginChainView::ensureRackOrderConsistency()
{
    auto currentOrder = getRackOrder();
    juce::StringArray newOrder;

    // 1) Keep existing order entries that are still valid.
    for (const auto &idStr : currentOrder)
    {
        const auto id = te::EditItemID::fromVar(idStr);

        if (auto p = getPluginFromList(m_track->pluginList, id))
        {
            if (!isPluginHidden(*m_track, p.get()))
                newOrder.addIfNotAlreadyThere(idStr);

            continue;
        }

        if (auto *ml = m_track->getModifierList())
            if (te::findModifierForID(*ml, id) != nullptr)
                newOrder.addIfNotAlreadyThere(idStr);
    }

    // 2) Append any visible plugins that are missing.
    for (auto *p : m_track->getAllPlugins())
        if (p != nullptr && !isPluginHidden(*m_track, p))
            newOrder.addIfNotAlreadyThere(p->itemID.toString());

    // 3) Append any missing modifiers.
    if (auto *ml = m_track->getModifierList())
        for (auto m : ml->getModifiers())
            if (m != nullptr)
                newOrder.addIfNotAlreadyThere(m->itemID.toString());

    if (newOrder != currentOrder)
        saveRackOrder(newOrder);
}

void PluginChainView::moveItem(PluginChainItemView *item, int targetIndex)
{
    te::EditItemID id;
    if (item->getPlugin())
        id = item->getPlugin()->itemID;
    else if (item->getModifier())
        id = item->getModifier()->itemID;

    if (id.isValid())
    {
        auto order = getRackOrder();
        order.removeString(id.toString());

        // Keep visual rack order aligned with the actual engine plugin order.
        if (item->getPlugin())
        {
            int targetTrackPluginIndex = getPluginIndexForVisualIndex(targetIndex);
            if (EngineHelpers::movePluginWithChainRules(m_track, item->getPlugin(), targetTrackPluginIndex))
            {
                const int pluginOrdinal = getVisiblePluginOrdinalForID(*m_track, id);
                const int visualIndex = ::getVisualIndexForPluginOrdinal(order, *m_track, pluginOrdinal);
                order.insert(visualIndex, id.toString());
            }
            else
            {
                const int fallbackIndex = juce::jlimit(0, order.size(), targetIndex);
                order.insert(fallbackIndex, id.toString());
            }
        }
        else
        {
            if (targetIndex >= order.size())
                order.add(id.toString());
            else
                order.insert(targetIndex, id.toString());
        }

        saveRackOrder(order);

        rebuildView();
    }
}

void PluginChainView::buttonClicked(juce::Button *button)
{
    for (auto &b : m_contentComp->m_addButtons)
    {
        if (b == button)
        {
            if (auto plugin = showMenuAndCreatePluginForRole(m_track->edit, b->getSectionRole(), button))
            {
                const auto order = getRackOrder();
                const int visualIndex = ::getVisualIndexForPluginOrdinal(order, *m_track, b->getTargetPluginOrdinal());
                insertPluginAtVisualIndex(plugin, visualIndex, true);
            }

            m_evs.m_selectionManager.selectOnly(m_track);
            break;
        }
    }
}

void PluginChainView::setTrack(te::Track::Ptr track, bool forceRefresh)
{
    if (m_track == track && m_track != nullptr && !forceRefresh)
    {
        m_nameLabel.setText(m_track->getName(), juce::dontSendNotification);
        m_modifierSidebar.setTrack(m_track);
        updateTrackPresetManager();
        m_modifierDetailPanel.setModifier(m_modifierSidebar.getSelectedModifier());
        resized();
        repaint();
        return;
    }

    detachTrackListeners();

    m_track = track;
    attachTrackListeners();
    m_trackID = m_track != nullptr ? m_track->itemID.toString() : juce::String();
    m_nameLabel.setText(m_track != nullptr ? m_track->getName() : juce::String(), juce::dontSendNotification);

    m_modifierSidebar.setTrack(m_track);
    updateTrackPresetManager();
    m_modifierDetailPanel.setModifier(m_modifierSidebar.getSelectedModifier());

    const bool canShowChannelStrip = m_track != nullptr && (m_track->isMasterTrack() || m_track->isAudioTrack() || m_track->isFolderTrack());
    if (canShowChannelStrip)
    {
        m_channelStrip = std::make_unique<MixerChannelStripComponent>(m_evs, m_track);
        addAndMakeVisible(*m_channelStrip);
    }
    else
    {
        m_channelStrip.reset();
    }

    rebuildView();
}

void PluginChainView::clearTrack()
{
    detachTrackListeners();

    m_track = nullptr;
    m_trackID = "";

    m_modifierSidebar.setTrack(nullptr);
    m_modifierDetailPanel.setModifier(nullptr);
    if (m_trackPresetManager)
    {
        removeChildComponent(m_trackPresetManager.get());
        m_trackPresetManager.reset();
    }
    m_trackPresetAdapter.reset();
    m_channelStrip.reset();
    m_selectedRackItemID = {};

    rebuildView();
}

void PluginChainView::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (v == m_observedTrackState)
    {
        if (m_track != nullptr)
            m_nameLabel.setText(m_track->getName(), juce::dontSendNotification);

        updateTrackPresetManager();
        markAndUpdate(m_updateLayout);
        repaint();
        return;
    }

    if (v == m_observedTrackRackState)
    {
        if (i == IDs::selectedModifier)
        {
            m_modifierDetailPanel.setModifier(m_modifierSidebar.getSelectedModifier());
            markAndUpdate(m_updateLayout);
            repaint();
            return;
        }

        markAndUpdate(m_updatePlugins);
    }
}

void PluginChainView::updateTrackPresetManager()
{
    auto *audioTrack = dynamic_cast<te::AudioTrack *>(m_track.get());
    if (audioTrack == nullptr)
    {
        if (m_trackPresetManager)
        {
            removeChildComponent(m_trackPresetManager.get());
            m_trackPresetManager.reset();
        }
        m_trackPresetAdapter.reset();
        return;
    }

    const bool isMidiPresetTarget = EngineHelpers::isMidiTrack(*audioTrack);
    const auto desiredKind = isMidiPresetTarget ? TrackPresetAdapterBase::PresetKind::midi : TrackPresetAdapterBase::PresetKind::audio;

    if (m_trackPresetAdapter != nullptr && &m_trackPresetAdapter->getTrack() == audioTrack && m_trackPresetAdapter->getPresetKind() == desiredKind)
    {
        if (m_trackPresetManager)
            m_trackPresetManager->setHeaderColour(audioTrack->getColour());
        return;
    }

    if (m_trackPresetManager)
    {
        removeChildComponent(m_trackPresetManager.get());
        m_trackPresetManager.reset();
    }
    m_trackPresetAdapter.reset();

    juce::String presetTitle;
    if (isMidiPresetTarget)
    {
        m_trackPresetAdapter = std::make_unique<MidiTrackPresetAdapter>(*audioTrack, m_evs.m_applicationState);
        presetTitle = "MIDI Track Presets";
    }
    else
    {
        m_trackPresetAdapter = std::make_unique<AudioTrackPresetAdapter>(*audioTrack, m_evs.m_applicationState);
        presetTitle = "Audio Track Presets";
    }

    m_trackPresetManager = std::make_unique<PresetManagerComponent>(*m_trackPresetAdapter, audioTrack->getColour(), presetTitle);
    addAndMakeVisible(*m_trackPresetManager);
}

juce::String PluginChainView::getCurrentTrackID() { return m_trackID; }

void PluginChainView::insertPluginAtVisualIndex(te::Plugin::Ptr plugin, int visualIndex, bool selectInserted)
{
    if (m_track == nullptr || plugin == nullptr)
        return;

    ensureRackOrderConsistency();
    auto order = getRackOrder();

    const int clampedVisualIndex = juce::jlimit(0, order.size(), visualIndex);
    const int targetTrackPluginIndex = getPluginIndexForVisualIndex(clampedVisualIndex);

    const auto insertResult = EngineHelpers::insertPluginWithPreset(m_evs, m_track, plugin, targetTrackPluginIndex);
    if (insertResult != EngineHelpers::PluginInsertResult::inserted)
    {
        UIHelpers::showPluginInsertBlockedDialog(insertResult);
        return;
    }

    const int pluginOrdinal = getVisiblePluginOrdinalForID(*m_track, plugin->itemID);
    const bool wasInserted = pluginOrdinal >= 0;
    if (pluginOrdinal >= 0)
    {
        order.removeString(plugin->itemID.toString());
        const int actualVisualIndex = ::getVisualIndexForPluginOrdinal(order, *m_track, pluginOrdinal);
        order.insert(actualVisualIndex, plugin->itemID.toString());
        saveRackOrder(order);
    }

    if (selectInserted && wasInserted)
    {
        m_selectedRackItemID = plugin->itemID;
        m_scrollToSelectedAfterRebuild = true;
    }
}

void PluginChainView::insertSoundFontAtVisualIndex(const juce::File &file, int visualIndex, bool selectInserted)
{
    if (auto plugin = EngineHelpers::createSoundFontPlugin(m_evs.m_edit, file))
    {
        const auto pluginID = plugin->itemID;
        insertPluginAtVisualIndex(plugin, visualIndex, selectInserted);

        for (auto *insertedPlugin : m_track->pluginList.getPlugins())
        {
            if (insertedPlugin != nullptr && insertedPlugin->itemID == pluginID)
            {
                if (auto *soundFontPlugin = dynamic_cast<SoundFontPlugin *>(insertedPlugin))
                {
                    if (soundFontPlugin->hasLoadedSoundFont())
                        return;

                    const auto errorMessage = soundFontPlugin->getLastError();
                    insertedPlugin->deleteFromParent();
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Failed to load SoundFont", errorMessage.isNotEmpty() ? errorMessage : "The selected SoundFont could not be loaded.");
                }

                break;
            }
        }
    }
}

void PluginChainView::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &c)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void PluginChainView::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &c, int)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void PluginChainView::valueTreeChildOrderChanged(juce::ValueTree &c, int, int)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void PluginChainView::handleAsyncUpdate()
{
    if (compareAndReset(m_updatePlugins))
        rebuildView();

    if (compareAndReset(m_updateLayout))
    {
        resized();
        repaint();
    }
}

void PluginChainView::rebuildView()
{
    m_contentComp->m_rackItems.clear();

    if (m_track != nullptr)
    {
        ensureRackOrderConsistency();
        auto order = getRackOrder();

        for (auto idStr : order)
        {
            auto id = te::EditItemID::fromVar(idStr);

            if (auto p = getPluginFromList(m_track->pluginList, id))
            {
                auto view = std::make_unique<PluginChainItemView>(m_evs, m_track, p);
                m_contentComp->addAndMakeVisible(view.get());
                m_contentComp->m_rackItems.add(std::move(view));
            }
        }
    }

    rebuildPluginList();

    if (getSelectedRackItemIndex() < 0 && m_contentComp->m_rackItems.size() > 0)
    {
        if (auto plugin = m_contentComp->m_rackItems[0]->getPlugin())
            m_selectedRackItemID = plugin->itemID;
    }
    else if (m_contentComp->m_rackItems.isEmpty())
    {
        m_selectedRackItemID = {};
    }

    resized();

    if (m_scrollToSelectedAfterRebuild)
    {
        m_scrollToSelectedAfterRebuild = false;
        if (int idx = getSelectedRackItemIndex(); idx >= 0)
        {
            selectRackItemByIndex(idx);

            auto safeRack = juce::Component::SafePointer<PluginChainView>(this);
            juce::MessageManager::callAsync(
                [safeRack]
                {
                    if (safeRack == nullptr)
                        return;

                    if (int asyncIdx = safeRack->getSelectedRackItemIndex(); asyncIdx >= 0)
                        safeRack->selectRackItemByIndex(asyncIdx);
                });
        }
    }
}

void PluginChainView::rebuildPluginList()
{
    m_pluginListButtons.clear();

    auto addSectionHeader = [this](const juce::String &title, EngineHelpers::PluginChainRole role, bool showAddButton)
    {
        auto header = std::make_unique<RackPluginListItem>(m_evs, m_track, title, role, showAddButton);
        header->onAdd = [this](EngineHelpers::PluginChainRole sectionRole, juce::Component *target) { addPluginAtCurrentPosition(sectionRole, target); };
        m_pluginListContent.addAndMakeVisible(header.get());
        m_pluginListButtons.add(header.release());
    };

    auto addListEntryForItem = [this](PluginChainItemView *item)
    {
        juce::String name = "Plugin";
        te::EditItemID id;
        te::Plugin::Ptr plugin;

        if (auto p = item->getPlugin())
        {
            plugin = p;
            name = plugin->getName();
            id = plugin->itemID;
        }

        if (name.isEmpty())
            name = "Plugin";

        auto button = std::make_unique<RackPluginListItem>(m_evs, m_track, plugin, id, name);
        button->setSelected(id == m_selectedRackItemID);
        button->onClick = [this, id]
        {
            const int index = getRackItemIndexForID(id);
            if (index >= 0)
                selectRackItemByIndex(index);
        };
        button->onReorder = [this](te::EditItemID sourceID, te::EditItemID targetID, bool placeAfter) { reorderPluginListItem(sourceID, targetID, placeAfter); };
        auto safeRack = juce::Component::SafePointer<PluginChainView>(this);

        button->onDelete = [safeRack, plugin]
        {
            if (safeRack == nullptr || plugin == nullptr)
                return;

            juce::MessageManager::callAsync(
                [safeRack, plugin]
                {
                    if (safeRack != nullptr && plugin != nullptr)
                    {
                        plugin->deleteFromParent();
                    }
                });
        };

        button->onToggleEnabled = [safeRack, plugin]
        {
            if (safeRack == nullptr || plugin == nullptr)
                return;

            juce::MessageManager::callAsync(
                [safeRack, plugin]
                {
                    if (safeRack == nullptr || plugin == nullptr)
                        return;

                    plugin->setEnabled(!plugin->isEnabled());
                    safeRack->rebuildPluginList();
                    safeRack->resized();
                    safeRack->repaint();
                });
        };

        m_pluginListContent.addAndMakeVisible(button.get());
        m_pluginListButtons.add(button.release());
    };

    auto addEntriesForRole = [&](EngineHelpers::PluginChainRole role)
    {
        for (auto *item : m_contentComp->m_rackItems)
        {
            if (item == nullptr)
                continue;

            auto plugin = item->getPlugin();
            if (plugin == nullptr)
                continue;

            if (EngineHelpers::getPluginChainRole(*plugin) != role)
                continue;

            addListEntryForItem(item);
        }
    };

    if (m_track != nullptr)
    {
        const auto sectionSpecs = getPluginChainSectionSpecs(m_track.get());
        for (const auto &sectionSpec : sectionSpecs)
        {
            const bool showAddButton = sectionSpec.role != EngineHelpers::PluginChainRole::instrument || !EngineHelpers::trackHasInstrumentPlugin(*m_track);
            addSectionHeader(sectionSpec.title, sectionSpec.role, showAddButton);
            addEntriesForRole(sectionSpec.role);
        }
    }
}

int PluginChainView::getRackItemIndexForID(te::EditItemID id) const
{
    if (!id.isValid())
        return -1;

    for (int i = 0; i < m_contentComp->m_rackItems.size(); ++i)
    {
        if (auto plugin = m_contentComp->m_rackItems[i]->getPlugin())
            if (plugin->itemID == id)
                return i;
    }

    return -1;
}

void PluginChainView::reorderPluginListItem(te::EditItemID sourceID, te::EditItemID targetID, bool placeAfter)
{
    const int sourceIndex = getRackItemIndexForID(sourceID);
    const int targetIndex = getRackItemIndexForID(targetID);
    if (sourceIndex < 0 || targetIndex < 0)
        return;

    if (sourceIndex == targetIndex)
        return;

    const int destinationIndex = PluginChainLayout::getReorderDestinationIndex(sourceIndex, targetIndex, placeAfter, m_contentComp->m_rackItems.size());
    if (destinationIndex < 0)
        return;

    if (auto *item = m_contentComp->m_rackItems[sourceIndex])
    {
        if (auto plugin = item->getPlugin())
            m_selectedRackItemID = plugin->itemID;

        moveItem(item, destinationIndex);
    }
}

void PluginChainView::selectRackItemByIndex(int index)
{
    if (index < 0 || index >= m_contentComp->m_rackItems.size())
        return;

    if (auto *item = m_contentComp->m_rackItems[index])
    {
        if (auto plugin = item->getPlugin())
            m_selectedRackItemID = plugin->itemID;

        for (int i = 0; i < m_pluginListButtons.size(); ++i)
            if (auto *listItem = static_cast<RackPluginListItem *>(m_pluginListButtons[i]))
                listItem->setSelected(listItem->getItemID() == m_selectedRackItemID);

        animateScrollToX(getTargetScrollXForItem(*item));
        repaint();
    }
}

int PluginChainView::getSelectedRackItemIndex() const
{
    if (!m_selectedRackItemID.isValid())
        return -1;

    for (int i = 0; i < m_contentComp->m_rackItems.size(); ++i)
    {
        auto *item = m_contentComp->m_rackItems[i];
        if (auto plugin = item->getPlugin())
            if (plugin->itemID == m_selectedRackItemID)
                return i;
    }

    return -1;
}

void PluginChainView::layoutSelectedRackItem()
{
    for (auto *item : m_contentComp->m_rackItems)
        item->setVisible(true);

    m_contentComp->refreshButtonsAndLayout();
}

void PluginChainView::updateRackContentPosition() { m_contentComp->setTopLeftPosition(-m_contentScrollX, 0); }

int PluginChainView::getMaxContentScrollX() const
{
    return PluginChainLayout::getMaxScrollX(m_contentComp->getWidth(), m_pluginCanvas.getWidth());
}

void PluginChainView::updateHorizontalScrollBar()
{
    const auto visibleWidth = juce::jmax(0, m_pluginCanvas.getWidth());
    const auto totalWidth = juce::jmax(visibleWidth, m_contentComp->getWidth());

    m_updatingHorizontalScrollBar = true;
    m_horizontalScrollBar.setRangeLimits({0.0, (double)totalWidth}, juce::dontSendNotification);
    m_horizontalScrollBar.setCurrentRange({(double)m_contentScrollX, (double)(m_contentScrollX + visibleWidth)}, juce::dontSendNotification);
    m_updatingHorizontalScrollBar = false;
}

int PluginChainView::getTargetScrollXForItem(const PluginChainItemView &item) const
{
    const int maxX = getMaxContentScrollX();
    const int targetX = item.getX();
    return juce::jlimit(0, maxX, targetX);
}

void PluginChainView::animateScrollToX(int targetX)
{
    m_targetContentScrollX = (double)juce::jlimit(0, getMaxContentScrollX(), targetX);
    startTimerHz(60);
}

void PluginChainView::timerCallback()
{
    const auto currentX = m_contentScrollX;
    const auto diff = m_targetContentScrollX - (double)currentX;

    if (std::abs(diff) < 0.75)
    {
        m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), (int)std::round(m_targetContentScrollX));
        updateRackContentPosition();
        updateHorizontalScrollBar();
        stopTimer();
        return;
    }

    const double step = diff * 0.24;
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), (int)std::round((double)currentX + step));
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

void PluginChainView::scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved != &m_horizontalScrollBar || m_updatingHorizontalScrollBar)
        return;

    stopTimer();
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), (int)std::round(newRangeStart));
    m_targetContentScrollX = (double)m_contentScrollX;
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

void PluginChainView::addPluginAtCurrentPosition(EngineHelpers::PluginChainRole role, juce::Component *targetComponent)
{
    if (m_track == nullptr)
        return;

    if (role == EngineHelpers::PluginChainRole::instrument && EngineHelpers::trackHasInstrumentPlugin(*m_track))
        return;

    if (auto plugin = showMenuAndCreatePluginForRole(m_track->edit, role, targetComponent))
    {
        int selectedRoleInsertOrdinal = -1;
        if (const int selectedIndex = getSelectedRackItemIndex(); selectedIndex >= 0)
        {
            if (auto *selectedItem = m_contentComp->m_rackItems[selectedIndex])
            {
                if (auto selectedPlugin = selectedItem->getPlugin())
                {
                    if (EngineHelpers::getPluginChainRole(*selectedPlugin) == role)
                    {
                        const int selectedOrdinal = getVisiblePluginOrdinalForID(*m_track, selectedPlugin->itemID);
                        if (selectedOrdinal >= 0)
                            selectedRoleInsertOrdinal = selectedOrdinal + 1;
                    }
                }
            }
        }

        const auto order = getRackOrder();
        const int requestedOrdinal = selectedRoleInsertOrdinal >= 0 ? selectedRoleInsertOrdinal : m_contentComp->m_rackItems.size();
        const int insertVisualIndex = ::getVisualIndexForPluginOrdinal(order, *m_track, requestedOrdinal);
        insertPluginAtVisualIndex(plugin, insertVisualIndex, true);
    }
}

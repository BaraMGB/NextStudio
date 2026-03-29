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

#include "LowerRange/PluginChain/PluginChainView.h"
#include "Plugins/SimpleSynth/SimpleSynthPluginComponent.h"
#include "SideBrowser/InstrumentEffectChooser.h"
#include "UI/PluginInsertFeedback.h"
#include "UI/PluginMenu.h"
#include "Utilities/Utilities.h"
#include <map>
#include <vector>

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

struct ChainSectionSpec
{
    EngineHelpers::PluginChainRole role;
    juce::String title;
};

std::vector<ChainSectionSpec> getSectionSpecsForTrack(const te::Track *track)
{
    if (track != nullptr && EngineHelpers::isMidiTrack(*track))
    {
        return {{EngineHelpers::PluginChainRole::midiEffect, "MIDI Plugins"}, {EngineHelpers::PluginChainRole::instrument, "Instrument"}, {EngineHelpers::PluginChainRole::audioEffect, "Audio Effects"}};
    }

    return {{EngineHelpers::PluginChainRole::audioEffect, "Audio Effects"}};
}

} // namespace

//==============================================================================
class PluginChainView::RackContentComponent : public juce::Component
{
public:
    struct RoleBuckets
    {
        std::vector<PluginChainItemView *> midiItems;
        std::vector<PluginChainItemView *> instrumentItems;
        std::vector<PluginChainItemView *> audioItems;

        const std::vector<PluginChainItemView *> &forRole(EngineHelpers::PluginChainRole role) const
        {
            if (role == EngineHelpers::PluginChainRole::midiEffect)
                return midiItems;
            if (role == EngineHelpers::PluginChainRole::instrument)
                return instrumentItems;
            return audioItems;
        }
    };

    RackContentComponent(PluginChainView &v)
        : m_owner(v)
    {
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(m_owner.m_evs.m_applicationState.getBackgroundColour2().withAlpha(0.3f));

        g.setFont(13.0f);
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

    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override { m_owner.mouseWheelMove(event, wheel); }

    void createAddButton(EngineHelpers::PluginChainRole role, int targetPluginOrdinal, const juce::String &label, int x, int y, int w, int h)
    {
        auto adder = std::make_unique<AddButton>(m_owner.m_track, m_owner.m_evs.m_applicationState);
        adder->setSectionRole(role);
        adder->setTargetPluginOrdinal(targetPluginOrdinal);
        adder->setButtonText(label);
        addAndMakeVisible(adder.get());
        adder->addListener(&m_owner);
        adder->setBounds(x, y, w, h);
        m_addButtons.add(std::move(adder));
    }

    RoleBuckets collectRoleBuckets() const
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

    int calculateSectionWidth(EngineHelpers::PluginChainRole role, const std::vector<PluginChainItemView *> &roleItems, int contentHeight, int sectionMinWidth) const
    {
        const int addButtonWidth = (role != EngineHelpers::PluginChainRole::instrument) ? 18 : 0;
        int requiredWidth = 14;
        if (role != EngineHelpers::PluginChainRole::instrument)
            requiredWidth += addButtonWidth + 8;

        for (auto *rackItem : roleItems)
        {
            int itemWidth = rackItem->isCollapsed() ? rackItem->getHeaderWidth() : (contentHeight * rackItem->getNeededWidthFactor()) / 2;
            if (role != EngineHelpers::PluginChainRole::instrument)
                requiredWidth += itemWidth + 6 + addButtonWidth + 6;
            else
                requiredWidth += itemWidth + 6;
        }

        if (role == EngineHelpers::PluginChainRole::instrument && roleItems.empty())
            requiredWidth = juce::jmax(requiredWidth, 156);

        return juce::jmax(sectionMinWidth, requiredWidth);
    }

    void layoutSection(const ChainSectionSpec &sectionSpec, const std::vector<PluginChainItemView *> &roleItems, int sectionStartX, int sectionWidth, int contentTop, int contentHeight, int sectionButtonHeight, int &visiblePluginOrdinal)
    {
        const auto role = sectionSpec.role;
        const int addButtonWidth = (role != EngineHelpers::PluginChainRole::instrument) ? 18 : 0;
        int cursorX = sectionStartX + 8;

        if (role != EngineHelpers::PluginChainRole::instrument)
        {
            createAddButton(role, visiblePluginOrdinal, "+", cursorX, contentTop + 6, addButtonWidth, sectionButtonHeight);
            cursorX += addButtonWidth + 8;
        }

        for (auto *rackItem : roleItems)
        {
            int itemWidth = rackItem->isCollapsed() ? rackItem->getHeaderWidth() : (contentHeight * rackItem->getNeededWidthFactor()) / 2;
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

    void refreshButtonsAndLayout()
    {
        m_addButtons.clear();
        m_sections.clear();

        int height = getHeight();
        if (height <= 0)
            return;

        int sectionX = 8;
        const int sectionGap = 10;
        const int contentTop = 20;
        const int contentHeight = juce::jmax(1, height - contentTop - 4);
        const int sectionMinWidth = 170;

        if (m_owner.m_track != nullptr)
        {
            const auto sectionSpecs = getSectionSpecsForTrack(m_owner.m_track.get());
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

    struct SectionVisual
    {
        juce::String title;
        juce::Rectangle<int> bounds;
    };

    juce::OwnedArray<PluginChainItemView> m_rackItems;
    juce::OwnedArray<AddButton> m_addButtons;
    std::vector<SectionVisual> m_sections;
    PluginChainView &m_owner;
};

class PluginChainView::PluginListPanelComponent : public juce::Component
{
public:
    explicit PluginListPanelComponent(PluginChainView &owner)
        : m_owner(owner)
    {
    }

    void paint(juce::Graphics &g) override
    {
        auto headerColour = m_owner.m_track != nullptr ? m_owner.m_track->getColour() : m_owner.m_evs.m_applicationState.getPrimeColour();
        GUIHelpers::drawHeaderBox(g, getLocalBounds().toFloat(), headerColour, m_owner.m_evs.m_applicationState.getBorderColour(), m_owner.m_evs.m_applicationState.getBackgroundColour2(), 20.0f, GUIHelpers::HeaderPosition::top, "Plugins");
    }

private:
    PluginChainView &m_owner;
};

class RackPluginListItem
    : public juce::Component
    , public juce::DragAndDropTarget
{
public:
    explicit RackPluginListItem(EditViewState &evs, te::Track::Ptr t, juce::String sectionLabel, EngineHelpers::PluginChainRole sectionRole, bool showAddButton)
        : m_evs(evs),
          m_track(t),
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

    RackPluginListItem(EditViewState &evs, te::Track::Ptr t, te::Plugin::Ptr plugin, te::EditItemID id, juce::String labelText)
        : m_evs(evs),
          m_track(t),
          m_plugin(std::move(plugin)),
          m_itemID(id),
          m_label(std::move(labelText))
    {
    }

    static void drawEyeIcon(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour, bool enabled)
    {
        g.setColour(colour);
        g.drawEllipse(area, 1.4f);
        g.fillEllipse(area.getCentreX() - 1.6f, area.getCentreY() - 1.6f, 3.2f, 3.2f);

        if (!enabled)
            g.drawLine(area.getX(), area.getY(), area.getRight(), area.getBottom(), 1.2f);
    }

    static juce::Image createEyeMenuIcon(juce::Colour colour, bool enabled)
    {
        juce::Image icon(juce::Image::ARGB, 16, 16, true);
        juce::Graphics g(icon);
        drawEyeIcon(g, {1.0f, 4.0f, 14.0f, 8.0f}, colour, enabled);
        return icon;
    }

    static juce::Image createTrashMenuIcon(juce::Colour colour)
    {
        juce::Image icon(juce::Image::ARGB, 16, 16, true);
        juce::Graphics g(icon);
        GUIHelpers::drawFromSvg(g, BinaryData::trashcan_svg, colour, {1.0f, 1.0f, 14.0f, 14.0f});
        return icon;
    }

    bool isInterestedInDragSource(const SourceDetails &details) override
    {
        if (m_isSectionHeader)
            return false;

        return details.description == "RackPluginListItem";
    }

    void itemDragMove(const SourceDetails &details) override
    {
        m_dragOver = true;
        m_dropAfter = details.localPosition.getY() > (float)getHeight() * 0.5f;
        repaint();
    }

    void itemDragExit(const SourceDetails &) override
    {
        m_dragOver = false;
        repaint();
    }

    void itemDropped(const SourceDetails &details) override
    {
        m_dragOver = false;
        repaint();

        auto *source = dynamic_cast<RackPluginListItem *>(details.sourceComponent.get());
        if (source == nullptr || source == this || !source->m_itemID.isValid() || !m_itemID.isValid() || onReorder == nullptr)
            return;

        const bool placeAfter = details.localPosition.getY() > (float)getHeight() * 0.5f;
        onReorder(source->m_itemID, m_itemID, placeAfter);
    }

    void setSelected(bool shouldBeSelected)
    {
        if (m_isSectionHeader)
            shouldBeSelected = false;

        if (m_selected == shouldBeSelected)
            return;

        m_selected = shouldBeSelected;
        repaint();
    }

    te::EditItemID getItemID() const { return m_itemID; }

    void resized() override
    {
        if (!m_isSectionHeader || !m_headerAddButton.isVisible())
            return;

        auto area = getLocalBounds().reduced(4, 2);
        m_headerAddButton.setBounds(area.removeFromRight(22));
    }

    void paint(juce::Graphics &g) override
    {
        if (m_isSectionHeader)
        {
            g.fillAll(m_evs.m_applicationState.getBackgroundColour2().withAlpha(0.55f));
            g.setColour(m_evs.m_applicationState.getTextColour().withAlpha(0.95f));
            g.setFont(juce::Font(13.0f, juce::Font::bold));
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
            auto trackColour = m_track != nullptr ? m_track->getColour() : m_evs.m_applicationState.getPrimeColour();
            g.fillAll(trackColour.withAlpha(0.2f));
        }

        g.setColour(m_evs.m_applicationState.getTextColour());
        g.setFont(14.0f);
        auto textArea = getLocalBounds().reduced(6, 0);
        constexpr int iconSize = 14;
        constexpr int iconGap = 4;
        auto iconArea = textArea.removeFromRight((iconSize * 2) + iconGap);
        const int iconY = (getHeight() - iconSize) / 2;
        m_eyeBounds = juce::Rectangle<int>(iconArea.getX(), iconY, iconSize, iconSize).reduced(1);
        m_trashBounds = juce::Rectangle<int>(iconArea.getX() + iconSize + iconGap, iconY, iconSize, iconSize).reduced(1);

        auto iconColour = juce::Colours::lightgrey;
        drawEyeIcon(g, m_eyeBounds.toFloat(), iconColour, m_plugin != nullptr ? m_plugin->isEnabled() : true);
        GUIHelpers::drawFromSvg(g, BinaryData::trashcan_svg, iconColour, m_trashBounds.toFloat());

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

    void mouseDown(const juce::MouseEvent &e) override
    {
        juce::ignoreUnused(e);
        m_didDrag = false;
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (m_isSectionHeader)
            return;

        if (m_didDrag || e.getDistanceFromDragStart() < 4)
            return;

        if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            m_didDrag = true;
            auto dragImage = createComponentSnapshot(getLocalBounds());
            container->startDragging("RackPluginListItem", this, dragImage, true);
        }
    }

    void mouseUp(const juce::MouseEvent &e) override
    {
        if (m_isSectionHeader)
            return;

        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            const auto iconColour = juce::Colours::lightgrey;
            const bool pluginEnabled = (m_plugin != nullptr ? m_plugin->isEnabled() : true);

            menu.addItem(1, "Delete Plugin", true, false, createTrashMenuIcon(iconColour));
            menu.addItem(2, pluginEnabled ? "Disable Plugin" : "Enable Plugin", true, false, createEyeMenuIcon(iconColour, pluginEnabled));

            const int result = menu.show();
            if (result == 1)
            {
                if (onDelete)
                    onDelete();
            }
            else if (result == 2)
            {
                if (onToggleEnabled)
                    onToggleEnabled();
            }
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

        if (!m_didDrag && !e.mods.isPopupMenu() && onClick)
            onClick();
    }

    std::function<void()> onClick;
    std::function<void(te::EditItemID, te::EditItemID, bool)> onReorder;
    std::function<void()> onDelete;
    std::function<void()> onToggleEnabled;
    std::function<void(EngineHelpers::PluginChainRole, juce::Component *)> onAdd;

private:
    EditViewState &m_evs;
    te::Track::Ptr m_track;
    te::Plugin::Ptr m_plugin;
    te::EditItemID m_itemID;
    juce::String m_label;
    juce::Rectangle<int> m_eyeBounds;
    juce::Rectangle<int> m_trashBounds;
    bool m_selected{false};
    bool m_dragOver{false};
    bool m_dropAfter{false};
    bool m_didDrag{false};
    bool m_isSectionHeader{false};
    EngineHelpers::PluginChainRole m_sectionRole{EngineHelpers::PluginChainRole::audioEffect};
    juce::TextButton m_headerAddButton;
};

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

    addAndMakeVisible(m_horizontalScrollBar);
    m_horizontalScrollBar.setSingleStepSize(30.0);
    m_horizontalScrollBar.addListener(this);

    m_pluginListViewport.setViewedComponent(&m_pluginListContent, false);
    m_pluginListViewport.setScrollBarsShown(true, false, false, false);
    m_pluginPanel->addAndMakeVisible(m_pluginListViewport);

    addAndMakeVisible(m_modifierSidebar);
    addChildComponent(m_modifierDetailPanel); // Start hidden

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
}

void PluginChainView::detachTrackListeners()
{
    if (m_observedTrackState.isValid())
        m_observedTrackState.removeListener(this);

    if (m_observedPluginListState.isValid())
        m_observedPluginListState.removeListener(this);

    m_observedTrackState = {};
    m_observedPluginListState = {};
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

    if (m_trackPresetManager)
    {
        auto presetArea = area.removeFromLeft(MODIFIER_STACK_WIDTH);
        m_trackPresetManager->setBounds(presetArea.reduced(2));
    }

    auto modifierArea = area.removeFromLeft(MODIFIER_STACK_WIDTH);
    m_modifierSidebar.setBounds(modifierArea.reduced(2));

    const bool shouldShowModifierDetail = (m_track != nullptr && m_evs.getTrackSelectedModifier(m_track->itemID).isValid());
    if (shouldShowModifierDetail)
    {
        m_modifierDetailPanel.setBounds(area.removeFromLeft(300));
        m_modifierDetailPanel.setVisible(true);
    }
    else
    {
        m_modifierDetailPanel.setVisible(false);
    }

    area.removeFromLeft(20);

    if (m_channelStrip != nullptr)
        m_channelStrip->setBounds(area.removeFromRight(CHANNEL_STRIP_WIDTH));

    auto listArea = area.removeFromLeft(PLUGIN_LIST_WIDTH);
    m_pluginPanel->setBounds(listArea.reduced(2));

    auto listContentArea = m_pluginPanel->getLocalBounds();
    listContentArea.removeFromTop(20);

    listContentArea.removeFromTop(2);

    listContentArea.reduce(2, 2);
    m_pluginListViewport.setBounds(listContentArea);

    int y = 0;
    for (auto *button : m_pluginListButtons)
    {
        button->setBounds(0, y, juce::jmax(0, m_pluginListViewport.getWidth() - 12), PLUGIN_LIST_ROW_HEIGHT);
        y += PLUGIN_LIST_ROW_HEIGHT;
    }
    m_pluginListContent.setSize(juce::jmax(0, m_pluginListViewport.getWidth() - 12), juce::jmax(m_pluginListViewport.getHeight(), y));

    auto scrollbarArea = area.removeFromBottom(HORIZONTAL_SCROLLBAR_HEIGHT);
    m_horizontalScrollBar.setBounds(scrollbarArea);
    m_pluginCanvas.setBounds(area);

    int contentHeight = juce::jmax(0, m_pluginCanvas.getHeight());
    m_contentComp->setSize(juce::jmax(0, m_contentComp->getWidth()), contentHeight);
    layoutSelectedRackItem();
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), m_contentScrollX);
    m_targetContentScrollX = (double)m_contentScrollX;
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

juce::StringArray PluginChainView::getRackOrder() const
{
    auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
    juce::String orderString = state.getProperty("rackItemOrder").toString();
    juce::StringArray order;
    order.addTokens(orderString, ",", "");
    return order;
}

void PluginChainView::saveRackOrder(const juce::StringArray &order)
{
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
                const int visualIndex = getVisualIndexForPluginOrdinal(order, *m_track, pluginOrdinal);
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
                const int visualIndex = getVisualIndexForPluginOrdinal(order, *m_track, b->getTargetPluginOrdinal());
                insertPluginAtVisualIndex(plugin, visualIndex, true);
            }

            m_evs.m_selectionManager.selectOnly(m_track);
            break;
        }
    }
}

void PluginChainView::setTrack(te::Track::Ptr track)
{
    if (m_track == track && m_track != nullptr)
    {
        m_nameLabel.setText(m_track->getName(), juce::dontSendNotification);
        m_modifierSidebar.setTrack(m_track);
        updateTrackPresetManager();
        return;
    }

    detachTrackListeners();

    m_track = track;
    attachTrackListeners();
    m_trackID = m_track->itemID.toString();
    m_nameLabel.setText(m_track->getName(), juce::dontSendNotification);

    m_modifierSidebar.setTrack(m_track);
    m_modifierDetailPanel.setModifier(nullptr);
    updateTrackPresetManager();

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

    rebuildView();
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

    if (m_trackPresetAdapter != nullptr && &m_trackPresetAdapter->getTrack() == audioTrack)
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

    m_trackPresetAdapter = std::make_unique<TrackPresetAdapter>(*audioTrack, m_evs.m_applicationState);
    m_trackPresetManager = std::make_unique<PresetManagerComponent>(*m_trackPresetAdapter, audioTrack->getColour(), "TrackPresets");
    addAndMakeVisible(*m_trackPresetManager);
}

juce::String PluginChainView::getCurrentTrackID() { return m_trackID; }

juce::OwnedArray<AddButton> &PluginChainView::getAddButtons() { return m_contentComp->m_addButtons; }

juce::OwnedArray<PluginChainItemView> &PluginChainView::getPluginComponents() { return m_contentComp->m_rackItems; }

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
        const int actualVisualIndex = getVisualIndexForPluginOrdinal(order, *m_track, pluginOrdinal);
        order.insert(actualVisualIndex, plugin->itemID.toString());
        saveRackOrder(order);
    }

    if (selectInserted && wasInserted)
    {
        m_selectedRackItemID = plugin->itemID;
        m_scrollToSelectedAfterRebuild = true;
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
        const auto sectionSpecs = getSectionSpecsForTrack(m_track.get());
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

    int destinationIndex = targetIndex + (placeAfter ? 1 : 0);
    if (sourceIndex < destinationIndex)
        --destinationIndex;

    destinationIndex = juce::jlimit(0, m_contentComp->m_rackItems.size() - 1, destinationIndex);

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

int PluginChainView::getLastPluginLeftEdgeX() const
{
    int leftEdge = 0;
    for (auto *item : m_contentComp->m_rackItems)
        leftEdge = juce::jmax(leftEdge, item->getX());

    return leftEdge;
}

int PluginChainView::getMaxContentScrollX() const { return juce::jmax(0, getLastPluginLeftEdgeX()); }

void PluginChainView::updateHorizontalScrollBar()
{
    const auto visibleWidth = juce::jmax(0, m_pluginCanvas.getWidth());
    const auto totalWidth = juce::jmax(visibleWidth, getMaxContentScrollX() + visibleWidth);

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
        const int insertVisualIndex = getVisualIndexForPluginOrdinal(order, *m_track, requestedOrdinal);
        insertPluginAtVisualIndex(plugin, insertVisualIndex, true);
    }
}

bool PluginChainView::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry" || dragSourceDetails.description == "Instrument or Effect" || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        return true;
    }
    return false;
}

void PluginChainView::itemDragMove(const SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginComp" || dragSourceDetails.description == "PluginListEntry" || dragSourceDetails.description == "Instrument or Effect" || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)

    {
        m_isOver = true;
    }

    if (dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        m_dragSource = dragSourceDetails.sourceComponent.get();
    }
    repaint();
}

void PluginChainView::itemDragExit(const SourceDetails &details)
{
    m_isOver = false;

    if (details.description != te::AutomationDragDropTarget::automatableDragString)
        m_dragSource = nullptr;

    repaint();
}

void PluginChainView::itemDropped(const juce::DragAndDropTarget::SourceDetails &details)
{
    m_dragSource = nullptr;

    te::Track::Ptr track;
    if (m_track != nullptr)
        track = m_track;
    else
        track = EngineHelpers::addAudioTrack(true, m_evs.m_applicationState.getRandomTrackColour(), m_evs);

    if (details.description == "PluginListEntry")
        if (auto listbox = dynamic_cast<PluginListbox *>(details.sourceComponent.get()))
            if (auto plugin = listbox->getSelectedPlugin(m_evs.m_edit))
            {
                if (track == m_track)
                {
                    const int endVisualIndex = getRackOrder().size();
                    insertPluginAtVisualIndex(plugin, endVisualIndex, true);
                }
                else
                {
                    m_selectedRackItemID = plugin->itemID;
                    m_scrollToSelectedAfterRebuild = true;
                    const auto insertResult = EngineHelpers::insertPluginWithPreset(m_evs, track, plugin);
                    if (insertResult != EngineHelpers::PluginInsertResult::inserted)
                        UIHelpers::showPluginInsertBlockedDialog(insertResult);
                }
            }
    if (details.description == "Instrument or Effect")
        if (auto lb = dynamic_cast<InstrumentEffectTable *>(details.sourceComponent.get()))
            if (auto plugin = lb->getSelectedPlugin(m_evs.m_edit))
            {
                if (track == m_track)
                {
                    const int endVisualIndex = getRackOrder().size();
                    insertPluginAtVisualIndex(plugin, endVisualIndex, true);
                }
                else
                {
                    m_selectedRackItemID = plugin->itemID;
                    m_scrollToSelectedAfterRebuild = true;
                    const auto insertResult = EngineHelpers::insertPluginWithPreset(m_evs, track, plugin);
                    if (insertResult != EngineHelpers::PluginInsertResult::inserted)
                        UIHelpers::showPluginInsertBlockedDialog(insertResult);
                }
            }

    m_isOver = false;
    repaint();
}

void AddButton::itemDropped(const SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<juce::ListBox *>(dragSourceDetails.sourceComponent.get()))
        {
            if (auto lbm = dynamic_cast<PluginListbox *>(listbox->getModel()))
            {
                // Resolve owning rack view from the AddButton hierarchy.
                auto pluginRackComp = findParentComponentOfClass<PluginChainView>();
                if (pluginRackComp)
                {
                    auto plugin = lbm->getSelectedPlugin(m_track->edit);
                    if (plugin)
                    {
                        const auto order = pluginRackComp->getRackOrder();
                        const int targetVisualIndex = getVisualIndexForPluginOrdinal(order, *m_track, getTargetPluginOrdinal());
                        pluginRackComp->insertPluginAtVisualIndex(plugin, targetVisualIndex, true);
                    }
                }
            }
        }
    }

    if (dragSourceDetails.description == "PluginComp" || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        auto pluginRackComp = findParentComponentOfClass<PluginChainView>();
        if (pluginRackComp)
        {
            auto *view = dynamic_cast<PluginChainItemView *>(dragSourceDetails.sourceComponent.get());
            if (view)
            {
                const auto order = pluginRackComp->getRackOrder();
                const int targetIndex = getVisualIndexForPluginOrdinal(order, *m_track, getTargetPluginOrdinal());
                pluginRackComp->moveItem(view, targetIndex);
            }
        }
    }
}

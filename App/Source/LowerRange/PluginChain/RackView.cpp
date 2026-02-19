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

#include "LowerRange/PluginChain/RackView.h"
#include "Plugins/SimpleSynth/SimpleSynthPluginComponent.h"
#include "SideBrowser/InstrumentEffectChooser.h"
#include "UI/PluginMenu.h"
#include "Utilities/Utilities.h"

//==============================================================================
class RackView::RackContentComponent : public juce::Component
{
public:
    RackContentComponent(RackView &v)
        : m_owner(v)
    {
    }

    void paint(juce::Graphics &) override {}

    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override { m_owner.mouseWheelMove(event, wheel); }

    void refreshButtonsAndLayout()
    {
        m_addButtons.clear();

        int height = getHeight();
        if (height <= 0)
            return;

        int x = 0;

        if (m_owner.m_track != nullptr)
        {
            // --- First Add Button ---
            auto firstAdder = std::make_unique<AddButton>(m_owner.m_track, m_owner.m_evs.m_applicationState);
            addAndMakeVisible(firstAdder.get());
            firstAdder->addListener(&m_owner);
            firstAdder->setButtonText("+");

            firstAdder->setBounds(juce::Rectangle<int>(x, 0, 15, height).reduced(0, 10));
            x += 15;

            m_addButtons.add(std::move(firstAdder));

            // --- Rack Items and Interleaved Buttons ---
            for (auto p : m_rackItems)
            {
                x += 5; // padding

                int itemWidth = 0;
                if (p->isCollapsed())
                    itemWidth = p->getHeaderWidth();
                else
                    itemWidth = (height * p->getNeededWidthFactor()) / 2;

                p->setBounds(x, 0, itemWidth, height);
                x += itemWidth;

                x += 5; // padding

                auto adder = std::make_unique<AddButton>(m_owner.m_track, m_owner.m_evs.m_applicationState);
                if (p->getPlugin())
                    adder->setPlugin(p->getPlugin());

                addAndMakeVisible(adder.get());
                adder->setButtonText("+");
                adder->setBounds(juce::Rectangle<int>(x, 0, 15, height).reduced(0, 10));
                adder->addListener(&m_owner);
                m_addButtons.add(std::move(adder));

                x += 15;
            }
            x += 5; // final padding
        }

        setSize(x, height);
    }

    juce::OwnedArray<RackItemView> m_rackItems;
    juce::OwnedArray<AddButton> m_addButtons;
    RackView &m_owner;
};

class RackView::PluginListPanelComponent : public juce::Component
{
public:
    explicit PluginListPanelComponent(RackView &owner)
        : m_owner(owner)
    {
    }

    void paint(juce::Graphics &g) override
    {
        auto headerColour = m_owner.m_track != nullptr ? m_owner.m_track->getColour() : m_owner.m_evs.m_applicationState.getPrimeColour();
        GUIHelpers::drawHeaderBox(g, getLocalBounds().toFloat(), headerColour, m_owner.m_evs.m_applicationState.getBorderColour(), m_owner.m_evs.m_applicationState.getBackgroundColour2(), 20.0f, GUIHelpers::HeaderPosition::top, "Plugins");
    }

private:
    RackView &m_owner;
};

class RackPluginListItem
    : public juce::Component
    , public juce::DragAndDropTarget
{
public:
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

    bool isInterestedInDragSource(const SourceDetails &details) override { return details.description == "RackPluginListItem"; }

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
        if (m_selected == shouldBeSelected)
            return;

        m_selected = shouldBeSelected;
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
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
};

//==============================================================================
RackView::RackView(EditViewState &evs)
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

    m_pluginPanel->addAndMakeVisible(m_addPluginButton);
    m_addPluginButton.setButtonText("Add plugin");

    m_addPluginButton.onClick = [this] { addPluginAtCurrentPosition(); };

    addAndMakeVisible(m_modifierSidebar);
    addChildComponent(m_modifierDetailPanel); // Start hidden

    m_modifierSidebar.onModifierSelected = [this](te::Modifier::Ptr m)
    {
        m_modifierDetailPanel.setModifier(m);
        markAndUpdate(m_updateLayout);
    };
}

RackView::~RackView()
{
    for (auto &b : m_contentComp->m_addButtons)
    {
        b->removeListener(this);
    }

    detachTrackListeners();
    m_horizontalScrollBar.removeListener(this);
    m_pluginListViewport.setViewedComponent(nullptr, false);
}

void RackView::attachTrackListeners()
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

void RackView::detachTrackListeners()
{
    if (m_observedTrackState.isValid())
        m_observedTrackState.removeListener(this);

    if (m_observedPluginListState.isValid())
        m_observedPluginListState.removeListener(this);

    m_observedTrackState = {};
    m_observedPluginListState = {};
}

void RackView::paint(juce::Graphics &g)
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

void RackView::paintOverChildren(juce::Graphics &g)
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

                // Need bounds relative to THIS (RackView)
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

void RackView::mouseDown(const juce::MouseEvent &)
{
    // editViewState.selectionManager.selectOnly (track.get());
}

void RackView::mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel)
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

void RackView::resized()
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

    auto controls = listContentArea.removeFromTop(CONTROL_ROW_HEIGHT).reduced(4, 2);

    m_addPluginButton.setBounds(controls);

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

juce::StringArray RackView::getRackOrder() const
{
    auto state = m_evs.getTrackRackViewState(m_track->itemID);
    juce::String orderString = state.getProperty("rackItemOrder").toString();
    juce::StringArray order;
    order.addTokens(orderString, ",", "");
    return order;
}

void RackView::saveRackOrder(const juce::StringArray &order)
{
    auto state = m_evs.getTrackRackViewState(m_track->itemID);
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

int RackView::getPluginIndexForVisualIndex(int visualIndex) const
{
    auto order = getRackOrder();
    int targetPluginIndex = 0;
    for (int i = 0; i < visualIndex && i < order.size(); ++i)
    {
        if (getPluginFromList(m_track->pluginList, te::EditItemID::fromVar(order[i])))
            targetPluginIndex++;
    }
    return targetPluginIndex;
}

void RackView::ensureRackOrderConsistency()
{
    auto currentOrder = getRackOrder();
    juce::StringArray newOrder;

    // 1. Keep existing items if they still exist and are not hidden
    for (auto idStr : currentOrder)
    {
        auto id = te::EditItemID::fromVar(idStr);
        if (auto p = getPluginFromList(m_track->pluginList, id))
        {
            if (!isPluginHidden(*m_track, p.get()))
                newOrder.add(idStr);
        }
        else if (auto *ml = m_track->getModifierList())
        {
            if (te::findModifierForID(*ml, id) != nullptr)
            {
                newOrder.add(idStr);
            }
        }
    }

    // 2. Add any new items not in the list
    for (auto *p : m_track->getAllPlugins())
    {
        if (isPluginHidden(*m_track, p))
            continue;

        if (!newOrder.contains(p->itemID.toString()))
            newOrder.add(p->itemID.toString());
    }

    if (auto *ml = m_track->getModifierList())
    {
        for (auto m : ml->getModifiers())
        {
            if (!newOrder.contains(m->itemID.toString()))
                newOrder.add(m->itemID.toString());
        }
    }

    if (newOrder != currentOrder)
        saveRackOrder(newOrder);
}

void RackView::moveItem(RackItemView *item, int targetIndex)
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

        if (targetIndex >= order.size())
            order.add(id.toString());
        else
            order.insert(targetIndex, id.toString());

        saveRackOrder(order);

        // Sync PluginList order
        if (item->getPlugin())
        {
            int targetPluginIndex = getPluginIndexForVisualIndex(targetIndex);

            m_track->pluginList.insertPlugin(item->getPlugin(), targetPluginIndex, nullptr);
        }

        rebuildView();
    }
}

void RackView::buttonClicked(juce::Button *button)
{
    for (auto &b : m_contentComp->m_addButtons)
    {
        if (b == button)
        {
            int visualIndex = m_contentComp->m_addButtons.indexOf(b);

            juce::PopupMenu m;
            m.addItem(1, "Plugins...");

            int result = m.showAt(button);

            if (result == 1)
            {
                if (auto plugin = showMenuAndCreatePlugin(m_track->edit))
                {
                    insertPluginAtVisualIndex(plugin, visualIndex, true);
                }
            }

            m_evs.m_selectionManager.selectOnly(m_track);
        }
    }
}

void RackView::setTrack(te::Track::Ptr track)
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

void RackView::clearTrack()
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

void RackView::updateTrackPresetManager()
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

juce::String RackView::getCurrentTrackID() { return m_trackID; }

juce::OwnedArray<AddButton> &RackView::getAddButtons() { return m_contentComp->m_addButtons; }

juce::OwnedArray<RackItemView> &RackView::getPluginComponents() { return m_contentComp->m_rackItems; }

void RackView::insertPluginAtVisualIndex(te::Plugin::Ptr plugin, int visualIndex, bool selectInserted)
{
    if (m_track == nullptr || plugin == nullptr)
        return;

    ensureRackOrderConsistency();
    auto order = getRackOrder();

    const int clampedVisualIndex = juce::jlimit(0, order.size(), visualIndex);
    const int targetPluginIndex = getPluginIndexForVisualIndex(clampedVisualIndex);

    EngineHelpers::insertPluginWithPreset(m_evs, m_track, plugin, targetPluginIndex);
    order.insert(clampedVisualIndex, plugin->itemID.toString());
    saveRackOrder(order);

    if (selectInserted)
    {
        m_selectedRackItemID = plugin->itemID;
        m_scrollToSelectedAfterRebuild = true;
    }
}

void RackView::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &c)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void RackView::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &c, int)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void RackView::valueTreeChildOrderChanged(juce::ValueTree &c, int, int)
{
    if (c.hasType(te::IDs::PLUGIN))
        markAndUpdate(m_updatePlugins);
}

void RackView::handleAsyncUpdate()
{
    if (compareAndReset(m_updatePlugins))
        rebuildView();

    if (compareAndReset(m_updateLayout))
    {
        resized();
        repaint();
    }
}

void RackView::rebuildView()
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
                auto view = std::make_unique<RackItemView>(m_evs, m_track, p);
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

            auto safeRack = juce::Component::SafePointer<RackView>(this);
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

void RackView::rebuildPluginList()
{
    m_pluginListButtons.clear();

    for (int i = 0; i < m_contentComp->m_rackItems.size(); ++i)
    {
        auto *item = m_contentComp->m_rackItems[i];
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
        button->onClick = [this, i] { selectRackItemByIndex(i); };
        button->onReorder = [this](te::EditItemID sourceID, te::EditItemID targetID, bool placeAfter) { reorderPluginListItem(sourceID, targetID, placeAfter); };
        auto safeRack = juce::Component::SafePointer<RackView>(this);

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
    }
}

int RackView::getRackItemIndexForID(te::EditItemID id) const
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

void RackView::reorderPluginListItem(te::EditItemID sourceID, te::EditItemID targetID, bool placeAfter)
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

void RackView::selectRackItemByIndex(int index)
{
    if (index < 0 || index >= m_contentComp->m_rackItems.size())
        return;

    if (auto *item = m_contentComp->m_rackItems[index])
    {
        if (auto plugin = item->getPlugin())
            m_selectedRackItemID = plugin->itemID;

        for (int i = 0; i < m_pluginListButtons.size(); ++i)
            if (auto *listItem = static_cast<RackPluginListItem *>(m_pluginListButtons[i]))
                listItem->setSelected(i == index);

        animateScrollToX(getTargetScrollXForItem(*item));
        repaint();
    }
}

int RackView::getSelectedRackItemIndex() const
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

void RackView::layoutSelectedRackItem()
{
    for (auto *item : m_contentComp->m_rackItems)
        item->setVisible(true);

    m_contentComp->refreshButtonsAndLayout();
}

void RackView::updateRackContentPosition() { m_contentComp->setTopLeftPosition(-m_contentScrollX, 0); }

int RackView::getLastPluginLeftEdgeX() const
{
    int leftEdge = 0;
    for (auto *item : m_contentComp->m_rackItems)
        leftEdge = juce::jmax(leftEdge, item->getX());

    return leftEdge;
}

int RackView::getMaxContentScrollX() const { return juce::jmax(0, getLastPluginLeftEdgeX()); }

void RackView::updateHorizontalScrollBar()
{
    const auto visibleWidth = juce::jmax(0, m_pluginCanvas.getWidth());
    const auto totalWidth = juce::jmax(visibleWidth, getMaxContentScrollX() + visibleWidth);

    m_updatingHorizontalScrollBar = true;
    m_horizontalScrollBar.setRangeLimits({0.0, (double)totalWidth}, juce::dontSendNotification);
    m_horizontalScrollBar.setCurrentRange({(double)m_contentScrollX, (double)(m_contentScrollX + visibleWidth)}, juce::dontSendNotification);
    m_updatingHorizontalScrollBar = false;
}

int RackView::getTargetScrollXForItem(const RackItemView &item) const
{
    const int maxX = getMaxContentScrollX();
    const int targetX = item.getX();
    return juce::jlimit(0, maxX, targetX);
}

void RackView::animateScrollToX(int targetX)
{
    m_targetContentScrollX = (double)juce::jlimit(0, getMaxContentScrollX(), targetX);
    startTimerHz(60);
}

void RackView::timerCallback()
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

void RackView::scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved != &m_horizontalScrollBar || m_updatingHorizontalScrollBar)
        return;

    stopTimer();
    m_contentScrollX = juce::jlimit(0, getMaxContentScrollX(), (int)std::round(newRangeStart));
    m_targetContentScrollX = (double)m_contentScrollX;
    updateRackContentPosition();
    updateHorizontalScrollBar();
}

void RackView::addPluginAtCurrentPosition()
{
    if (m_track == nullptr)
        return;

    if (auto plugin = showMenuAndCreatePlugin(m_track->edit))
    {
        int insertVisualIndex = m_contentComp->m_rackItems.size();
        int selected = getSelectedRackItemIndex();
        if (selected >= 0)
            insertVisualIndex = selected + 1;

        insertPluginAtVisualIndex(plugin, insertVisualIndex, true);
    }
}

bool RackView::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry" || dragSourceDetails.description == "Instrument or Effect" || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        return true;
    }
    return false;
}

void RackView::itemDragMove(const SourceDetails &dragSourceDetails)
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

void RackView::itemDragExit(const SourceDetails &details)
{
    m_isOver = false;

    if (details.description != te::AutomationDragDropTarget::automatableDragString)
        m_dragSource = nullptr;

    repaint();
}

void RackView::itemDropped(const juce::DragAndDropTarget::SourceDetails &details)
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
                m_selectedRackItemID = plugin->itemID;
                m_scrollToSelectedAfterRebuild = true;
                EngineHelpers::insertPluginWithPreset(m_evs, track, plugin);
            }
    if (details.description == "Instrument or Effect")
        if (auto lb = dynamic_cast<InstrumentEffectTable *>(details.sourceComponent.get()))
            if (auto plugin = lb->getSelectedPlugin(m_evs.m_edit))
            {
                m_selectedRackItemID = plugin->itemID;
                m_scrollToSelectedAfterRebuild = true;
                EngineHelpers::insertPluginWithPreset(m_evs, track, plugin);
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
                // Find RackView via hierarchy search because AddButton is now inside RackContentComponent
                auto pluginRackComp = findParentComponentOfClass<RackView>();
                if (pluginRackComp)
                {
                    // Calculate the visual index where the user dropped the item
                    int visualIndex = pluginRackComp->getAddButtons().indexOf(this);

                    auto plugin = lbm->getSelectedPlugin(m_track->edit);
                    if (plugin)
                    {
                        pluginRackComp->insertPluginAtVisualIndex(plugin, visualIndex, true);
                    }
                }
            }
        }
    }

    if (dragSourceDetails.description == "PluginComp" || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        auto pluginRackComp = findParentComponentOfClass<RackView>();
        if (pluginRackComp)
        {
            auto *view = dynamic_cast<RackItemView *>(dragSourceDetails.sourceComponent.get());
            if (view)
            {
                int targetIndex = pluginRackComp->getAddButtons().indexOf(this);
                pluginRackComp->moveItem(view, targetIndex);
            }
        }
    }
}

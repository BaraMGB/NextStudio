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

#include "LowerRange/Mixer/MixerChannelStripComponent.h"
#include "LowerRange/PluginChain/PresetManagerComponent.h"
#include "LowerRange/PluginChain/TrackPresetAdapter.h"
#include "UI/ModifierDetailPanel.h"
#include "UI/ModifierSidebar.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"
// #include "SideBrowser/PluginBrowser.h"

namespace te = tracktion_engine;

class AddButton;
class RackPluginListItem;

class PluginChainView
    : public juce::Component
    , private FlaggedAsyncUpdater
    , private te::ValueTreeAllEventListener
    , public juce::Button::Listener
    , public juce::ScrollBar::Listener
    , public juce::DragAndDropTarget
    , private juce::Timer
{
public:
    PluginChainView(EditViewState &);
    ~PluginChainView() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override;
    void resized() override;
    void buttonClicked(juce::Button *button) override;
    void scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;

    void setTrack(te::Track::Ptr track);
    void clearTrack();
    juce::String getCurrentTrackID();

    juce::OwnedArray<AddButton> &getAddButtons();
    juce::OwnedArray<PluginChainItemView> &getPluginComponents();
    void insertPluginAtVisualIndex(te::Plugin::Ptr plugin, int visualIndex, bool selectInserted);

    void ensureRackOrderConsistency();
    juce::StringArray getRackOrder() const;
    void saveRackOrder(const juce::StringArray &order);
    void moveItem(PluginChainItemView *item, int targetIndex);
    int getPluginIndexForVisualIndex(int visualIndex) const;

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails & /*dragSourceDetails*/) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    void clearDragSource()
    {
        m_dragSource = nullptr;
        repaint();
    }

    EditViewState &getEditViewState() { return m_evs; }

private:
    void attachTrackListeners();
    void detachTrackListeners();

    void valueTreeChanged() override {}
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    void handleAsyncUpdate() override;

    void rebuildView();
    void updateTrackPresetManager();
    void rebuildPluginList();
    void selectRackItemByIndex(int index);
    void layoutSelectedRackItem();
    void updateRackContentPosition();
    void updateHorizontalScrollBar();
    int getLastPluginLeftEdgeX() const;
    int getMaxContentScrollX() const;
    int getTargetScrollXForItem(const PluginChainItemView &item) const;
    void animateScrollToX(int targetX);
    void addPluginAtCurrentPosition();
    void reorderPluginListItem(te::EditItemID sourceID, te::EditItemID targetID, bool placeAfter);
    int getRackItemIndexForID(te::EditItemID id) const;
    int getSelectedRackItemIndex() const;
    void timerCallback() override;

    EditViewState &m_evs;
    te::Track::Ptr m_track;
    juce::ValueTree m_observedTrackState;
    juce::ValueTree m_observedPluginListState;
    juce::Label m_nameLabel;
    juce::String m_trackID{""};

    class RackContentComponent;
    class PluginListPanelComponent;
    std::unique_ptr<RackContentComponent> m_contentComp;
    std::unique_ptr<PluginListPanelComponent> m_pluginPanel;
    juce::Component m_pluginCanvas;
    juce::ScrollBar m_horizontalScrollBar{false};

    ModifierSidebar m_modifierSidebar;
    ModifierDetailPanel m_modifierDetailPanel;
    std::unique_ptr<TrackPresetAdapter> m_trackPresetAdapter;
    std::unique_ptr<PresetManagerComponent> m_trackPresetManager;
    std::unique_ptr<MixerChannelStripComponent> m_channelStrip;

    juce::TextButton m_addPluginButton{"Add plugin"};
    juce::Component m_pluginListContent;
    juce::Viewport m_pluginListViewport;
    juce::OwnedArray<RackPluginListItem> m_pluginListButtons;
    te::EditItemID m_selectedRackItemID;
    bool m_scrollToSelectedAfterRebuild = false;
    int m_contentScrollX = 0;
    double m_targetContentScrollX = 0.0;
    bool m_updatingHorizontalScrollBar = false;

    bool m_updatePlugins = false;
    bool m_updateLayout = false;
    bool m_isOver = false;
    juce::Component::SafePointer<juce::Component> m_dragSource;

    te::EditItemID m_id;
    const int HEADERWIDTH = 20;
    static constexpr int CHANNEL_STRIP_WIDTH = 95;
    static constexpr int HORIZONTAL_SCROLLBAR_HEIGHT = 20;
    static constexpr int PLUGIN_LIST_WIDTH = 220;
    static constexpr int MODIFIER_STACK_WIDTH = 170;
    static constexpr int PLUGIN_LIST_ROW_HEIGHT = 24;
    static constexpr int CONTROL_ROW_HEIGHT = 28;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainView)
};

class AddButton
    : public juce::TextButton
    , public juce::DragAndDropTarget
{
public:
    AddButton(te::Track::Ptr track, ApplicationViewState &appState)
        : m_track(track),
          m_appState(appState)
    {
    }
    inline bool isInterestedInDragSource(const SourceDetails & /*dragSourceDetails*/) override { return true; }
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    void itemDragMove(const SourceDetails &dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginComp" || dragSourceDetails.description == "PluginListEntry")
        {
            isOver = true;
        }
        repaint();
    }

    void itemDragExit(const SourceDetails & /*dragSourceDetails*/) override
    {
        isOver = false;
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        auto cornerSize = 5.f;
        auto area = getLocalBounds().toFloat();
        area.reduce(0, 1);
        auto borderRect = area;

        auto backgroundColour = m_appState.getButtonBackgroundColour();
        if (isOver)
            backgroundColour = backgroundColour.brighter(0.4f);

        g.setColour(backgroundColour);
        GUIHelpers::drawRoundedRectWithSide(g, area, cornerSize, false, true, false, true);

        g.setColour(m_appState.getButtonTextColour());
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, false);

        g.setColour(m_appState.getBorderColour());
        GUIHelpers::strokeRoundedRectWithSide(g, borderRect, cornerSize, false, true, false, true);
    }

    void setPlugin(te::Plugin::Ptr pln) { plugin = std::move(pln); }

    te::Plugin::Ptr plugin{nullptr};

private:
    bool isOver{false};
    te::Track::Ptr m_track;
    ApplicationViewState &m_appState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AddButton)
};

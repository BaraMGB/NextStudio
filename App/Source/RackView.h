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

#include "EditViewState.h"
#include "ModifierDetailPanel.h"
#include "ModifierSidebar.h"
#include "Utilities.h"
// #include "PluginBrowser.h"

namespace te = tracktion_engine;

class AddButton;

class RackView
    : public juce::Component
    , private FlaggedAsyncUpdater
    , private te::ValueTreeAllEventListener
    , public juce::Button::Listener
    , public juce::DragAndDropTarget
{
public:
    RackView(EditViewState &);
    ~RackView() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void resized() override;
    void buttonClicked(juce::Button *button) override;

    void setTrack(te::Track::Ptr track);
    void clearTrack();
    juce::String getCurrentTrackID();

    juce::OwnedArray<AddButton> &getAddButtons();
    juce::OwnedArray<RackItemView> &getPluginComponents();

    void ensureRackOrderConsistency();
    juce::StringArray getRackOrder() const;
    void saveRackOrder(const juce::StringArray &order);
    void moveItem(RackItemView *item, int targetIndex);
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
    void valueTreeChanged() override {}
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    void handleAsyncUpdate() override;

    void rebuildView();

    EditViewState &m_evs;
    te::Track::Ptr m_track;
    juce::Label m_nameLabel;
    juce::String m_trackID{""};

    class RackContentComponent;
    std::unique_ptr<RackContentComponent> m_contentComp;
    juce::Viewport m_viewport;

    ModifierSidebar m_modifierSidebar;
    ModifierDetailPanel m_modifierDetailPanel;

    bool m_updatePlugins = false;
    bool m_isOver = false;
    juce::Component::SafePointer<juce::Component> m_dragSource;

    te::EditItemID m_id;
    const int HEADERWIDTH = 20;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackView)
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
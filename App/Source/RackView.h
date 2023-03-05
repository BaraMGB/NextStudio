#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "SideBarBrowser.h"

namespace te = tracktion_engine;

class AddButton;

class RackView : public juce::Component,
                            private FlaggedAsyncUpdater,
                            private te::ValueTreeAllEventListener,
                            public juce::Button::Listener,
                            public juce::DragAndDropTarget
{
public:
    RackView (EditViewState&);
    ~RackView() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

    void setTrack(te::Track::Ptr track);
    void clearTrack();
    juce::String getCurrentTrackID();

    juce::OwnedArray<AddButton> & getAddButtons();
    juce::OwnedArray<RackItemView> & getPluginComponents();

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& /*dragSourceDetails*/) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

private:
    void valueTreeChanged() override {}
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;

    void handleAsyncUpdate() override;

    void rebuildView();

    EditViewState& m_evs;
    te::Track::Ptr m_track;
    juce::Label m_nameLabel;
    juce::String m_trackID{""};

    juce::OwnedArray<RackItemView> m_rackItems;
    
    juce::OwnedArray<AddButton> m_addButtons;

    bool m_updatePlugins = false;
    bool m_isOver = false;
    te::EditItemID m_id;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackView)
};

class AddButton : public juce::TextButton
                , public juce::DragAndDropTarget
{
public:
    AddButton(te::Track::Ptr track) : m_track(track) {}
    inline bool isInterestedInDragSource (const SourceDetails& /*dragSourceDetails*/) override { return true; }
    void itemDropped(const SourceDetails& dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginListEntry")
        {
            if (auto listbox = dynamic_cast<juce::ListBox*>(dragSourceDetails.sourceComponent.get ()))
            {
                if (auto lbm = dynamic_cast<PluginListBoxComponent*> (listbox->getModel ()))
                {
                    auto pluginRackComp = dynamic_cast<RackView*>(getParentComponent());
                    if (pluginRackComp)
                    {
                        EngineHelpers::insertPlugin (m_track,
                                                     lbm->getSelectedPlugin(),
                                                     pluginRackComp->getAddButtons ().indexOf (this));
                    }
                }

            }
        }


        if (dragSourceDetails.description == "PluginComp")
        {
            auto pluginRackComp = dynamic_cast<RackView*>(getParentComponent());
            if (pluginRackComp)
            {
                for (auto & pluginComp : pluginRackComp->getPluginComponents())
                {
                    if (pluginComp == dragSourceDetails.sourceComponent)
                    {
                        auto sourceIndex = m_track->getAllPlugins().indexOf(pluginComp->getPlugin());
                        auto plugToMove = pluginComp->getPlugin();
                        auto targetIndex = pluginRackComp->getAddButtons().indexOf(this);

                        targetIndex = targetIndex > sourceIndex ? targetIndex - 1 : targetIndex;

                        plugToMove->deleteFromParent();
                        m_track->pluginList.insertPlugin(plugToMove, targetIndex,nullptr);
                    }
                }
            }
        }
    }
    
    
    void itemDragMove(const SourceDetails& dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginComp"
            || dragSourceDetails.description == "PluginListEntry")
        {
            isOver = true;
        }
        repaint();
    }
    
    void itemDragExit (const SourceDetails& /*dragSourceDetails*/) override
    {
        isOver = false;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (isOver)
        {
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setColour(juce::Colours::grey);
        }
        int cornerSize = 5;
        auto area = getLocalBounds().toFloat();
        g.fillRoundedRectangle(area, (float) cornerSize);
        area.reduce(1,1);
        g.setColour(juce::Colour(0xff1b1b1b));

        g.drawText(getButtonText(),getLocalBounds(),juce::Justification::centred
                   , false);
    }

    void setPlugin(te::Plugin::Ptr pln)
    {
        plugin = std::move(pln);
    }

    te::Plugin::Ptr plugin {nullptr};
private:
    bool isOver {false};
    te::Track::Ptr m_track;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AddButton)
};



#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
class AddButton;

namespace te = tracktion_engine;


class PluginRackComponent : public Component,
                             private FlaggedAsyncUpdater,
                             private te::ValueTreeAllEventListener,
                             public juce::Button::Listener
{
public:
    PluginRackComponent (EditViewState&, te::Track::Ptr);
    ~PluginRackComponent();

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    void resized() override;
    void buttonClicked(Button* button) override;

    OwnedArray<AddButton> & getAddButtons()
    {
        return addButtons;
    }

    tracktion_engine::Track::Ptr getTrack()
    {
        return track;
    }

    OwnedArray<PluginWindowComponent> & getPluginComponents()
    {
        return plugins;
    }

    void buildPlugins();
private:
    void valueTreeChanged() override {}
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;

    void handleAsyncUpdate() override;


    EditViewState& editViewState;
    te::Track::Ptr track;

    OwnedArray<PluginWindowComponent> plugins;
    
    OwnedArray<AddButton> addButtons;

    bool updatePlugins = false;
};

class AddButton : public TextButton
                , public DragAndDropTarget
{
public:

    inline bool isInterestedInDragSource (const SourceDetails& /*dragSourceDetails*/) override { return true; }
    void itemDropped(const SourceDetails& dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginComp")
        {
            auto pluginRackComp = dynamic_cast<PluginRackComponent*>(getParentComponent());
            if (pluginRackComp)
            {
                auto sourceIndex = 0;
                for (auto & pluginComp : pluginRackComp->getPluginComponents())
                {
                    if (pluginComp == dragSourceDetails.sourceComponent)
                    {
                        sourceIndex = pluginRackComp->getTrack()->getAllPlugins().indexOf(pluginComp->getPlugin());
                        auto plugToMove = pluginComp->getPlugin();
                        auto targetIndex = pluginRackComp->getAddButtons().indexOf(this);

                        targetIndex = targetIndex > sourceIndex ? targetIndex - 1 : targetIndex;

                        plugToMove->deleteFromParent();
                        pluginRackComp->getTrack()->pluginList.insertPlugin(plugToMove, targetIndex,nullptr);
                    }
                }
            }
        }
    }
    
    
    void itemDragMove(const SourceDetails& dragSourceDetails) override
    {
        if (dragSourceDetails.description == "PluginComp")
        {
            isOver = true;
        }
        repaint();
    }
    
    void itemDragExit (const SourceDetails& dragSourceDetails) override
    {
        isOver = false;
        repaint();
    }

    void paint(Graphics& g) override
    {
        if (isOver)
        {
            g.setColour(Colours::white);
        }
        else
        {
            g.setColour(Colours::grey);
        }
        int cornerSize = 5;
        auto area = getLocalBounds().toFloat();
        g.fillRoundedRectangle( area, cornerSize);
        area.reduce(1,1);
        g.setColour(Colour(0xff1b1b1b));

        g.drawText(getButtonText(),getLocalBounds(),juce::Justification::centred
                   , false);
    }

    void setPlugin(te::Plugin::Ptr pln)
    {
        plugin = pln;
    }

    te::Plugin::Ptr plugin {nullptr};
private:

    bool isOver {false};

};


class LowerRangeComponent : public Component
                          , public juce::ChangeListener
{
public:
    LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent();

    void changeListenerCallback (ChangeBroadcaster*) override;

    void paint (Graphics& g) override;
    void resized () override;

private:

    EditViewState& editViewState;

    tracktion_engine::Track::Ptr m_pointedTrack{nullptr};
    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;

};

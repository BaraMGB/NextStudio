#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditorComponent.h"

namespace te = tracktion_engine;

class AddButton;

class PluginRackComponent : public juce::Component,
                             private FlaggedAsyncUpdater,
                             private te::ValueTreeAllEventListener,
                             public juce::Button::Listener
{
public:
    PluginRackComponent (EditViewState&, te::Track::Ptr);
    ~PluginRackComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

    juce::OwnedArray<AddButton> & getAddButtons()
    {
        return addButtons;
    }

    tracktion_engine::Track::Ptr getTrack()
    {
        return track;
    }

    juce::OwnedArray<PluginWindowComponent> & getPluginComponents()
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

    juce::OwnedArray<PluginWindowComponent> plugins;
    
    juce::OwnedArray<AddButton> addButtons;

    bool updatePlugins = false;
};

class AddButton : public juce::TextButton
                , public juce::DragAndDropTarget
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
        g.fillRoundedRectangle(area, cornerSize);
        area.reduce(1,1);
        g.setColour(juce::Colour(0xff1b1b1b));

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


class LowerRangeComponent : public juce::Component
                          , public juce::ChangeListener
{
public:
    LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent();

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void paint (juce::Graphics& g) override;
    void resized () override;

    void showPluginRack(te::Track *);
    void showPianoRoll(tracktion_engine::MidiClip *);

private:

    EditViewState& editViewState;

    tracktion_engine::Track::Ptr m_pointedTrack{nullptr};
    te::MidiClip * m_pointedClip{nullptr};
    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;
    juce::OwnedArray<PianoRollComponent> m_pianoRollComps;
    const int m_splitterHeight {10};

};

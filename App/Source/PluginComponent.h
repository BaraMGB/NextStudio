#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "AutomatableSliderComponent.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PluginViewComponent : public juce::Component
{
public:
    PluginViewComponent (EditViewState&, te::Plugin::Ptr);


    [[nodiscard]] te::Plugin::Ptr getPlugin() const;

    void setPlugin(const te::Plugin::Ptr &getPlugin);
    virtual int getNeededWidth() {return 1;}

    juce::Colour getTrackColour() {m_plugin->getOwnerTrack()->getColour();}

private:

    EditViewState&     m_editViewState;
    te::Plugin::Ptr    m_plugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginViewComponent)
};

//------------------------------------------------------------------------------

class VolumePluginComponent : public PluginViewComponent
{
public:
    VolumePluginComponent (EditViewState&, te::Plugin::Ptr);


    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider       m_volumeKnob;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumePluginComponent)
};

//------------------------------------------------------------------------------

class ParameterComponent : public juce::Component
                         , public juce::ChangeBroadcaster
{
public:
    explicit ParameterComponent(te::AutomatableParameter& ap);
    ~ParameterComponent() override= default;

    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    te::AutomatableParameter& getParameter(){return m_parameter;}

private:

    te::AutomatableParameter& m_parameter;
    juce::Label m_parameterName;
    AutomatableSliderComponent m_parameterSlider;
    [[maybe_unused]] bool m_updateKnob {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterComponent)


};

class VstPluginComponent : public PluginViewComponent
                         , public juce::ChangeListener
{
public:
    VstPluginComponent (EditViewState&, te::Plugin::Ptr);
    ~VstPluginComponent() override;

    int getNeededWidth() override {return 2;}

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void mouseDown(const juce::MouseEvent& ) override
    {
    }

    void resized() override;

private:

    juce::OwnedArray<ParameterComponent> m_parameterComponents;
    std::unique_ptr<ParameterComponent> m_lastChangedParameterComponent;
    juce::Viewport m_viewPort;
    juce::Component m_pluginListComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VstPluginComponent)
};

//-------------------------------------------------------------------------------------

class PluginWindowComponent : public juce::Component
{
public:
    PluginWindowComponent (EditViewState&, te::Plugin::Ptr);
    ~PluginWindowComponent() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void resized() override;

    juce::Colour getTrackColour() {plugin->getOwnerTrack()->getColour();}
    int getNeededWidthFactor() { return m_pluginComponent->getNeededWidth();}
    [[maybe_unused]] void setNeededWidthFactor(int wf){ m_neededWidthFactor = wf; }
    te::Plugin::Ptr getPlugin()
    {
        return plugin;
    }



private:
    juce::Label name;
    int m_headerWidth {20};
    [[maybe_unused]] int m_neededWidthFactor {1};
    EditViewState& editViewState;
    te::Plugin::Ptr plugin;
    std::unique_ptr<PluginViewComponent> m_pluginComponent;

    bool m_clickOnHeader {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindowComponent)
};

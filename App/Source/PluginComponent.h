#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PluginViewComponent : public juce::Component
{
public:
    PluginViewComponent (EditViewState&, te::Plugin::Ptr, juce::Colour);


    te::Plugin::Ptr getPlugin() const;

    void setPlugin(const te::Plugin::Ptr &getPlugin);
    virtual const int getNeededWidth() {return 1;}


private:
    EditViewState&     m_editViewState;
    te::Plugin::Ptr    m_plugin;
    juce::Colour       m_trackColour;
};

//------------------------------------------------------------------------------

class VolumePluginComponent : public PluginViewComponent
{
public:
    VolumePluginComponent (EditViewState&, te::Plugin::Ptr, juce::Colour);


    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider       m_volumeKnob;
};

//------------------------------------------------------------------------------

class ParameterComponent : public juce::Component
                         , public juce::ChangeBroadcaster
{
public:
    ParameterComponent(te::AutomatableParameter& ap)
        : m_parameter(ap)
    {
        m_parameterName.setText(ap.getParameterName(),
                                juce::NotificationType::dontSendNotification);

        m_parameterSlider.setOpaque(false);
        addAndMakeVisible(m_parameterName);
        addAndMakeVisible(m_parameterSlider);
        m_parameterSlider.setRange(0.0f, 3.0f, 0.01f);
        m_parameterSlider.setSkewFactorFromMidPoint(1.0f);
        m_parameterSlider.setValue(ap.getCurrentValue());
        m_parameterSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        m_parameterSlider.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
        m_parameterSlider.onValueChange = [this, &ap]
        {
            if ((float) m_parameterSlider.getValue()
             != (float) ap.getCurrentValue())
            {
                ap.setParameter(m_parameterSlider.getValue(),
                                juce::NotificationType::dontSendNotification);
                sendChangeMessage();
            }
        };
    }

    void updateValue()
    {
        m_parameterSlider.setValue(getParameter().getCurrentValue());
    }

    void resized() override
    {
        auto area = getLocalBounds();

        m_parameterSlider.setBounds(area.removeFromLeft(area.getHeight()));
        m_parameterName.setBounds(area);
    }

    te::AutomatableParameter & getParameter()
    {
        return m_parameter;
    }


private:
    te::AutomatableParameter & m_parameter;
    juce::Label m_parameterName;
    juce::Slider m_parameterSlider;
};


class VstPluginComponent : public PluginViewComponent
                         , public tracktion_engine::AutomatableParameter::Listener
                         , public juce::ChangeListener
{

public:
    VstPluginComponent (EditViewState&, te::Plugin::Ptr, juce::Colour);
    ~VstPluginComponent();

    const int getNeededWidth() override {return 2;}

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void mouseDown(const juce::MouseEvent& ) override
    {
    }

    void curveHasChanged (tracktion_engine::AutomatableParameter& ap) override
    {
    }

    void parameterChanged (tracktion_engine::AutomatableParameter&ap , float /*newValue*/) override;
   // void paint (juce::Graphics&) override;
    void resized();

private:

    juce::OwnedArray<ParameterComponent> m_parameterComponents;
    std::unique_ptr<ParameterComponent> m_lastChangedParameterComponent;
    juce::Viewport m_viewPort;
    juce::Component m_pluginListComponent;
};

//-------------------------------------------------------------------------------------

class PluginWindowComponent : public juce::Component
{
public:
    PluginWindowComponent (EditViewState&, te::Plugin::Ptr, juce::Colour);
    ~PluginWindowComponent();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void resized() override;

    int getNeededWidthFactor() { return m_pluginComponent->getNeededWidth();}
    void setNeededWidthFactor(int wf){ m_neededWidthFactor = wf; }
    te::Plugin::Ptr getPlugin()
    {
        return plugin;
    }

private:
    juce::Label name;
    int m_headerWidth {20};
    int m_neededWidthFactor {1};
    EditViewState& editViewState;
    te::Plugin::Ptr plugin;
    std::unique_ptr<PluginViewComponent> m_pluginComponent;
    juce::Colour m_trackColour;

    bool m_clickOnHeader {false};
};


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
#include "AutomatableSliderComponent.h"
#include "Utilities.h"
#include "Browser_Base.h"
#include "PluginViewComponent.h"

namespace te = tracktion_engine;


class BorderlessButton : public juce::DrawableButton

{
public:
    BorderlessButton(const juce::String& buttonName
                   , juce::DrawableButton::ButtonStyle buttonStyle) 
    : juce::DrawableButton(buttonName, buttonStyle) {   }
        
    void paint (juce::Graphics& g) override {}
};

class ParameterComponent : public juce::Component
{
public:
    explicit ParameterComponent(te::AutomatableParameter& ap);
    ~ParameterComponent() override= default;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool isDragged() { return m_isDragged; }
    te::AutomatableParameter& getParameter(){return m_parameter;}

private:

    te::AutomatableParameter& m_parameter;
    juce::Label m_parameterName;
    AutomatableSliderComponent m_parameterSlider;

    bool m_isDragged {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterComponent)


};


//------------------------------------------------------------------------------

class FilterPluginComponent : public PluginViewComponent
                              , private te::ValueTreeAllEventListener
{
public:
    FilterPluginComponent (EditViewState&, te::Plugin::Ptr);
    ~FilterPluginComponent()
    {
        m_plugin->state.removeListener(this);
    }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateLabel( juce::UndoManager & um);
    void valueTreeChanged () override {}
    void valueTreePropertyChanged (juce::ValueTree& v
                                   , const juce::Identifier& i) override
    {
        if (i == te::IDs::frequency )
        {
            m_freqPar->updateLabel();
        }
    }
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override{}
 
    std::unique_ptr<AutomatableParameterComponent>    m_freqPar;
    juce::ToggleButton                                m_modeButton;
    juce::Label                                       m_modeLabel;
    std::unique_ptr<te::LowPassPlugin>                m_filterPlugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPluginComponent)
};
class EqPluginComponent : public PluginViewComponent
                              , private te::ValueTreeAllEventListener
{
public:
    EqPluginComponent (EditViewState&evs, te::Plugin::Ptr p)
        : PluginViewComponent(evs, p)
    {
        m_lowFreqComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass freq"), "Freq");
        m_lowGainComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass gain"), "Gain");
        m_lowQComp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Low-pass Q"), "Q");
        m_midFreq1Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid freq 1"), "Freq");
        m_midGain1Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid gain 1"), "Gain");
        m_midQ1Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid Q 1"), "Q");
        m_midFreq2Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid freq 2"), "Freq");
        m_midGain2Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid gain 2"), "Gain");
        m_midQ2Comp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("Mid Q 2"), "Q");
        m_hiFreqComp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass freq"), "Freq");
        m_hiGainComp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass gain"), "Gain");
        m_hiQComp=std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("High-pass Q"), "Q");

        addAndMakeVisible (*m_lowFreqComp);
        addAndMakeVisible (* m_lowGainComp);
        addAndMakeVisible (* m_lowQComp);
        addAndMakeVisible (*m_midFreq1Comp);
        addAndMakeVisible (*m_midGain1Comp);
        addAndMakeVisible (*m_midQ1Comp);
        addAndMakeVisible (*m_midFreq2Comp);
        addAndMakeVisible (*m_midGain2Comp);
        addAndMakeVisible (*m_midQ2Comp);
        addAndMakeVisible (*m_hiFreqComp);
        addAndMakeVisible (*m_hiGainComp);
        addAndMakeVisible (*m_hiQComp);

        m_plugin->state.addListener(this);
    }
    ~EqPluginComponent()
    {
        m_plugin->state.removeListener(this);
    }

    void paint (juce::Graphics&) override{}
    void resized() override
    {
        auto area = getLocalBounds();
        auto w = getWidth() /4;
        auto h = getHeight() / 12;
        auto low = area.removeFromLeft(w);
        auto mid1 = area.removeFromLeft(w);
        auto mid2 = area.removeFromLeft(w);
        auto hi = area.removeFromLeft(w);

        m_lowFreqComp->setBounds(low.removeFromTop(h*4));
        m_lowGainComp->setBounds(low.removeFromTop(h*4));
        m_lowQComp->setBounds(low.removeFromTop(h*4));
        m_midFreq1Comp->setBounds(mid1.removeFromTop(h*4));
        m_midGain1Comp->setBounds(mid1.removeFromTop(h*4));
        m_midQ1Comp->setBounds(mid1.removeFromTop(h*4));
        m_midFreq2Comp->setBounds(mid2.removeFromTop(h*4));
        m_midGain2Comp->setBounds(mid2.removeFromTop(h*4));
        m_midQ2Comp->setBounds(mid2.removeFromTop(h*4));
        m_hiFreqComp->setBounds(hi.removeFromTop(h*4));
        m_hiGainComp->setBounds(hi.removeFromTop(h*4));
        m_hiQComp->setBounds(hi.removeFromTop(h*4));
    }

    int getNeededWidth() override {return 4;}
private:
    void valueTreeChanged () override {}
    void valueTreePropertyChanged (juce::ValueTree& v
                                   , const juce::Identifier& i) override
    {     
       if (i == te::IDs::loFreq)
           m_lowFreqComp->updateLabel();
       if (i == te::IDs::loGain )
           m_lowGainComp->updateLabel();
       if (i == te::IDs::loQ)
           m_lowQComp->updateLabel();

       if (i == te::IDs::midFreq1)
           m_midFreq1Comp->updateLabel();
       if (i == te::IDs::midGain1)
           m_midGain1Comp->updateLabel();
       if (i == te::IDs::midQ1)
           m_midQ1Comp->updateLabel();

       if (i == te::IDs::midFreq2)
           m_midFreq2Comp->updateLabel();
       if (i == te::IDs::midGain2)
           m_midGain2Comp->updateLabel();
       if (i == te::IDs::midQ2)
           m_midQ2Comp->updateLabel();

       if (i == te::IDs::hiQ)
           m_hiQComp->updateLabel();
       if (i == te::IDs::hiFreq)
           m_hiFreqComp->updateLabel();
       if (i == te::IDs::hiGain )
           m_hiGainComp->updateLabel();
    }
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override{}
    std::unique_ptr<AutomatableParameterComponent>    
        m_lowFreqComp,
        m_lowGainComp,       
        m_lowQComp,
        m_midFreq1Comp,
        m_midGain1Comp,
        m_midQ1Comp,
        m_midFreq2Comp,
        m_midGain2Comp,
        m_midQ2Comp,
        m_hiFreqComp,
        m_hiGainComp,
        m_hiQComp;

   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqPluginComponent)
};


class DelayPluginComponent : public PluginViewComponent
                              , private te::ValueTreeAllEventListener
{
public:
    DelayPluginComponent (EditViewState& evs, te::Plugin::Ptr p): PluginViewComponent(evs, p)
{
        m_fbParCom =  std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
        addAndMakeVisible(*m_fbParCom);
        m_mix =  std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix proportion"), "Mix");
        addAndMakeVisible(*m_mix);
        
        m_time = std::make_unique<NonAutomatableParameterComponent>(m_plugin->state.getPropertyAsValue(te::IDs::length, &m_editViewState.m_edit.getUndoManager()),"Time", 0,1000);
    addAndMakeVisible(*m_time);
        m_plugin->state.addListener(this);

}

    ~DelayPluginComponent()
    {
        m_plugin->state.removeListener(this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto h = bounds.getHeight()/12;
        m_fbParCom->setBounds(bounds.removeFromTop(h*4));
        m_mix->setBounds(bounds.removeFromTop(h*4));
        m_time->setBounds(bounds.removeFromTop(h*4));
    }

private:
    void valueTreeChanged () override {}
    void valueTreePropertyChanged (juce::ValueTree& v
                                   , const juce::Identifier& i) override
    {
       if (i == te::IDs::feedback)
           m_fbParCom->updateLabel();
       if (i == te::IDs::mix )
           m_mix->updateLabel();
    }
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override{}
 
    std::unique_ptr<AutomatableParameterComponent>    m_mix, m_fbParCom;
    std::unique_ptr<NonAutomatableParameterComponent> m_time;
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayPluginComponent)
};


class VolumePluginComponent : public PluginViewComponent
                              , private te::ValueTreeAllEventListener
{
public:
    VolumePluginComponent (EditViewState&, te::Plugin::Ptr);
    ~VolumePluginComponent()
    {
        m_plugin->state.removeListener(this);
    }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void valueTreeChanged () override {}
    void valueTreePropertyChanged (juce::ValueTree& v
                                   , const juce::Identifier& i) override
    {
       if (i == te::IDs::pan)
           m_panParComp->updateLabel();
       if (i == te::IDs::volume )
           m_volParComp->updateLabel();
    }
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override{}
 
    std::unique_ptr<AutomatableParameterComponent>    m_volParComp, m_panParComp;
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumePluginComponent)
};

//------------------------------------------------------------------------------

class VstPluginComponent : public PluginViewComponent
                         , private te::AutomatableParameter::Listener
{
public:
    VstPluginComponent (EditViewState&, te::Plugin::Ptr);
    ~VstPluginComponent() override;


    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& ) override
    {
    }


    int getNeededWidth() override {return 2;}
private:

    void curveHasChanged(te::AutomatableParameter&) override{} 

    void parameterChanged (te::AutomatableParameter& param, float /*newValue*/) override;
    std::unique_ptr<ParameterComponent> m_lastChangedParameterComponent;
    juce::Viewport m_viewPort;
    juce::Component m_pluginListComponent;
    juce::OwnedArray<ParameterComponent> m_parameters;
    te::Plugin::Ptr m_plugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VstPluginComponent)
};

//-------------------------------------------------------------------------------------

class RackItemView : public juce::Component
                          , public juce::Button::Listener
{
public:
    RackItemView (EditViewState&, te::Plugin::Ptr);
    ~RackItemView() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void resized() override;

    void buttonClicked(juce::Button* button) override;

    int getNeededWidthFactor() { return m_pluginComponent->getNeededWidth();}
    [[maybe_unused]] void setNeededWidthFactor(int wf){ m_neededWidthFactor = wf; }
    te::Plugin::Ptr getPlugin()
    {
        return plugin;
    }

private:

    juce::Colour getTrackColour();

    juce::Label name;
    int m_headerWidth {20};
    [[maybe_unused]] int m_neededWidthFactor {1};
    EditViewState& editViewState;
    te::Plugin::Ptr plugin;
    std::unique_ptr<PluginViewComponent> m_pluginComponent;
    BorderlessButton   m_showPluginBtn;    
    bool m_clickOnHeader {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackItemView)
};

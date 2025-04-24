#pragma once

// #include "PluginComponent.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include "AutomatableSliderComponent.h"
#include "Utilities.h"
#include "PluginViewComponent.h"

namespace te = tracktion_engine;



class FilterComponent : public juce::Component
{
public:
    FilterComponent(te::FourOscPlugin& plugin)
        : m_plugin(plugin)
    {
        // Filter type combobox
        m_typeCombo.reset(new juce::ComboBox("Type"));
        m_typeCombo->addItem("Low Pass", 1);
        m_typeCombo->addItem("Band Pass", 2);
        m_typeCombo->addItem("High Pass", 3);
        m_typeCombo->setSelectedId(m_plugin.filterTypeValue.get() + 1, juce::dontSendNotification);
        m_typeCombo->onChange = [this] {
            m_plugin.filterTypeValue = m_typeCombo->getSelectedId() - 1;
        };
        addAndMakeVisible(*m_typeCombo);
        
        // Slope combobox
        m_slopeCombo.reset(new juce::ComboBox("Slope"));
        m_slopeCombo->addItem("12 dB", 1);
        m_slopeCombo->addItem("24 dB", 2);
        m_slopeCombo->setSelectedId(m_plugin.filterSlopeValue.get() + 1, juce::dontSendNotification);
        m_slopeCombo->onChange = [this] {
            m_plugin.filterSlopeValue = m_slopeCombo->getSelectedId() - 1;
        };
        addAndMakeVisible(*m_slopeCombo);
        
        // Parameters
        m_freqParamComp = std::make_unique<AutomatableParameterComponent>(m_plugin.filterFreq, "Freq");
        m_resParamComp = std::make_unique<AutomatableParameterComponent>(m_plugin.filterResonance, "Res");
        m_amountParamComp = std::make_unique<AutomatableParameterComponent>(m_plugin.filterAmount, "Amount");
        m_keyParamComp = std::make_unique<AutomatableParameterComponent>(m_plugin.filterKey, "Key");
        m_velocityParamComp = std::make_unique<AutomatableParameterComponent>(m_plugin.filterVelocity, "Vel");
        
        addAndMakeVisible(*m_freqParamComp);
        addAndMakeVisible(*m_resParamComp);
        addAndMakeVisible(*m_amountParamComp);
        addAndMakeVisible(*m_keyParamComp);
        addAndMakeVisible(*m_velocityParamComp);

    }
    
    ~FilterComponent() override = default;
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(juce::Colour(0xff333333));
        GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText("Filter", area.removeFromTop(20), juce::Justification::centred);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        area.removeFromTop(20); // Space for title
        
        // Top row with comboboxes
        auto topRow = area.removeFromTop(24);
        m_typeCombo->setBounds(topRow.removeFromLeft(topRow.getWidth() / 2).reduced(5, 0));
        m_slopeCombo->setBounds(topRow.reduced(5, 0));
        
        // First row of parameters
        auto paramRow1 = area.removeFromTop(area.getHeight() / 2);
        auto paramWidth = paramRow1.getWidth() / 3;
        
        m_freqParamComp->setBounds(paramRow1.removeFromLeft(paramWidth).reduced(2));
        m_resParamComp->setBounds(paramRow1.removeFromLeft(paramWidth).reduced(2));
        m_amountParamComp->setBounds(paramRow1.reduced(2));
        
        // Second row of parameters
        auto paramRow2 = area;
        paramWidth = paramRow2.getWidth() / 2;
        
        m_keyParamComp->setBounds(paramRow2.removeFromLeft(paramWidth).reduced(2));
        m_velocityParamComp->setBounds(paramRow2.reduced(2));
    }
    
private:
    te::FourOscPlugin& m_plugin;
    
    std::unique_ptr<juce::ComboBox> m_typeCombo;
    std::unique_ptr<juce::ComboBox> m_slopeCombo;
    
    std::unique_ptr<AutomatableParameterComponent> m_freqParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_resParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_amountParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_keyParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_velocityParamComp;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterComponent)
};
class OscComponent : public juce::Component
{
public:
    OscComponent(te::FourOscPlugin::OscParams& params, juce::Colour colorToUse)
        : m_params(params)
        , m_colour(colorToUse)
    {

        // Wave shape combobox
        m_waveShapeCombo.reset(new juce::ComboBox("Wave"));
        m_waveShapeCombo->addItem("None", 1);
        m_waveShapeCombo->addItem("Sine", 2);
        m_waveShapeCombo->addItem("Triangle", 5);
        m_waveShapeCombo->addItem("Saw", 4);
        m_waveShapeCombo->addItem("Square", 3);
        m_waveShapeCombo->addItem("Random", 6);
        m_waveShapeCombo->setSelectedId(m_params.waveShapeValue.get() + 1, juce::dontSendNotification);
        m_waveShapeCombo->onChange = [this] {
            if (m_waveShapeCombo->getSelectedId() > 0)
            {
               GUIHelpers::log("FourOscPluginComponent: WS nr: ", m_waveShapeCombo->getSelectedId()) ;
               m_params.waveShapeValue = m_waveShapeCombo->getSelectedId() - 1;
            }
            else
            {
               GUIHelpers::log("FourOscPluginComponent: ", "WaveShapeComboId == 0!!!") ;
            }
        };
        addAndMakeVisible(*m_waveShapeCombo);
        
        // Set up automatable parameter components
        m_tuneParamComp = std::make_unique<AutomatableParameterComponent>(m_params.tune, "Tune");
        m_fineTuneParamComp = std::make_unique<AutomatableParameterComponent>(m_params.fineTune, "Fine");
        m_levelParamComp = std::make_unique<AutomatableParameterComponent>(m_params.level, "Level");
        m_pulseWidthParamComp = std::make_unique<AutomatableParameterComponent>(m_params.pulseWidth, "Width");
        m_detuneParamComp = std::make_unique<AutomatableParameterComponent>(m_params.detune, "Detune");
        m_spreadParamComp = std::make_unique<AutomatableParameterComponent>(m_params.spread, "Spread");
        m_panParamComp = std::make_unique<AutomatableParameterComponent>(m_params.pan, "Pan");
        m_voicesParamComp = std::make_unique<NonAutomatableParameterComponent>(m_params.voicesValue.getPropertyAsValue(), "Voices", 1, 16);
        m_tuneParamComp->setKnobColour(m_colour);
        m_fineTuneParamComp->setKnobColour(m_colour);
        m_levelParamComp->setKnobColour(m_colour);
        m_pulseWidthParamComp->setKnobColour(m_colour);
        m_detuneParamComp->setKnobColour(m_colour);
        m_spreadParamComp->setKnobColour(m_colour);
        m_panParamComp->setKnobColour(m_colour);
        addAndMakeVisible(*m_voicesParamComp);
        addAndMakeVisible(*m_tuneParamComp);
        addAndMakeVisible(*m_fineTuneParamComp);
        addAndMakeVisible(*m_levelParamComp);
        addAndMakeVisible(*m_pulseWidthParamComp);
        addAndMakeVisible(*m_detuneParamComp);
        addAndMakeVisible(*m_spreadParamComp);
        addAndMakeVisible(*m_panParamComp);
    }
    
    ~OscComponent() override = default;
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        
        // Wave Shape at the top
        m_waveShapeCombo->setBounds(area.removeFromTop(24).reduced(5, 0));
        
        // Rest of controls in 2 rows
        auto rowHeight = area.getHeight() / 2;
        auto row1 = area.removeFromTop(rowHeight);
        auto row2 = area;
        
        int paramWidth = row1.getWidth() / 4;
        
        m_levelParamComp->setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        m_tuneParamComp->setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        m_fineTuneParamComp->setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        m_voicesParamComp->setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        
        paramWidth = row2.getWidth() / 4;
        m_pulseWidthParamComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
        m_detuneParamComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
        m_spreadParamComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
        m_panParamComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
    }
    
private:
    te::FourOscPlugin::OscParams& m_params;
    juce::Colour m_colour;
    std::unique_ptr<juce::ComboBox> m_waveShapeCombo;
    
    std::unique_ptr<AutomatableParameterComponent> m_tuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_fineTuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_levelParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_pulseWidthParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_detuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_spreadParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_panParamComp;
    std::unique_ptr<NonAutomatableParameterComponent> m_voicesParamComp;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscComponent)
};

//==============================================================================

class LFOComponent : public juce::Component
{
public:
    LFOComponent(te::FourOscPlugin::LFOParams& params)
        : m_params(params)
    {
        // Wave shape combobox
        m_waveShapeCombo.reset(new juce::ComboBox("Wave"));
        m_waveShapeCombo->addItem("None", 1);
        m_waveShapeCombo->addItem("Sine", 2);
        m_waveShapeCombo->addItem("Triangle", 3);
        m_waveShapeCombo->addItem("Saw Up", 4);
        m_waveShapeCombo->addItem("Saw Down", 5);
        m_waveShapeCombo->addItem("Square", 6);
        m_waveShapeCombo->addItem("Random", 7);
        m_waveShapeCombo->setSelectedId(m_params.waveShapeValue.get() + 1, juce::dontSendNotification);
        m_waveShapeCombo->onChange = [this] {
            m_params.waveShapeValue = m_waveShapeCombo->getSelectedId() - 1;
        };
        addAndMakeVisible(*m_waveShapeCombo);
        
        // Sync toggle
        m_syncButton.reset(new juce::ToggleButton("Sync"));
        m_syncButton->setToggleState(m_params.syncValue.get(), juce::dontSendNotification);
        m_syncButton->onClick = [this] {
            m_params.syncValue = m_syncButton->getToggleState();
        };
        addAndMakeVisible(*m_syncButton);
        
        // Parameter components
        m_rateParamComp = std::make_unique<AutomatableParameterComponent>(m_params.rate, "Rate");
        m_depthParamComp = std::make_unique<AutomatableParameterComponent>(m_params.depth, "Depth");
        
        addAndMakeVisible(*m_rateParamComp);
        addAndMakeVisible(*m_depthParamComp);
    }
    
    ~LFOComponent() override = default;
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(juce::Colour(0xff333333));
        GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        
        // Top row with combobox and sync button
        auto topRow = area.removeFromTop(24);
        m_waveShapeCombo->setBounds(topRow.removeFromLeft(topRow.getWidth() - 80).reduced(5, 0));
        m_syncButton->setBounds(topRow.reduced(5, 0));
        
        // Parameters
        auto paramWidth = area.getWidth() / 2;
        m_rateParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_depthParamComp->setBounds(area.reduced(2));
    }
    
private:
    te::FourOscPlugin::LFOParams& m_params;
    
    std::unique_ptr<juce::ComboBox> m_waveShapeCombo;
    std::unique_ptr<juce::ToggleButton> m_syncButton;
    
    std::unique_ptr<AutomatableParameterComponent> m_rateParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_depthParamComp;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOComponent)
};

//==============================================================================

class EnvelopeComponent : public juce::Component
{
public:
    EnvelopeComponent(const juce::String& name,
                      te::AutomatableParameter::Ptr attackParam,
                      te::AutomatableParameter::Ptr decayParam,
                      te::AutomatableParameter::Ptr sustainParam,
                      te::AutomatableParameter::Ptr releaseParam)
        : m_name(name)
    {
        m_attackParamComp = std::make_unique<AutomatableParameterComponent>(attackParam, "A");
        m_decayParamComp = std::make_unique<AutomatableParameterComponent>(decayParam, "D");
        m_sustainParamComp = std::make_unique<AutomatableParameterComponent>(sustainParam, "S");
        m_releaseParamComp = std::make_unique<AutomatableParameterComponent>(releaseParam, "R");
        
        addAndMakeVisible(*m_attackParamComp);
        addAndMakeVisible(*m_decayParamComp);
        addAndMakeVisible(*m_sustainParamComp);
        addAndMakeVisible(*m_releaseParamComp);
    }
    
    ~EnvelopeComponent() override = default;
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(juce::Colour(0xff333333));
        GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(m_name, area.removeFromTop(20), juce::Justification::centred);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        area.removeFromTop(20); // Space for title
        
        auto paramWidth = area.getWidth() / 4;
        m_attackParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_decayParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_sustainParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_releaseParamComp->setBounds(area.reduced(2));
    }
    
private:
    juce::String m_name;
    
    std::unique_ptr<AutomatableParameterComponent> m_attackParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_decayParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_sustainParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_releaseParamComp;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
};

//==============================================================================
//==============================================================================

class EffectComponent : public juce::Component
{
public:
    EffectComponent(const juce::String& name, 
                    juce::CachedValue<bool>& enabledValue, 
                    te::AutomatableParameter::Ptr param1,
                    te::AutomatableParameter::Ptr param2,
                    te::AutomatableParameter::Ptr param3,
                    te::AutomatableParameter::Ptr param4 = nullptr)
        : m_name(name), m_enabledValue(enabledValue)
    {
        m_enabledButton.reset(new juce::ToggleButton(""));
        m_enabledButton->setToggleState(m_enabledValue.get(), juce::dontSendNotification);
        m_enabledButton->onClick = [this] {
            m_enabledValue = m_enabledButton->getToggleState();
        };
        addAndMakeVisible(*m_enabledButton);
        
        m_param1Component = std::make_unique<AutomatableParameterComponent>(param1, param1->getParameterName());
        m_param2Component = std::make_unique<AutomatableParameterComponent>(param2, param2->getParameterName());
        m_param3Component = std::make_unique<AutomatableParameterComponent>(param3, param3->getParameterName());
        
        addAndMakeVisible(*m_param1Component);
        addAndMakeVisible(*m_param2Component);
        addAndMakeVisible(*m_param3Component);
        
        if (param4 != nullptr)
        {
            m_param4Component = std::make_unique<AutomatableParameterComponent>(param4, param4->getParameterName());
            addAndMakeVisible(*m_param4Component);
        }
    }
    
    ~EffectComponent() override = default;
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(juce::Colour(0xff333333));
        GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, true, true, true);
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(m_name, area.removeFromTop(20).withTrimmedRight(30), juce::Justification::centred);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        auto titleArea = area.removeFromTop(20);
        
        // Toggle button in title area
        m_enabledButton->setBounds(titleArea.removeFromRight(30).reduced(5, 0));
        
        // Parameters
        int numParams = m_param4Component != nullptr ? 4 : 3;
        auto paramWidth = area.getWidth() / numParams;
        
        m_param1Component->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_param2Component->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        m_param3Component->setBounds(area.removeFromLeft(paramWidth).reduced(2));
        
        if (m_param4Component != nullptr)
            m_param4Component->setBounds(area.reduced(2));
    }
    
private:
    juce::String m_name;
    juce::CachedValue<bool>& m_enabledValue;
    
    std::unique_ptr<juce::ToggleButton> m_enabledButton;
    
    std::unique_ptr<AutomatableParameterComponent> m_param1Component;
    std::unique_ptr<AutomatableParameterComponent> m_param2Component;
    std::unique_ptr<AutomatableParameterComponent> m_param3Component;
    std::unique_ptr<AutomatableParameterComponent> m_param4Component;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectComponent)
};

//==============================================================================

//==============================================================================
class SimpleFourOscPluginComponent : public PluginViewComponent,
                                     private juce::ValueTree::Listener
{
public:
    SimpleFourOscPluginComponent(EditViewState& evs, te::Plugin::Ptr p)
        : PluginViewComponent(evs, p)
    {
        m_fourOscPlugin = dynamic_cast<te::FourOscPlugin*>(p.get());
        jassert(m_fourOscPlugin != nullptr);
        
        // Create TabComponent
        m_tabComponent.reset(new juce::TabbedComponent(juce::TabbedButtonBar::TabsAtLeft));
    
        addAndMakeVisible(*m_tabComponent);
        
        // --- MAIN TAB ---
        auto mainPanel = std::make_unique<juce::Component>();
        
        // Voice mode combobox
        m_voiceModeCombo.reset(new juce::ComboBox("Voice Mode"));
        m_voiceModeCombo->addItem("Mono", 1);
        m_voiceModeCombo->addItem("Legato", 2);
        m_voiceModeCombo->addItem("Poly", 3);
        m_voiceModeCombo->setSelectedId(m_fourOscPlugin->voiceModeValue.get() + 1, juce::dontSendNotification);
        m_voiceModeCombo->onChange = [this] {
            m_fourOscPlugin->voiceModeValue = m_voiceModeCombo->getSelectedId() - 1;
        };
        mainPanel->addAndMakeVisible(*m_voiceModeCombo);
        // Create oscillator selection buttons
        for (int i = 0; i < 4; ++i)
        {
            juce::Colour colour;

            switch (i)
            {
                case 0: colour = juce::Colour(0xffddcc00); break;
                case 1: colour = juce::Colours::green; break;
                case 2: colour = juce::Colours::blue; break;
                case 3: colour = juce::Colours::red; break;
            }
            auto oscButton = std::make_unique<juce::TextButton>("OSC " + juce::String(i + 1));
            oscButton->setColour(juce::TextButton::ColourIds::buttonColourId, colour.withBrightness(0.8f));
            oscButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, colour.withBrightness(1.1f));
            oscButton->setClickingTogglesState(true);
            oscButton->setRadioGroupId(1); // Make them act as radio buttons
            
            // Set the first button as selected initially
            if (i == 0)
                oscButton->setToggleState(true, juce::dontSendNotification);
            
            // Add a lambda to handle button click
            auto oscIndex = i; // Capture for lambda
            oscButton->onClick = [this, oscIndex] {
                // Update current oscillator index
                m_currentOscIndex = oscIndex;
                
                GUIHelpers::log("BUTTON: ", oscIndex);
                // Update visibility of oscillator components
                updateOscComponentVisibility();
                this->repaint();
            };
            
            m_oscSelectButtons.add(oscButton.release());
            mainPanel->addAndMakeVisible(*m_oscSelectButtons.getLast());
        }
        // Master level
        m_masterLevelSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->masterLevel.get()));
        m_masterLevelSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_masterLevelLabel.reset(new juce::Label("Master", "Master"));
        m_masterLevelLabel->setJustificationType(juce::Justification::centred);
        mainPanel->addAndMakeVisible(*m_masterLevelSlider);
        mainPanel->addAndMakeVisible(*m_masterLevelLabel);
        
        // Oscillator Wave Shapes
        for (int i = 0; i < m_fourOscPlugin->oscParams.size(); ++i)
        {
            juce::Colour colour;

            switch (i)
            {
                case 0: colour = juce::Colours::yellow; break;
                case 1: colour = juce::Colours::green; break;
                case 2: colour = juce::Colours::blue; break;
                case 3: colour = juce::Colours::red; break;
            }
            auto oscComponent = std::make_unique<OscComponent>(*m_fourOscPlugin->oscParams[i], colour);
            
            m_oscComponents.add(std::move(oscComponent));
            mainPanel->addAndMakeVisible(m_oscComponents.getLast());
        }
        
        // ADSR Sliders for Amp Envelope
        setupADSRSliders(mainPanel.get(), 
                         m_ampAttackSlider, m_ampDecaySlider, m_ampSustainSlider, m_ampReleaseSlider,
                         m_fourOscPlugin->ampAttack, m_fourOscPlugin->ampDecay, 
                         m_fourOscPlugin->ampSustain, m_fourOscPlugin->ampRelease);
        
        m_ampEnvLabel.reset(new juce::Label("AmpEnv", "AMP ENV"));
        m_ampEnvLabel->setJustificationType(juce::Justification::centred);
        mainPanel->addAndMakeVisible(*m_ampEnvLabel);




        // Filter Sliders
        m_filterFreqSlider.reset(new AutomatableParameterComponent(*m_fourOscPlugin->filterFreq.get(), "Freq"));
        m_filterResSlider.reset(new AutomatableParameterComponent(*m_fourOscPlugin->filterResonance.get(), "Res"));
        
        m_filterTypeCombo.reset(new juce::ComboBox("FilterType"));
        m_filterTypeCombo->addItem("Off", 1);
        m_filterTypeCombo->addItem("Low Pass", 2);
        m_filterTypeCombo->addItem("High Pass", 3);
        m_filterTypeCombo->setSelectedId(m_fourOscPlugin->filterTypeValue.get() + 1, juce::dontSendNotification);
        m_filterTypeCombo->onChange = [this] {
            m_fourOscPlugin->filterTypeValue = m_filterTypeCombo->getSelectedId() - 1;
        };
        
        m_filterLabel.reset(new juce::Label("Filter", "FILTER"));
        
        m_filterLabel->setJustificationType(juce::Justification::centred);
        
        mainPanel->addAndMakeVisible(*m_filterFreqSlider);
        mainPanel->addAndMakeVisible(*m_filterResSlider);
        mainPanel->addAndMakeVisible(*m_filterTypeCombo);
        mainPanel->addAndMakeVisible(*m_filterLabel);
        
        m_tabComponent->addTab("Main", juce::Colour(0x00ffffff), mainPanel.release(), true);
    
        m_tabComponent->setColour(juce::TabbedComponent::ColourIds::backgroundColourId, juce::Colour(0x00ffffff));
        m_tabComponent->setOutline(0);
        // --- EFFECTS TAB ---
        auto effectsPanel = std::make_unique<juce::Component>();
        
        // Distortion
        m_distortionSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->distortion.get()));
        m_distortionSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        m_distortionToggle.reset(new juce::ToggleButton("Distortion"));
        m_distortionToggle->setToggleState(m_fourOscPlugin->distortionOnValue.get(), juce::dontSendNotification);
        m_distortionToggle->onClick = [this] {
            m_fourOscPlugin->distortionOnValue = m_distortionToggle->getToggleState();
        };
        
        effectsPanel->addAndMakeVisible(*m_distortionSlider);
        effectsPanel->addAndMakeVisible(*m_distortionToggle);
        
        // Reverb
        m_reverbSizeSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->reverbSize.get()));
        m_reverbMixSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->reverbMix.get()));
        
        m_reverbSizeSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_reverbMixSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        m_reverbToggle.reset(new juce::ToggleButton("Reverb"));
        m_reverbToggle->setToggleState(m_fourOscPlugin->reverbOnValue.get(), juce::dontSendNotification);
        m_reverbToggle->onClick = [this] {
            m_fourOscPlugin->reverbOnValue = m_reverbToggle->getToggleState();
        };
        
        m_reverbSizeLabel.reset(new juce::Label("Size", "Size"));
        m_reverbMixLabel.reset(new juce::Label("Mix", "Mix"));
        
        m_reverbSizeLabel->setJustificationType(juce::Justification::centred);
        m_reverbMixLabel->setJustificationType(juce::Justification::centred);
        
        effectsPanel->addAndMakeVisible(*m_reverbSizeSlider);
        effectsPanel->addAndMakeVisible(*m_reverbMixSlider);
        effectsPanel->addAndMakeVisible(*m_reverbToggle);
        effectsPanel->addAndMakeVisible(*m_reverbSizeLabel);
        effectsPanel->addAndMakeVisible(*m_reverbMixLabel);
        
        // Delay
        m_delayFeedbackSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->delayFeedback.get()));
        m_delayMixSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->delayMix.get()));
        
        m_delayFeedbackSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_delayMixSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        m_delayToggle.reset(new juce::ToggleButton("Delay"));
        m_delayToggle->setToggleState(m_fourOscPlugin->delayOnValue.get(), juce::dontSendNotification);
        m_delayToggle->onClick = [this] {
            m_fourOscPlugin->delayOnValue = m_delayToggle->getToggleState();
        };
        
        m_delayFeedbackLabel.reset(new juce::Label("Feedback", "Feedback"));
        m_delayMixLabel.reset(new juce::Label("Mix", "Mix"));
        
        m_delayFeedbackLabel->setJustificationType(juce::Justification::centred);
        m_delayMixLabel->setJustificationType(juce::Justification::centred);
        
        effectsPanel->addAndMakeVisible(*m_delayFeedbackSlider);
        effectsPanel->addAndMakeVisible(*m_delayMixSlider);
        effectsPanel->addAndMakeVisible(*m_delayToggle);
        effectsPanel->addAndMakeVisible(*m_delayFeedbackLabel);
        effectsPanel->addAndMakeVisible(*m_delayMixLabel);
        
        // Chorus
        m_chorusDepthSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->chorusDepth.get()));
        m_chorusMixSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->chorusMix.get()));
        
        m_chorusDepthSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_chorusMixSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        m_chorusToggle.reset(new juce::ToggleButton("Chorus"));
        m_chorusToggle->setToggleState(m_fourOscPlugin->chorusOnValue.get(), juce::dontSendNotification);
        m_chorusToggle->onClick = [this] {
            m_fourOscPlugin->chorusOnValue = m_chorusToggle->getToggleState();
        };
        
        m_chorusDepthLabel.reset(new juce::Label("Depth", "Depth"));
        m_chorusMixLabel.reset(new juce::Label("Mix", "Mix"));
        
        m_chorusDepthLabel->setJustificationType(juce::Justification::centred);
        m_chorusMixLabel->setJustificationType(juce::Justification::centred);
        
        effectsPanel->addAndMakeVisible(*m_chorusDepthSlider);
        effectsPanel->addAndMakeVisible(*m_chorusMixSlider);
        effectsPanel->addAndMakeVisible(*m_chorusToggle);
        effectsPanel->addAndMakeVisible(*m_chorusDepthLabel);
        effectsPanel->addAndMakeVisible(*m_chorusMixLabel);
        
        m_tabComponent->addTab("Effects", juce::Colour(0xff333333), effectsPanel.release(), true);
        
        // Listen to plugin state changes
        p->state.addListener(this);
        updateOscComponentVisibility();
    }
    
    ~SimpleFourOscPluginComponent() override
    {
        // Stop listening to plugin state changes
        m_plugin->state.removeListener(this);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff333333));

        
        g.setColour(juce::Colour(0xff4b4b4b));

        juce::Colour colour;

        switch (getActiveOscComponent())
        {
            case 0: colour = juce::Colour(0xffddcc00); break;
            case 1: colour = juce::Colours::green; break;
            case 2: colour = juce::Colours::blue; break;
            case 3: colour = juce::Colours::red; break;
        }
        g.setColour(colour);
        GUIHelpers::drawRoundedRectWithSide(g, m_rectsToPaint[0].expanded(3,3).toFloat(), 10, true, true, true, true);
    
        g.setColour(juce::Colour(0xff4b4b4b));
        for (auto rect : m_rectsToPaint)
        {
            GUIHelpers::drawRoundedRectWithSide(g, rect.toFloat(), 10, true, true, true, true);
        }
    }
    
void resized() override
    {
        m_rectsToPaint.clear();
        auto area = getLocalBounds();
        m_tabComponent->setBounds(area);

        if (auto* mainPanel = m_tabComponent->getTabContentComponent(0))
        {
            auto mainArea = mainPanel->getLocalBounds();
            auto tabWidth = m_tabComponent->getTabBarDepth();
        
            // Top row with voice mode and master level
            auto topRow = mainArea.removeFromTop(30);
            // OSC Selection Buttons - NEW CODE
            auto oscButtonsArea = topRow.removeFromLeft(topRow.getWidth() / 3).reduced(20, 2); // Leave space for master level
            int buttonWidth = oscButtonsArea.getWidth() / 4;
            
            for (int i = 0; i < m_oscSelectButtons.size(); ++i)
            {
                m_oscSelectButtons[i]->setBounds(oscButtonsArea.removeFromLeft(buttonWidth).reduced(2, 2));
            }
            
            m_voiceModeCombo->setBounds(topRow.removeFromLeft(topRow.getWidth()/4).reduced(2, 2));
            
            
            auto masterArea = topRow;
            m_masterLevelLabel->setBounds(masterArea);
            m_masterLevelSlider->setBounds(masterArea.reduced(2,2));
            
            // Rest of your layout code remains the same
            

            // Oscillators - MODIFIED CODE
            auto oscArea = mainArea.removeFromLeft(mainArea.getWidth()/3);

            auto oscRect = oscArea;
            oscRect.translate(tabWidth, 0);
            oscRect.reduce(5, 5);
            m_rectsToPaint.add(oscRect);
            
            // Layout all oscillator components in the same area
            // Only the selected one will be visible
            for (int i = 0; i < m_oscComponents.size(); ++i)
            {
                m_oscComponents[i]->setBounds(oscArea.reduced(5));
            }

            
            // Bottom area with amp env and filter
            auto envFilterArea = mainArea.removeFromLeft(mainArea.getWidth()/2);
            
            // Amp ENV area
            auto ampEnvArea = envFilterArea.removeFromTop(envFilterArea.getHeight()/2).reduced(5);
            auto envRect = ampEnvArea;
            envRect.translate(tabWidth, 0);
            m_rectsToPaint.add(envRect);
            m_ampEnvLabel->setBounds(ampEnvArea.removeFromTop(20));

            auto sliderWidth = ampEnvArea.getWidth() / 4;

            auto ampAttackArea = ampEnvArea.removeFromLeft(sliderWidth).reduced(5);
            m_ampAttackSlider->setBounds(ampAttackArea);

            auto ampDecayArea = ampEnvArea.removeFromLeft(sliderWidth).reduced(5);
            m_ampDecaySlider->setBounds(ampDecayArea);

            auto ampSustainArea = ampEnvArea.removeFromLeft(sliderWidth).reduced(5);
            m_ampSustainSlider->setBounds(ampSustainArea);

            auto ampReleaseArea = ampEnvArea.reduced(5);
            m_ampReleaseSlider->setBounds(ampReleaseArea);
                        
            // Filter area
            auto filterArea = envFilterArea.reduced(5);

            auto filterRect = filterArea;
            filterRect.translate(tabWidth, 0);
            m_rectsToPaint.add(filterRect);

            // Oberer Bereich: Label + Combo nebeneinander
            auto filterTopRow = filterArea.removeFromTop(30);
            auto filterLabelWidth = 80;
            m_filterLabel->setBounds(filterTopRow.removeFromLeft(filterLabelWidth));
            m_filterTypeCombo->setBounds(filterTopRow);

            // Frequenz und Resonanz
            auto filterParamWidth = filterArea.getWidth() / 2;
            auto filterFreqArea = filterArea.removeFromLeft(filterParamWidth).reduced(5);
            auto filterResArea = filterArea.reduced(5);

            m_filterFreqSlider->setBounds(filterFreqArea);
            m_filterResSlider->setBounds(filterResArea);
        }
    
        if (auto* effectsPanel = m_tabComponent->getTabContentComponent(1))
        {
        
            auto effectsArea = effectsPanel->getLocalBounds().reduced(5);
            auto effectHeight = effectsArea.getHeight() / 4;
            const int toggleWidth = 100; // Breite für die Toggle-Buttons
            const int labelWidth = 80;   // Breite für die Labels neben den Sliders
            const int spacing = 5;       // Allgemeiner Abstand

            // --- Distortion ---
            auto distortionArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
            m_distortionToggle->setBounds(distortionArea.removeFromLeft(toggleWidth));
            // Kein Label für Distortion im Original, Slider nimmt den Rest des Platzes ein
            m_distortionSlider->setBounds(distortionArea.reduced(spacing, 0)); // Etwas Abstand zum Toggle

            // --- Reverb ---
            auto reverbArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
            m_reverbToggle->setBounds(reverbArea.removeFromLeft(toggleWidth));

            // Berechne Breite für jeden Parameter-Bereich (Label + Slider)
            // Der verbleibende Platz wird durch 2 geteilt
            auto reverbParamWidth = reverbArea.getWidth() / 2;

            // Reverb Size
            auto reverbSizeArea = reverbArea.removeFromLeft(reverbParamWidth).reduced(spacing, 0); // Abstand zum Toggle/Rand
            m_reverbSizeLabel->setBounds(reverbSizeArea.removeFromLeft(labelWidth));
            m_reverbSizeSlider->setBounds(reverbSizeArea.reduced(spacing, 0)); // Abstand zum Label

            // Reverb Mix
            auto reverbMixArea = reverbArea.reduced(spacing, 0); // Abstand zum Size-Bereich
            m_reverbMixLabel->setBounds(reverbMixArea.removeFromLeft(labelWidth));
            m_reverbMixSlider->setBounds(reverbMixArea.reduced(spacing, 0)); // Abstand zum Label

            // --- Delay ---
            auto delayArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
            m_delayToggle->setBounds(delayArea.removeFromLeft(toggleWidth));

            auto delayParamWidth = delayArea.getWidth() / 2;

            // Delay Feedback
            auto delayFeedbackArea = delayArea.removeFromLeft(delayParamWidth).reduced(spacing, 0);
            m_delayFeedbackLabel->setBounds(delayFeedbackArea.removeFromLeft(labelWidth));
            m_delayFeedbackSlider->setBounds(delayFeedbackArea.reduced(spacing, 0));

            // Delay Mix
            auto delayMixArea = delayArea.reduced(spacing, 0);
            m_delayMixLabel->setBounds(delayMixArea.removeFromLeft(labelWidth));
            m_delayMixSlider->setBounds(delayMixArea.reduced(spacing, 0));

            // --- Chorus ---
            // Chorus nimmt den verbleibenden Platz ein
            auto chorusArea = effectsArea.reduced(spacing);
            m_chorusToggle->setBounds(chorusArea.removeFromLeft(toggleWidth));

            auto chorusParamWidth = chorusArea.getWidth() / 2;

            // Chorus Depth
            auto chorusDepthArea = chorusArea.removeFromLeft(chorusParamWidth).reduced(spacing, 0);
            m_chorusDepthLabel->setBounds(chorusDepthArea.removeFromLeft(labelWidth));
            m_chorusDepthSlider->setBounds(chorusDepthArea.reduced(spacing, 0));

            // Chorus Mix
            auto chorusMixArea = chorusArea.reduced(spacing, 0);
            m_chorusMixLabel->setBounds(chorusMixArea.removeFromLeft(labelWidth));
            m_chorusMixSlider->setBounds(chorusMixArea.reduced(spacing, 0));
        }
    }
    
    int getNeededWidth() override { return 6; }
    
private:

    void updateOscComponentVisibility()
    {
        // Hide all oscillator components except the selected one
        for (int i = 0; i < m_oscComponents.size(); ++i)
        {
            m_oscComponents[i]->setVisible(i == m_currentOscIndex);
        }
    }

    int getActiveOscComponent()
    {
        for (int i = 0; i < m_oscComponents.size(); ++i)
            if (m_oscComponents[i]->isVisible())
                return i;

        return -1;
    }
    te::FourOscPlugin* m_fourOscPlugin = nullptr;
    
    std::unique_ptr<juce::TabbedComponent> m_tabComponent;
    
    // Main Tab Components
    std::unique_ptr<juce::ComboBox> m_voiceModeCombo;

    juce::OwnedArray<juce::TextButton> m_oscSelectButtons;
    int m_currentOscIndex = 0;

    std::unique_ptr<AutomatableSliderComponent> m_masterLevelSlider;
    std::unique_ptr<juce::Label> m_masterLevelLabel;
    
    // Oscillators
    juce::OwnedArray<juce::ComboBox> m_oscWaveShapeCombos;
    juce::OwnedArray<AutomatableSliderComponent> m_oscLevelSliders;
    juce::OwnedArray<juce::Label> m_oscLabels;
    
    juce::OwnedArray<OscComponent> m_oscComponents;

    // Amp Envelope
    std::unique_ptr<AutomatableParameterComponent> m_ampAttackSlider;
    std::unique_ptr<AutomatableParameterComponent> m_ampDecaySlider;
    std::unique_ptr<AutomatableParameterComponent> m_ampSustainSlider;
    std::unique_ptr<AutomatableParameterComponent> m_ampReleaseSlider;
    std::unique_ptr<juce::Label> m_ampEnvLabel;
    
    // Filter
    std::unique_ptr<juce::ComboBox> m_filterTypeCombo;
    std::unique_ptr<AutomatableParameterComponent> m_filterFreqSlider;
    std::unique_ptr<AutomatableParameterComponent> m_filterResSlider;
    std::unique_ptr<juce::Label> m_filterLabel;
    
    // Effects Tab Components
    // Distortion
    std::unique_ptr<juce::ToggleButton> m_distortionToggle;
    std::unique_ptr<AutomatableSliderComponent> m_distortionSlider;
    
    // Reverb
    std::unique_ptr<juce::ToggleButton> m_reverbToggle;
    std::unique_ptr<AutomatableSliderComponent> m_reverbSizeSlider;
    std::unique_ptr<AutomatableSliderComponent> m_reverbMixSlider;
    std::unique_ptr<juce::Label> m_reverbSizeLabel;
    std::unique_ptr<juce::Label> m_reverbMixLabel;
    
    // Delay
    std::unique_ptr<juce::ToggleButton> m_delayToggle;
    std::unique_ptr<AutomatableSliderComponent> m_delayFeedbackSlider;
    std::unique_ptr<AutomatableSliderComponent> m_delayMixSlider;
    std::unique_ptr<juce::Label> m_delayFeedbackLabel;
    std::unique_ptr<juce::Label> m_delayMixLabel;
    
    // Chorus
    std::unique_ptr<juce::ToggleButton> m_chorusToggle;
    std::unique_ptr<AutomatableSliderComponent> m_chorusDepthSlider;
    std::unique_ptr<AutomatableSliderComponent> m_chorusMixSlider;
    std::unique_ptr<juce::Label> m_chorusDepthLabel;
    std::unique_ptr<juce::Label> m_chorusMixLabel;
    
    void setupADSRSliders(juce::Component* parent,
                         std::unique_ptr<AutomatableParameterComponent>& attack,
                         std::unique_ptr<AutomatableParameterComponent>& decay,
                         std::unique_ptr<AutomatableParameterComponent>& sustain,
                         std::unique_ptr<AutomatableParameterComponent>& release,
                         te::AutomatableParameter::Ptr attackParam,
                         te::AutomatableParameter::Ptr decayParam,
                         te::AutomatableParameter::Ptr sustainParam,
                         te::AutomatableParameter::Ptr releaseParam)
    {
        attack.reset(new AutomatableParameterComponent(*attackParam.get(), "Attack"));
        decay.reset(new AutomatableParameterComponent(*decayParam.get(), "Decay"));
        sustain.reset(new AutomatableParameterComponent(*sustainParam.get(), "Sustain"));
        release.reset(new AutomatableParameterComponent(*releaseParam.get(), "Release"));
        
        parent->addAndMakeVisible(*attack);
        parent->addAndMakeVisible(*decay);
        parent->addAndMakeVisible(*sustain);
        parent->addAndMakeVisible(*release);
    }
    
    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        // Update UI when plugin state changes
        if (m_voiceModeCombo != nullptr)
            m_voiceModeCombo->setSelectedId(m_fourOscPlugin->voiceModeValue.get() + 1, juce::dontSendNotification);
            
        // Update effect toggles
        if (m_distortionToggle != nullptr)
            m_distortionToggle->setToggleState(m_fourOscPlugin->distortionOnValue.get(), juce::dontSendNotification);
            
        if (m_reverbToggle != nullptr)
            m_reverbToggle->setToggleState(m_fourOscPlugin->reverbOnValue.get(), juce::dontSendNotification);
            
        if (m_delayToggle != nullptr)
            m_delayToggle->setToggleState(m_fourOscPlugin->delayOnValue.get(), juce::dontSendNotification);
            
        if (m_chorusToggle != nullptr)
            m_chorusToggle->setToggleState(m_fourOscPlugin->chorusOnValue.get(), juce::dontSendNotification);
            
        // Update oscillator wave shapes
        for (int i = 0; i < m_oscWaveShapeCombos.size(); ++i)
        {
            if (i < m_fourOscPlugin->oscParams.size())
                m_oscWaveShapeCombos[i]->setSelectedId(m_fourOscPlugin->oscParams[i]->waveShapeValue.get() + 1, juce::dontSendNotification);
        }
        
        // Update filter type
        if (m_filterTypeCombo != nullptr)
            m_filterTypeCombo->setSelectedId(m_fourOscPlugin->filterTypeValue.get() + 1, juce::dontSendNotification);
    }
    
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

private:
    juce::Array<juce::Rectangle<int>>   m_rectsToPaint;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleFourOscPluginComponent)
};


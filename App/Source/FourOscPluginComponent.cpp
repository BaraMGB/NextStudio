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

#include "FourOscPluginComponent.h"

OscComponent::OscComponent(te::FourOscPlugin::OscParams& params, juce::Colour colorToUse)
    : m_params(params)
    , m_colour(colorToUse)
{
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
            m_params.waveShapeValue = m_waveShapeCombo->getSelectedId() - 1;
    };
    addAndMakeVisible(*m_waveShapeCombo);

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

void OscComponent::updateUI()
{
    if (m_waveShapeCombo != nullptr)
        m_waveShapeCombo->setSelectedId(m_params.waveShapeValue.get() + 1, juce::dontSendNotification);
}

void OscComponent::resized() 
{
    auto area = getLocalBounds().reduced(5);

    m_waveShapeCombo->setBounds(area.removeFromTop(24).reduced(5, 0));

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


EnvelopeComponent::EnvelopeComponent(ApplicationViewState& appState, const juce::String& name, te::Plugin& plugin,
                      te::AutomatableParameter::Ptr attackParam,
                      te::AutomatableParameter::Ptr decayParam,
                      te::AutomatableParameter::Ptr sustainParam,
                      te::AutomatableParameter::Ptr releaseParam)
    : m_appstate(appState)
    , m_name(name)
    , m_plugin(plugin)
{
    m_attackParamComp = std::make_unique<AutomatableParameterComponent>(attackParam, "A");
    m_decayParamComp = std::make_unique<AutomatableParameterComponent>(decayParam, "D");
    m_sustainParamComp = std::make_unique<AutomatableParameterComponent>(sustainParam, "S");
    m_releaseParamComp = std::make_unique<AutomatableParameterComponent>(releaseParam, "R");


    m_name.setText(name ,juce::NotificationType::dontSendNotification);
    m_name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_name);
    m_name.setInterceptsMouseClicks (false, true);
    addAndMakeVisible(*m_attackParamComp);
    addAndMakeVisible(*m_decayParamComp);
    addAndMakeVisible(*m_sustainParamComp);
    addAndMakeVisible(*m_releaseParamComp);
}

void EnvelopeComponent::paint(juce::Graphics& g) 
{
    auto area = getLocalBounds();
    auto cornerSize = 10.0f;
    g.setColour(m_appstate.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto trackColour = m_plugin.getOwnerTrack()->getColour();

    auto labelingCol = trackColour.getBrightness() > 0.8f
        ? juce::Colour(0xff000000)
        : juce::Colour(0xffffffff);

    m_name.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    g.setColour(m_appstate.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().toFloat(), cornerSize, true, false, true, false);
}

void EnvelopeComponent::resized() 
{
    auto area = getLocalBounds();

    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX()
                                       , area.getHeight() - headerWidth
                                       , area.getHeight()
                                       , headerWidth);
    m_name.setBounds(header);

    m_name.setFont(juce::FontOptions(10));

    m_name.setTransform(juce::AffineTransform::rotation ( - (juce::MathConstants<float>::halfPi)
                                                         , header.getX() + 10.0
                                                         , header.getY() + 10.0 ));
    area.removeFromLeft(headerWidth);
    auto paramWidth = area.getWidth() / 4;
    m_attackParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_decayParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_sustainParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_releaseParamComp->setBounds(area.reduced(2));
}


FilterComponent::FilterComponent(te::FourOscPlugin& plugin, ApplicationViewState& appstate)
    : m_plugin(plugin)
    , m_appstate(appstate)
{

    m_name.setText("FILTER" ,juce::NotificationType::dontSendNotification);
    m_name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_name);
    m_name.setInterceptsMouseClicks (false, true);
    m_typeCombo.reset(new juce::ComboBox("Type"));
    m_typeCombo->addItem("None", 1);
    m_typeCombo->addItem("Low Pass", 2);
    m_typeCombo->addItem("High Pass", 3);
    // Add 1 to map plugin value (0, 1, 2) to ComboBox ID (1, 2, 3)
    m_typeCombo->setSelectedId(m_plugin.filterTypeValue.get() + 1, juce::dontSendNotification);
    m_typeCombo->onChange = [this] {
        // Subtract 1 to map ComboBox ID (1, 2, 3) back to plugin value (0, 1, 2)
        m_plugin.filterTypeValue = m_typeCombo->getSelectedId() - 1;
    };
    addAndMakeVisible(*m_typeCombo);

    m_slopeCombo.reset(new juce::ComboBox("Slope"));
    m_slopeCombo->addItem("12 dB", 1);
    m_slopeCombo->addItem("24 dB", 2);
    m_slopeCombo->setSelectedId(1, juce::dontSendNotification);
    m_slopeCombo->onChange = [this] {
        // Subtract 1 to map ComboBox ID (1, 2) back to plugin value (0, 1)
        m_plugin.filterSlopeValue = m_slopeCombo->getSelectedId() - 1;
    };
    addAndMakeVisible(*m_slopeCombo);

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
void FilterComponent::paint(juce::Graphics& g) 
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;
    g.setColour(m_appstate.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto trackColour = m_plugin.getOwnerTrack()->getColour();

    auto labelingCol = trackColour.getBrightness() > 0.8f
        ? juce::Colour(0xff000000)
        : juce::Colour(0xffffffff);

    m_name.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    g.setColour(m_appstate.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, false, true, false);
}

void FilterComponent::resized() 
{
    auto area = getLocalBounds().reduced(5);

    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX()
                                       , area.getHeight() - headerWidth
                                       , area.getHeight()
                                       , headerWidth);
    m_name.setBounds(header);

    m_name.setFont(juce::FontOptions(10));

    m_name.setTransform(juce::AffineTransform::rotation ( - (juce::MathConstants<float>::halfPi)
                                                         , header.getX() + 10.0
                                                         , header.getY() + 10.0 ));
    area.removeFromLeft(headerWidth);
    auto topRow = area.removeFromTop(24);

    m_typeCombo->setBounds(topRow.removeFromLeft(topRow.getWidth() / 2).reduced(5, 0));
    m_slopeCombo->setBounds(topRow.reduced(5, 0));

    auto freqResRect = area.removeFromTop(area.getHeight()/2);
    auto paramWidth = freqResRect.getWidth()/3;

    m_freqParamComp->setBounds(freqResRect.removeFromLeft(paramWidth).reduced(2));
    m_resParamComp->setBounds(freqResRect.removeFromLeft(paramWidth).reduced(2));
    m_amountParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_keyParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_velocityParamComp->setBounds(area.removeFromLeft(paramWidth).reduced(2));
}

void FilterComponent::updateUI()
{
    if (m_typeCombo != nullptr)
        m_typeCombo->setSelectedId(m_plugin.filterTypeValue.get() + 1, juce::dontSendNotification);

    if (m_slopeCombo != nullptr)
        m_slopeCombo->setSelectedId(m_plugin.filterSlopeValue.get() + 1, juce::dontSendNotification);
}



FourOscPluginComponent::FourOscPluginComponent(EditViewState& evs, te::Plugin::Ptr p)
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
        auto oscIndex = i;
        oscButton->onClick = [this, oscIndex] {
            m_currentOscIndex = oscIndex;
            updateOscComponentVisibility();
            this->repaint();
        };

        m_oscSelectButtons.add(oscButton.release());
        mainPanel->addAndMakeVisible(*m_oscSelectButtons.getLast());
    }
    m_masterLevelSlider.reset(new AutomatableSliderComponent(*m_fourOscPlugin->masterLevel.get()));
    m_masterLevelSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_masterLevelLabel.reset(new juce::Label("Master", "Master"));
    m_masterLevelLabel->setJustificationType(juce::Justification::right);
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
    m_ampEnvComponent = std::make_unique<EnvelopeComponent>(m_editViewState.m_applicationState, "AMP ENV",*m_fourOscPlugin, m_fourOscPlugin->ampAttack, m_fourOscPlugin->ampDecay, m_fourOscPlugin->ampSustain, m_fourOscPlugin->ampRelease );
    m_filterEnvComp = std::make_unique<EnvelopeComponent>(m_editViewState.m_applicationState, "FILTER ENV", *m_plugin,
                                                          m_fourOscPlugin->filterAttack,  // Assumed parameter name
                                                          m_fourOscPlugin->filterDecay,   // Assumed parameter name
                                                          m_fourOscPlugin->filterSustain, // Assumed parameter name
                                                          m_fourOscPlugin->filterRelease);// Assumed parameter name
    mainPanel->addAndMakeVisible(*m_filterEnvComp);
    mainPanel->addAndMakeVisible(m_ampEnvComponent.get());
    // Filter Sliders
    m_filterComponent = std::make_unique<FilterComponent>(*m_fourOscPlugin, m_editViewState.m_applicationState);
    mainPanel->addAndMakeVisible(m_filterComponent.get());

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

FourOscPluginComponent::~FourOscPluginComponent() 
{
    // Stop listening to plugin state changes
    m_plugin->state.removeListener(this);
}

void FourOscPluginComponent::paint(juce::Graphics& g) 
{
    auto background1 = m_editViewState.m_applicationState.getBackgroundColour1();
    auto background2 = m_editViewState.m_applicationState.getBackgroundColour2();
    g.fillAll(background2);


    g.setColour(background1);

    juce::Colour colour;

    switch (getActiveOscComponent())
    {
        case 0: colour = juce::Colour(0xffddcc00); break;
        case 1: colour = juce::Colours::green; break;
        case 2: colour = juce::Colours::blue; break;
        case 3: colour = juce::Colours::red; break;
    }
    g.setColour(colour);
    GUIHelpers::drawRoundedRectWithSide(g, m_rectsToPaint[0].expanded(1,1).toFloat(), 10, true, true, true, true);

    g.setColour(background1);
    for (auto rect : m_rectsToPaint)
    {
        GUIHelpers::drawRoundedRectWithSide(g, rect.toFloat(), 10, true, true, true, true);
    }
}

void FourOscPluginComponent::resized() 
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
        auto oscButtonsArea = topRow.removeFromLeft(topRow.getWidth() / 3).reduced(20, 2); // Leave space for master level
        int buttonWidth = oscButtonsArea.getWidth() / 4;

        for (int i = 0; i < m_oscSelectButtons.size(); ++i)
        {
            m_oscSelectButtons[i]->setBounds(oscButtonsArea.removeFromLeft(buttonWidth).reduced(2, 2));
        }

        m_voiceModeCombo->setBounds(topRow.removeFromLeft(topRow.getWidth()/4).reduced(2, 2));


        auto masterArea = topRow;
        m_masterLevelSlider->setBounds(masterArea.removeFromRight(masterArea.getHeight()));
        m_masterLevelLabel->setBounds(masterArea);

        auto oscArea = mainArea.removeFromLeft(mainArea.getWidth()/3);
        auto oscRect = oscArea;
        oscRect.translate(tabWidth, 0);
        oscRect.reduce(5, 5);
        m_rectsToPaint.add(oscRect);

        for (int i = 0; i < m_oscComponents.size(); ++i)
        {
            m_oscComponents[i]->setBounds(oscArea.reduced(5));
        }

        auto envFilterArea = mainArea.removeFromLeft(mainArea.getWidth()/2);

        auto ampEnvArea = envFilterArea.removeFromTop(envFilterArea.getHeight()/2).reduced(5);
        m_ampEnvComponent->setBounds(ampEnvArea);

        m_filterEnvComp->setBounds(envFilterArea.reduced(5)); // Add a little vertical padding
        m_filterComponent->setBounds(mainArea);
    }

    if (auto* effectsPanel = m_tabComponent->getTabContentComponent(1))
    {
        auto effectsArea = effectsPanel->getLocalBounds().reduced(5);
        auto effectHeight = effectsArea.getHeight() / 4;
        const int toggleWidth = 100;
        const int labelWidth = 80;
        const int spacing = 5;

        auto distortionArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
        m_distortionToggle->setBounds(distortionArea.removeFromLeft(toggleWidth));
        m_distortionSlider->setBounds(distortionArea.reduced(spacing, 0)); // Etwas Abstand zum Toggle

        auto reverbArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
        m_reverbToggle->setBounds(reverbArea.removeFromLeft(toggleWidth));
        auto reverbParamWidth = reverbArea.getWidth() / 2;
        auto reverbSizeArea = reverbArea.removeFromLeft(reverbParamWidth).reduced(spacing, 0); // Abstand zum Toggle/Rand
        m_reverbSizeLabel->setBounds(reverbSizeArea.removeFromLeft(labelWidth));
        m_reverbSizeSlider->setBounds(reverbSizeArea.reduced(spacing, 0)); // Abstand zum Label
        auto reverbMixArea = reverbArea.reduced(spacing, 0); // Abstand zum Size-Bereich
        m_reverbMixLabel->setBounds(reverbMixArea.removeFromLeft(labelWidth));
        m_reverbMixSlider->setBounds(reverbMixArea.reduced(spacing, 0)); // Abstand zum Label

        auto delayArea = effectsArea.removeFromTop(effectHeight).reduced(spacing);
        m_delayToggle->setBounds(delayArea.removeFromLeft(toggleWidth));
        auto delayParamWidth = delayArea.getWidth() / 2;
        auto delayFeedbackArea = delayArea.removeFromLeft(delayParamWidth).reduced(spacing, 0);
        m_delayFeedbackLabel->setBounds(delayFeedbackArea.removeFromLeft(labelWidth));
        m_delayFeedbackSlider->setBounds(delayFeedbackArea.reduced(spacing, 0));
        auto delayMixArea = delayArea.reduced(spacing, 0);
        m_delayMixLabel->setBounds(delayMixArea.removeFromLeft(labelWidth));
        m_delayMixSlider->setBounds(delayMixArea.reduced(spacing, 0));

        auto chorusArea = effectsArea.reduced(spacing);
        m_chorusToggle->setBounds(chorusArea.removeFromLeft(toggleWidth));
        auto chorusParamWidth = chorusArea.getWidth() / 2;
        auto chorusDepthArea = chorusArea.removeFromLeft(chorusParamWidth).reduced(spacing, 0);
        m_chorusDepthLabel->setBounds(chorusDepthArea.removeFromLeft(labelWidth));
        m_chorusDepthSlider->setBounds(chorusDepthArea.reduced(spacing, 0));
        auto chorusMixArea = chorusArea.reduced(spacing, 0);
        m_chorusMixLabel->setBounds(chorusMixArea.removeFromLeft(labelWidth));
        m_chorusMixSlider->setBounds(chorusMixArea.reduced(spacing, 0));
    }
}

void FourOscPluginComponent::updateOscComponentVisibility()
{
    for (int i = 0; i < m_oscComponents.size(); ++i)
        m_oscComponents[i]->setVisible(i == m_currentOscIndex);
}

int FourOscPluginComponent::getActiveOscComponent()
{
    for (int i = 0; i < m_oscComponents.size(); ++i)
        if (m_oscComponents[i]->isVisible())
            return i;

    return -1;
}

void FourOscPluginComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) 
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

    // Update filter UI
    if (m_filterComponent != nullptr)
        m_filterComponent->updateUI();

    // Update oscillator UIs
    for (int i = 0; i < m_oscComponents.size(); ++i)
    {
        if (m_oscComponents[i] != nullptr)
            m_oscComponents[i]->updateUI();
    }
}

// PluginPresetInterface implementation
juce::ValueTree FourOscPluginComponent::getPluginState()
{
    return m_fourOscPlugin->state.createCopy();
}

juce::ValueTree FourOscPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState ("PLUGIN");
    defaultState.setProperty ("type", "4osc", nullptr);
    // TODO: Populate with all default values for a true factory-fresh state
    return defaultState;
}

void FourOscPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_fourOscPlugin->restorePluginStateFromValueTree(state);
}

juce::String FourOscPluginComponent::getPresetSubfolder() const
{
    return "FourOSC";
}

juce::String FourOscPluginComponent::getPluginTypeName() const
{
    return "4osc";
}

ApplicationViewState& FourOscPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}


/*
  ==============================================================================

    SimpleSynthPluginComponent.cpp
    Created: 15 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "SimpleSynthPluginComponent.h"

//==============================================================================
// SimpleSynthOscSection
//==============================================================================

SimpleSynthOscSection::SimpleSynthOscSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState, int oscIndex)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_oscIndex(oscIndex)
    , m_waveComp(oscIndex == 0 ? plugin.waveParam : plugin.osc2WaveParam, "Wave")
    , m_coarseTuneComp(oscIndex == 0 ? plugin.coarseTuneParam : plugin.osc2CoarseParam, "Tune")
    , m_fineTuneComp(oscIndex == 0 ? plugin.fineTuneParam : plugin.osc2FineParam, "Fine")
{
    juce::String title = (oscIndex == 0) ? "OSC 1" : "OSC 2";
    m_nameLabel.setText(title, juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    addAndMakeVisible(m_waveComp);
    addAndMakeVisible(m_coarseTuneComp);
    addAndMakeVisible(m_fineTuneComp);

    if (m_oscIndex == 0)
    {
        m_unisonOrderComp = std::make_unique<AutomatableParameterComponent>(plugin.unisonOrderParam, "Voices");
        m_unisonDetuneComp = std::make_unique<AutomatableParameterComponent>(plugin.unisonDetuneParam, "Detune");
        m_unisonSpreadComp = std::make_unique<AutomatableParameterComponent>(plugin.unisonSpreadParam, "Spread");
        m_retriggerComp = std::make_unique<AutomatableToggleComponent>(plugin.retriggerParam, "Retrig");

        addAndMakeVisible(*m_unisonOrderComp);
        addAndMakeVisible(*m_unisonDetuneComp);
        addAndMakeVisible(*m_unisonSpreadComp);
        addAndMakeVisible(*m_retriggerComp);
    }
    else
    {
        m_levelComp = std::make_unique<AutomatableParameterComponent>(plugin.osc2LevelParam, "Level");
        addAndMakeVisible(*m_levelComp);
    }
}

void SimpleSynthOscSection::updateUI()
{
}

void SimpleSynthOscSection::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;
    
    // Background
    g.setColour(m_appState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    // Header Background
    auto trackColour = m_plugin.getOwnerTrack()->getColour();
    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    // Label Colour
    auto labelingCol = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    // Border
    g.setColour(m_appState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, false, true, false);
}

void SimpleSynthOscSection::resized()
{
    auto area = getLocalBounds().reduced(5);

    // Header (Vertical Label)
    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX(), area.getHeight() - headerWidth, area.getHeight(), headerWidth);
    m_nameLabel.setBounds(header);
    m_nameLabel.setFont(juce::FontOptions(10));
    m_nameLabel.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), 
                                                             header.getX() + 10.0f, 
                                                             header.getY() + 10.0f));
    area.removeFromLeft(headerWidth);

    // Content Layout
    auto topRow = area.removeFromTop(50);
    m_waveComp.setBounds(topRow.reduced(2));

    if (m_oscIndex == 0)
    {
        // OSC 1 Layout (Complex)
        auto row1 = area.removeFromTop(area.getHeight() / 2);
        auto paramWidth = row1.getWidth() / 3;

        m_coarseTuneComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        m_fineTuneComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        if (m_unisonOrderComp) m_unisonOrderComp->setBounds(row1.removeFromLeft(paramWidth).reduced(2));

        auto row2 = area;
        paramWidth = row2.getWidth() / 3;
        if (m_unisonDetuneComp) m_unisonDetuneComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
        if (m_unisonSpreadComp) m_unisonSpreadComp->setBounds(row2.removeFromLeft(paramWidth).reduced(2));
        if (m_retriggerComp) m_retriggerComp->setBounds(row2.reduced(2));
    }
    else
    {
        // OSC 2 Layout (Simple)
        auto row1 = area.removeFromTop(area.getHeight() / 2);
        auto paramWidth = row1.getWidth() / 2;
        
        m_coarseTuneComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));
        m_fineTuneComp.setBounds(row1.reduced(2));
        
        // Level takes full width of bottom row for prominence
        if (m_levelComp) m_levelComp->setBounds(area.reduced(2));
    }
}

//==============================================================================
// SimpleSynthMixSection
//==============================================================================

SimpleSynthMixSection::SimpleSynthMixSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_mixModeComp(plugin.mixModeParam, "Mix Mode")
    , m_crossModComp(plugin.crossModAmountParam, "Cross Mod")
{
    m_nameLabel.setText("MIX", juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    addAndMakeVisible(m_mixModeComp);
    addAndMakeVisible(m_crossModComp);
    
    updateUI();
}

void SimpleSynthMixSection::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;
    g.setColour(m_appState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto trackColour = m_plugin.getOwnerTrack()->getColour();
    auto labelingCol = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    g.setColour(m_appState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, false, true, false);
}

void SimpleSynthMixSection::resized()
{
    auto area = getLocalBounds().reduced(5);
    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX(), area.getHeight() - headerWidth, area.getHeight(), headerWidth);
    m_nameLabel.setBounds(header);
    m_nameLabel.setFont(juce::FontOptions(10));
    m_nameLabel.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), 
                                                             header.getX() + 10.0f, 
                                                             header.getY() + 10.0f));
    area.removeFromLeft(headerWidth);

    auto topHalf = area.removeFromTop(area.getHeight() / 2);
    m_mixModeComp.setBounds(topHalf.reduced(2));
    m_crossModComp.setBounds(area.reduced(2));
}

void SimpleSynthMixSection::updateUI()
{
    int mode = (int)m_plugin.mixModeParam->getCurrentValue();
    bool crossModActive = (mode == SimpleSynthPlugin::MixMode::ringMod || mode == SimpleSynthPlugin::MixMode::fm);
    m_crossModComp.setEnabled(crossModActive);
}

//==============================================================================
// SimpleSynthFilterSection
//==============================================================================

SimpleSynthFilterSection::SimpleSynthFilterSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_filterTypeComp(plugin.filterTypeParam, "Type")
    , m_cutoffComp(plugin.filterCutoffParam, "Cutoff")
    , m_resComp(plugin.filterResParam, "Res")
    , m_driveComp(plugin.filterDriveParam, "Drive")
    , m_envAmountComp(plugin.filterEnvAmountParam, "Env Amt")
{
    m_nameLabel.setText("FILTER", juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    addAndMakeVisible(m_filterTypeComp);
    addAndMakeVisible(m_cutoffComp);
    addAndMakeVisible(m_resComp);
    addAndMakeVisible(m_driveComp);
    addAndMakeVisible(m_envAmountComp);
    
    updateUI();
}

void SimpleSynthFilterSection::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;
    g.setColour(m_appState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto trackColour = m_plugin.getOwnerTrack()->getColour();
    auto labelingCol = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    g.setColour(m_appState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, false, true, false);
}

void SimpleSynthFilterSection::resized()
{
    auto area = getLocalBounds().reduced(5);
    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX(), area.getHeight() - headerWidth, area.getHeight(), headerWidth);
    m_nameLabel.setBounds(header);
    m_nameLabel.setFont(juce::FontOptions(10));
    m_nameLabel.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), 
                                                             header.getX() + 10.0f, 
                                                             header.getY() + 10.0f));
    area.removeFromLeft(headerWidth);

    auto rowHeight = area.getHeight() / 3;
    
    // Row 1: Type
    m_filterTypeComp.setBounds(area.removeFromTop(rowHeight).reduced(2));
    
    // Row 2: Cutoff, Res
    auto row2 = area.removeFromTop(rowHeight);
    auto halfWidth = row2.getWidth() / 2;
    m_cutoffComp.setBounds(row2.removeFromLeft(halfWidth).reduced(2));
    m_resComp.setBounds(row2.reduced(2));
    
    // Row 3: Drive, EnvAmt
    auto row3 = area;
    m_driveComp.setBounds(row3.removeFromLeft(halfWidth).reduced(2));
    m_envAmountComp.setBounds(row3.reduced(2));
}

void SimpleSynthFilterSection::updateUI()
{
    bool isLadder = m_plugin.filterTypeParam->getCurrentValue() < 0.5f;
    m_driveComp.setEnabled(isLadder);
}

//==============================================================================
// SimpleSynthEnvSection
//==============================================================================

SimpleSynthEnvSection::SimpleSynthEnvSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState, const juce::String& name, bool isFilterEnv)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_attackComp(isFilterEnv ? plugin.filterAttackParam : plugin.attackParam, "A")
    , m_decayComp(isFilterEnv ? plugin.filterDecayParam : plugin.decayParam, "D")
    , m_sustainComp(isFilterEnv ? plugin.filterSustainParam : plugin.sustainParam, "S")
    , m_releaseComp(isFilterEnv ? plugin.filterReleaseParam : plugin.releaseParam, "R")
{
    m_nameLabel.setText(name, juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    addAndMakeVisible(m_attackComp);
    addAndMakeVisible(m_decayComp);
    addAndMakeVisible(m_sustainComp);
    addAndMakeVisible(m_releaseComp);
}

void SimpleSynthEnvSection::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;
    g.setColour(m_appState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto trackColour = m_plugin.getOwnerTrack()->getColour();
    auto labelingCol = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    auto header = area.removeFromLeft(15);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    g.setColour(m_appState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, false, true, false);
}

void SimpleSynthEnvSection::resized()
{
    auto area = getLocalBounds().reduced(5);
    auto headerWidth = 15;
    auto header = juce::Rectangle<int>(area.getX(), area.getHeight() - headerWidth, area.getHeight(), headerWidth);
    m_nameLabel.setBounds(header);
    m_nameLabel.setFont(juce::FontOptions(10));
    m_nameLabel.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), 
                                                             header.getX() + 10.0f, 
                                                             header.getY() + 10.0f));
    area.removeFromLeft(headerWidth);

    auto paramWidth = area.getWidth() / 4;
    m_attackComp.setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_decayComp.setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_sustainComp.setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_releaseComp.setBounds(area.reduced(2));
}

//==============================================================================
// SimpleSynthPluginComponent
//==============================================================================

SimpleSynthPluginComponent::SimpleSynthPluginComponent(EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
    , m_synth(dynamic_cast<SimpleSynthPlugin*>(p.get()))
    , m_osc1Section(*m_synth, evs.m_applicationState, 0)
    , m_osc2Section(*m_synth, evs.m_applicationState, 1)
    , m_mixSection(*m_synth, evs.m_applicationState)
    , m_filterSection(*m_synth, evs.m_applicationState)
    , m_ampEnvSection(*m_synth, evs.m_applicationState, "AMP ENV", false)
    , m_filterEnvSection(*m_synth, evs.m_applicationState, "FILTER ENV", true)
    , m_levelSlider(*m_synth->levelParam)
{
    jassert(m_synth != nullptr);
    
    addAndMakeVisible(m_osc1Section);
    addAndMakeVisible(m_osc2Section);
    addAndMakeVisible(m_mixSection);
    addAndMakeVisible(m_filterSection);
    addAndMakeVisible(m_ampEnvSection);
    addAndMakeVisible(m_filterEnvSection);
    
    m_levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_levelSlider);
    
    m_levelLabel.setText("Master", juce::dontSendNotification);
    m_levelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_levelLabel);
    
    p->state.addListener(this);
}

SimpleSynthPluginComponent::~SimpleSynthPluginComponent()
{
    if (m_synth)
        m_synth->state.removeListener(this);
}

void SimpleSynthPluginComponent::paint(juce::Graphics& g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void SimpleSynthPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    
    // Master Section (Right Side)
    auto masterArea = area.removeFromRight(60);
    m_levelLabel.setBounds(masterArea.removeFromBottom(20));
    m_levelSlider.setBounds(masterArea);
    
    area.removeFromRight(5); // Spacing
    
    // New Layout: Osc 1 (25%) | Osc 2 (20%) | Mix (10%) | Filter (25%) | Envs (20%)
    
    // We have roughly 900-1000px usually.
    // Let's divide based on content.
    
    auto totalWidth = area.getWidth();
    int osc1W = totalWidth * 0.28f;
    int osc2W = totalWidth * 0.22f; // Slightly smaller (less knobs)
    int mixW = totalWidth * 0.12f;
    int filterW = totalWidth * 0.22f;
    // Remainder for Envs (~16%)
    
    m_osc1Section.setBounds(area.removeFromLeft(osc1W));
    m_osc2Section.setBounds(area.removeFromLeft(osc2W));
    m_mixSection.setBounds(area.removeFromLeft(mixW));
    m_filterSection.setBounds(area.removeFromLeft(filterW));
    
    auto envArea = area;
    auto envHeight = envArea.getHeight() / 2;
    m_ampEnvSection.setBounds(envArea.removeFromTop(envHeight).reduced(0, 2));
    m_filterEnvSection.setBounds(envArea.reduced(0, 2));
}

void SimpleSynthPluginComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&)
{
    m_osc1Section.updateUI();
    m_osc2Section.updateUI();
    m_mixSection.updateUI();
    m_filterSection.updateUI();
}

// PluginPresetInterface implementation
juce::ValueTree SimpleSynthPluginComponent::getPluginState()
{
    return m_synth->state.createCopy();
}

juce::ValueTree SimpleSynthPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", SimpleSynthPlugin::xmlTypeName, nullptr);
    // Add default values if needed, though they are usually handled by the plugin itself
    return defaultState;
}

void SimpleSynthPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_synth->restorePluginStateFromValueTree(state);
}

juce::String SimpleSynthPluginComponent::getPresetSubfolder() const
{
    return "SimpleSynth";
}

juce::String SimpleSynthPluginComponent::getPluginTypeName() const
{
    return SimpleSynthPlugin::xmlTypeName;
}

ApplicationViewState& SimpleSynthPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}

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

SimpleSynthOscSection::SimpleSynthOscSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_coarseTuneComp(plugin.coarseTuneParam, "Tune")
    , m_fineTuneComp(plugin.fineTuneParam, "Fine")
    , m_unisonOrderComp(plugin.unisonOrderParam, "Voices")
    , m_unisonDetuneComp(plugin.unisonDetuneParam, "Detune")
    , m_unisonSpreadComp(plugin.unisonSpreadParam, "Spread")
    , m_retriggerComp(plugin.retriggerParam, "Retrig")
{
    m_nameLabel.setText("OSCILLATOR", juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    m_waveCombo.addItem("Sine", 1);
    m_waveCombo.addItem("Triangle", 2);
    m_waveCombo.addItem("Saw", 3);
    m_waveCombo.addItem("Square", 4);
    m_waveCombo.addItem("Noise", 5);
    
    // Map internal 0-based enum to 1-based ID
    m_waveCombo.setSelectedId((int)plugin.waveValue.get() + 1, juce::dontSendNotification);
    m_waveCombo.onChange = [this] {
        m_plugin.waveValue = (float)(m_waveCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(m_waveCombo);

    addAndMakeVisible(m_coarseTuneComp);
    addAndMakeVisible(m_fineTuneComp);
    addAndMakeVisible(m_unisonOrderComp);
    addAndMakeVisible(m_unisonDetuneComp);
    addAndMakeVisible(m_unisonSpreadComp);
    addAndMakeVisible(m_retriggerComp);
}

void SimpleSynthOscSection::updateUI()
{
    m_waveCombo.setSelectedId((int)m_plugin.waveValue.get() + 1, juce::dontSendNotification);
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
    auto topRow = area.removeFromTop(24);
    m_waveCombo.setBounds(topRow.reduced(5, 0));

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto paramWidth = row1.getWidth() / 3;

    m_coarseTuneComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));
    m_fineTuneComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));
    m_unisonOrderComp.setBounds(row1.removeFromLeft(paramWidth).reduced(2));

    auto row2 = area;
    paramWidth = row2.getWidth() / 3;
    m_unisonDetuneComp.setBounds(row2.removeFromLeft(paramWidth).reduced(2));
    m_unisonSpreadComp.setBounds(row2.removeFromLeft(paramWidth).reduced(2));
    m_retriggerComp.setBounds(row2.reduced(2));
}

//==============================================================================
// SimpleSynthFilterSection
//==============================================================================

SimpleSynthFilterSection::SimpleSynthFilterSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_cutoffComp(plugin.filterCutoffParam, "Cutoff")
    , m_resComp(plugin.filterResParam, "Res")
{
    m_nameLabel.setText("FILTER", juce::dontSendNotification);
    m_nameLabel.setJustificationType(juce::Justification::centred);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_nameLabel);

    addAndMakeVisible(m_cutoffComp);
    addAndMakeVisible(m_resComp);
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

    auto paramWidth = area.getWidth() / 2;
    m_cutoffComp.setBounds(area.removeFromLeft(paramWidth).reduced(2));
    m_resComp.setBounds(area.reduced(2));
}

//==============================================================================
// SimpleSynthEnvSection
//==============================================================================

SimpleSynthEnvSection::SimpleSynthEnvSection(SimpleSynthPlugin& plugin, ApplicationViewState& appState, const juce::String& name)
    : m_plugin(plugin)
    , m_appState(appState)
    , m_attackComp(plugin.attackParam, "A")
    , m_decayComp(plugin.decayParam, "D")
    , m_sustainComp(plugin.sustainParam, "S")
    , m_releaseComp(plugin.releaseParam, "R")
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
    , m_oscSection(*m_synth, evs.m_applicationState)
    , m_filterSection(*m_synth, evs.m_applicationState)
    , m_ampEnvSection(*m_synth, evs.m_applicationState, "AMP ENV")
    , m_levelSlider(*m_synth->levelParam)
{
    jassert(m_synth != nullptr);
    
    addAndMakeVisible(m_oscSection);
    addAndMakeVisible(m_filterSection);
    addAndMakeVisible(m_ampEnvSection);
    
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
    
    // Split remaining width
    auto oscWidth = area.getWidth() * 0.4f; // 40% for OSC
    auto filterWidth = area.getWidth() * 0.25f; // 25% for Filter
    
    m_oscSection.setBounds(area.removeFromLeft(oscWidth));
    m_filterSection.setBounds(area.removeFromLeft(filterWidth));
    m_ampEnvSection.setBounds(area); // Rest for Envelope
}

void SimpleSynthPluginComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&)
{
    m_oscSection.updateUI();
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

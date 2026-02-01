/*
  ==============================================================================

    ArpeggiatorPluginComponent.cpp
    Created: 26 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Arpeggiator/ArpeggiatorPluginComponent.h"

using namespace tracktion_engine;

ArpeggiatorPluginComponent::ArpeggiatorPluginComponent(EditViewState &evs, Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_arpeggiator(dynamic_cast<ArpeggiatorPlugin *>(p.get()))
{
    if (m_arpeggiator != nullptr)
    {
        m_arpeggiator->state.addListener(this);

        m_modeComp = std::make_unique<AutomatableChoiceComponent>(m_arpeggiator->modeParam, "Mode");
        addAndMakeVisible(*m_modeComp);

        m_rateComp = std::make_unique<AutomatableChoiceComponent>(m_arpeggiator->rateParam, "Rate");
        addAndMakeVisible(*m_rateComp);

        m_octaveComp = std::make_unique<AutomatableChoiceComponent>(m_arpeggiator->octaveParam, "Octave");
        addAndMakeVisible(*m_octaveComp);

        m_gateComp = std::make_unique<AutomatableParameterComponent>(m_arpeggiator->gateParam, "Gate");
        addAndMakeVisible(*m_gateComp);
    }

    m_titleLabel.setText("ARPEGGIATOR", juce::dontSendNotification);
    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::FontOptions(12, juce::Font::bold));
    addAndMakeVisible(m_titleLabel);
}

ArpeggiatorPluginComponent::~ArpeggiatorPluginComponent()
{
    if (m_arpeggiator)
        m_arpeggiator->state.removeListener(this);
}

void ArpeggiatorPluginComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds().reduced(5);
    auto cornerSize = 10.0f;

    // Background
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, true, true, true);

    // Header Background (matching track color)
    auto trackColour = getTrackColour();
    auto header = area.removeFromTop(20);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, true, false, false);

    // Label Colour
    auto labelingCol = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_titleLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    // Border
    g.setColour(m_editViewState.m_applicationState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, true, true, true);
}

void ArpeggiatorPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(5);

    // Header
    auto headerHeight = 20;
    m_titleLabel.setBounds(area.removeFromTop(headerHeight));

    area.reduce(5, 5);

    auto rowHeight = area.getHeight() / 3;

    // Row 1: Mode
    m_modeComp->setBounds(area.removeFromTop(rowHeight).reduced(2));

    // Row 2: Rate
    m_rateComp->setBounds(area.removeFromTop(rowHeight).reduced(2));

    // Row 3: Octave and Gate
    auto row3 = area;
    auto halfWidth = row3.getWidth() / 2;
    m_octaveComp->setBounds(row3.removeFromLeft(halfWidth).reduced(2));
    m_gateComp->setBounds(row3.reduced(2));
}

juce::ValueTree ArpeggiatorPluginComponent::getPluginState()
{
    if (m_arpeggiator)
        return m_arpeggiator->state;
    return {};
}

juce::ValueTree ArpeggiatorPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree v(ArpeggiatorPlugin::xmlTypeName);
    // Defaults are handled in plugin constructor
    return v;
}

void ArpeggiatorPluginComponent::restorePluginState(const juce::ValueTree &state)
{
    if (m_arpeggiator)
        m_arpeggiator->restorePluginStateFromValueTree(state);
}

juce::String ArpeggiatorPluginComponent::getPresetSubfolder() const { return "Arpeggiator"; }

juce::String ArpeggiatorPluginComponent::getPluginTypeName() const { return ArpeggiatorPlugin::xmlTypeName; }

ApplicationViewState &ArpeggiatorPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void ArpeggiatorPluginComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (m_arpeggiator && v == m_arpeggiator->state)
    {
        // Update UI if needed, but automatable components handle themselves mostly
    }
}

/*
  ==============================================================================

    ChorusPluginComponent.cpp
    Created: 15 Feb 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Chorus/ChorusPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

#include <cmath>

namespace
{
constexpr float minPanelWidth = 50.0f;
constexpr float minPanelHeight = 40.0f;
constexpr float panelPadding = 4.0f;
constexpr float headerHeight = 22.0f;
} // namespace

class ChorusPluginComponent::ChorusFieldGraphComponent : public juce::Component
{
public:
    ChorusFieldGraphComponent(ChorusPluginComponent &owner, te::AutomatableParameter::Ptr depth, te::AutomatableParameter::Ptr rate, te::AutomatableParameter::Ptr width, te::AutomatableParameter::Ptr mix)
        : m_owner(owner),
          m_depth(std::move(depth)),
          m_rate(std::move(rate)),
          m_width(std::move(width)),
          m_mix(std::move(mix))
    {
    }

    void paint(juce::Graphics &g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < minPanelWidth || bounds.getHeight() < minPanelHeight)
            return;

        auto panel = bounds.reduced(panelPadding);
        const auto trackColour = m_owner.getTrackColour();
        auto &appState = m_owner.m_editViewState.m_applicationState;
        GUIHelpers::drawHeaderBox(g, panel, trackColour, appState.getBorderColour(), appState.getBackgroundColour1(), headerHeight, GUIHelpers::HeaderPosition::top);

        auto header = panel.removeFromTop(headerHeight).toNearestInt();
        g.setColour(trackColour.contrasting(0.9f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText("CHORUS FIELD", header.reduced(8, 0), juce::Justification::centredLeft, 1);

        const juce::String rateText = m_rate != nullptr ? m_rate->getCurrentValueAsString() : juce::String("1.00 Hz");
        const juce::String mixText = m_mix != nullptr ? m_mix->getCurrentValueAsString() : juce::String("50%");
        g.drawFittedText("Rate " + rateText + "  |  Mix " + mixText, header.reduced(8, 0), juce::Justification::centredRight, 1);

        auto graph = panel.reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff1a212b));
        g.fillRoundedRectangle(graph, 6.0f);

        const float depthMs = m_depth != nullptr ? juce::jlimit(0.0f, 30.0f, m_depth->getCurrentValue()) : 3.0f;
        const float rateHz = m_rate != nullptr ? juce::jlimit(0.02f, 10.0f, m_rate->getCurrentValue()) : 1.0f;
        const float width = m_width != nullptr ? juce::jlimit(0.0f, 1.0f, m_width->getCurrentValue()) : 0.5f;
        const float mix = m_mix != nullptr ? juce::jlimit(0.0f, 1.0f, m_mix->getCurrentValue()) : 0.5f;

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.08f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = graph.getX() + graph.getWidth() * ((float)i / 8.0f);
            g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        }

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.12f));
        g.drawHorizontalLine((int)(graph.getY() + graph.getHeight() * 0.35f), graph.getX(), graph.getRight());
        g.drawHorizontalLine((int)(graph.getY() + graph.getHeight() * 0.65f), graph.getX(), graph.getRight());

        const float depthNorm = juce::jlimit(0.0f, 1.0f, depthMs / 30.0f);
        // Visual-only scaling: faster rates draw more visible cycles in the panel.
        const float cycleCount = juce::jmap(rateHz, 0.02f, 10.0f, 0.75f, 3.25f);
        const float amp = graph.getHeight() * (0.04f + depthNorm * 0.18f);
        // Width is rendered as inter-channel phase spread.
        const float phaseOffset = juce::MathConstants<float>::pi * width;
        const float glow = juce::jmap(mix, 0.0f, 1.0f, 0.18f, 0.85f);

        juce::Path pathL;
        juce::Path pathR;
        const int steps = 96;

        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float)i / (float)steps;
            const float x = graph.getX() + graph.getWidth() * t;
            const float p = t * juce::MathConstants<float>::twoPi * cycleCount;
            const float yL = graph.getY() + graph.getHeight() * 0.35f + std::sin(p) * amp;
            const float yR = graph.getY() + graph.getHeight() * 0.65f + std::sin(p + phaseOffset) * amp;

            if (i == 0)
            {
                pathL.startNewSubPath(x, yL);
                pathR.startNewSubPath(x, yR);
            }
            else
            {
                pathL.lineTo(x, yL);
                pathR.lineTo(x, yR);
            }
        }

        g.setColour(trackColour.brighter(0.55f).withAlpha(glow * 0.35f));
        g.strokePath(pathL, juce::PathStrokeType(3.0f));
        g.strokePath(pathR, juce::PathStrokeType(3.0f));

        g.setColour(trackColour.brighter(0.25f).withAlpha(glow));
        g.strokePath(pathL, juce::PathStrokeType(1.25f));
        g.strokePath(pathR, juce::PathStrokeType(1.25f));

        const float markerX = graph.getX() + graph.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.18f, 0.82f);
        const float markerRadius = 3.0f + mix * 4.0f;
        g.setColour(trackColour.withAlpha(0.7f));
        g.fillEllipse(markerX - markerRadius, graph.getCentreY() - markerRadius, markerRadius * 2.0f, markerRadius * 2.0f);

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.58f));
        g.setFont(10.0f);
        g.drawFittedText("L", juce::Rectangle<int>((int)graph.getX() + 4, (int)(graph.getY() + 4), 12, 12), juce::Justification::centred, 1);
        g.drawFittedText("R", juce::Rectangle<int>((int)graph.getX() + 4, (int)(graph.getBottom() - 16), 12, 12), juce::Justification::centred, 1);
    }

private:
    ChorusPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_depth;
    te::AutomatableParameter::Ptr m_rate;
    te::AutomatableParameter::Ptr m_width;
    te::AutomatableParameter::Ptr m_mix;
};

ChorusPluginComponent::ChorusPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_graph = std::make_unique<ChorusFieldGraphComponent>(*this, m_plugin->getAutomatableParameterByID(NextChorusPlugin::depthMsParamID), m_plugin->getAutomatableParameterByID(NextChorusPlugin::speedHzParamID), m_plugin->getAutomatableParameterByID(NextChorusPlugin::widthParamID), m_plugin->getAutomatableParameterByID(NextChorusPlugin::mixProportionParamID));

    m_depth = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextChorusPlugin::depthMsParamID), "Depth");
    m_rate = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextChorusPlugin::speedHzParamID), "Rate");
    m_width = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextChorusPlugin::widthParamID), "Width");
    m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextChorusPlugin::mixProportionParamID), "Mix");

    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_depth);
    addAndMakeVisible(*m_rate);
    addAndMakeVisible(*m_width);
    addAndMakeVisible(*m_mix);

    m_plugin->state.addListener(this);
}

ChorusPluginComponent::~ChorusPluginComponent() { m_plugin->state.removeListener(this); }

void ChorusPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void ChorusPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(4);

    auto graphArea = area.removeFromTop((int)(area.getHeight() * 0.46f));
    m_graph->setBounds(graphArea);

    area.removeFromTop(4);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    auto colW1 = row1.getWidth() / 2;
    m_depth->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_rate->setBounds(row1.reduced(2));

    auto colW2 = row2.getWidth() / 2;
    m_width->setBounds(row2.removeFromLeft(colW2).reduced(2));
    m_mix->setBounds(row2.reduced(2));
}

juce::ValueTree ChorusPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree ChorusPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", NextChorusPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void ChorusPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String ChorusPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String ChorusPluginComponent::getPluginTypeName() const { return NextChorusPlugin::xmlTypeName; }

ApplicationViewState &ChorusPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void ChorusPluginComponent::valueTreeChanged() {}

void ChorusPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    static const juce::Identifier depthId(NextChorusPlugin::depthMsParamID);
    static const juce::Identifier rateId(NextChorusPlugin::speedHzParamID);
    static const juce::Identifier widthId(NextChorusPlugin::widthParamID);
    static const juce::Identifier mixId(NextChorusPlugin::mixProportionParamID);

    if (i == depthId && m_depth)
        m_depth->updateLabel();
    else if (i == rateId && m_rate)
        m_rate->updateLabel();
    else if (i == widthId && m_width)
        m_width->updateLabel();
    else if (i == mixId && m_mix)
        m_mix->updateLabel();

    if (m_graph)
        m_graph->repaint();
}

void ChorusPluginComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) {}

void ChorusPluginComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) {}

void ChorusPluginComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) {}

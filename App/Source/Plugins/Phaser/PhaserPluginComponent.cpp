#include "Plugins/Phaser/PhaserPluginComponent.h"

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

class PhaserPluginComponent::PhaserSweepGraphComponent : public juce::Component
{
public:
    PhaserSweepGraphComponent(PhaserPluginComponent &owner, te::AutomatableParameter::Ptr depth, te::AutomatableParameter::Ptr rate, te::AutomatableParameter::Ptr feedback, te::AutomatableParameter::Ptr mix)
        : m_owner(owner),
          m_depth(std::move(depth)),
          m_rate(std::move(rate)),
          m_feedback(std::move(feedback)),
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
        g.drawFittedText("PHASER SWEEP", header.reduced(8, 0), juce::Justification::centredLeft, 1);

        const juce::String rateText = m_rate != nullptr ? m_rate->getCurrentValueAsString() : juce::String("0.45 Hz");
        const juce::String mixText = m_mix != nullptr ? m_mix->getCurrentValueAsString() : juce::String("60%");
        g.drawFittedText("Rate " + rateText + "  |  Mix " + mixText, header.reduced(8, 0), juce::Justification::centredRight, 1);

        auto graph = panel.reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff1a212b));
        g.fillRoundedRectangle(graph, 6.0f);

        const float depth = m_depth != nullptr ? juce::jlimit(0.0f, 1.0f, m_depth->getCurrentValue()) : 0.55f;
        const float rateHz = m_rate != nullptr ? juce::jlimit(0.02f, 10.0f, m_rate->getCurrentValue()) : 0.45f;
        const float feedback = m_feedback != nullptr ? juce::jlimit(-0.95f, 0.95f, m_feedback->getCurrentValue()) : 0.35f;
        const float mix = m_mix != nullptr ? juce::jlimit(0.0f, 1.0f, m_mix->getCurrentValue()) : 0.6f;

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.08f));
        for (int i = 1; i < 10; ++i)
        {
            const float x = graph.getX() + graph.getWidth() * ((float)i / 10.0f);
            g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        }

        const float cycleCount = juce::jmap(rateHz, 0.02f, 10.0f, 1.0f, 5.0f);
        const float notchDepth = graph.getHeight() * (0.08f + depth * 0.34f);
        const float feedbackBoost = juce::jmap(std::abs(feedback), 0.0f, 0.95f, 1.0f, 1.55f);

        juce::Path sweep;
        const int steps = 140;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float)i / (float)steps;
            const float x = graph.getX() + graph.getWidth() * t;
            const float p = t * juce::MathConstants<float>::twoPi * cycleCount;
            const float notch = std::sin(p) * std::cos(p * 0.5f + feedback * juce::MathConstants<float>::pi * 0.5f);
            const float y = graph.getCentreY() + notch * notchDepth * feedbackBoost;

            if (i == 0)
                sweep.startNewSubPath(x, y);
            else
                sweep.lineTo(x, y);
        }

        const float glow = juce::jmap(mix, 0.0f, 1.0f, 0.2f, 0.9f);
        g.setColour(trackColour.brighter(0.6f).withAlpha(glow * 0.4f));
        g.strokePath(sweep, juce::PathStrokeType(3.5f));

        g.setColour(trackColour.brighter(0.3f).withAlpha(glow));
        g.strokePath(sweep, juce::PathStrokeType(1.2f));

        const float markerX = graph.getX() + graph.getWidth() * juce::jmap(feedback, -0.95f, 0.95f, 0.1f, 0.9f);
        const float markerRadius = 2.5f + depth * 4.5f;
        g.setColour(trackColour.withAlpha(0.72f));
        g.fillEllipse(markerX - markerRadius, graph.getCentreY() - markerRadius, markerRadius * 2.0f, markerRadius * 2.0f);
    }

private:
    PhaserPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_depth;
    te::AutomatableParameter::Ptr m_rate;
    te::AutomatableParameter::Ptr m_feedback;
    te::AutomatableParameter::Ptr m_mix;
};

PhaserPluginComponent::PhaserPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_graph = std::make_unique<PhaserSweepGraphComponent>(*this, m_plugin->getAutomatableParameterByID(NextPhaserPlugin::depthParamID), m_plugin->getAutomatableParameterByID(NextPhaserPlugin::rateParamID), m_plugin->getAutomatableParameterByID(NextPhaserPlugin::feedbackParamID), m_plugin->getAutomatableParameterByID(NextPhaserPlugin::mixParamID));

    m_depth = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextPhaserPlugin::depthParamID), "Depth");
    m_rate = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextPhaserPlugin::rateParamID), "Rate");
    m_feedback = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextPhaserPlugin::feedbackParamID), "Feedback");
    m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextPhaserPlugin::mixParamID), "Mix");

    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_depth);
    addAndMakeVisible(*m_rate);
    addAndMakeVisible(*m_feedback);
    addAndMakeVisible(*m_mix);

    m_plugin->state.addListener(this);
}

PhaserPluginComponent::~PhaserPluginComponent() { m_plugin->state.removeListener(this); }

void PhaserPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void PhaserPluginComponent::resized()
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
    m_feedback->setBounds(row2.removeFromLeft(colW2).reduced(2));
    m_mix->setBounds(row2.reduced(2));
}

juce::ValueTree PhaserPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree PhaserPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", NextPhaserPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void PhaserPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String PhaserPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String PhaserPluginComponent::getPluginTypeName() const { return NextPhaserPlugin::xmlTypeName; }

ApplicationViewState &PhaserPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void PhaserPluginComponent::valueTreeChanged() {}

void PhaserPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    static const juce::Identifier depthId(NextPhaserPlugin::depthParamID);
    static const juce::Identifier rateId(NextPhaserPlugin::rateParamID);
    static const juce::Identifier feedbackId(NextPhaserPlugin::feedbackParamID);
    static const juce::Identifier mixId(NextPhaserPlugin::mixParamID);

    if (i == depthId && m_depth)
        m_depth->updateLabel();
    else if (i == rateId && m_rate)
        m_rate->updateLabel();
    else if (i == feedbackId && m_feedback)
        m_feedback->updateLabel();
    else if (i == mixId && m_mix)
        m_mix->updateLabel();

    if (m_graph)
        m_graph->repaint();
}

void PhaserPluginComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) {}

void PhaserPluginComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) {}

void PhaserPluginComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) {}

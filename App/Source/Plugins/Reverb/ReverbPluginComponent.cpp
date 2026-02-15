/*
  ==============================================================================

    ReverbPluginComponent.cpp
    Created: 14 Feb 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Reverb/ReverbPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

#include <cmath>

namespace
{
constexpr float minPanelWidth = 50.0f;
constexpr float minPanelHeight = 40.0f;
constexpr float panelPadding = 4.0f;
constexpr float headerHeight = 22.0f;
constexpr float chamberInset = 8.0f;
constexpr float chamberCorner = 6.0f;
constexpr int gridLines = 6;
} // namespace

class ReverbPluginComponent::ReverbSpaceGraphComponent : public juce::Component
{
public:
    ReverbSpaceGraphComponent(ReverbPluginComponent &owner, te::AutomatableParameter::Ptr roomSize, te::AutomatableParameter::Ptr damping, te::AutomatableParameter::Ptr wet, te::AutomatableParameter::Ptr dry, te::AutomatableParameter::Ptr width, te::AutomatableParameter::Ptr mode)
        : m_owner(owner),
          m_roomSize(std::move(roomSize)),
          m_damping(std::move(damping)),
          m_wet(std::move(wet)),
          m_dry(std::move(dry)),
          m_width(std::move(width)),
          m_mode(std::move(mode))
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

        auto header = panel.removeFromTop(headerHeight);

        g.setColour(trackColour.contrasting(0.9f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText("REVERB CHAMBER", header.toNearestInt().reduced(8, 0), juce::Justification::centredLeft, 1);

        const float wet = m_wet != nullptr ? juce::jlimit(0.0f, 1.0f, m_wet->getCurrentValue()) : 0.33f;
        const float dry = m_dry != nullptr ? juce::jlimit(0.0f, 1.0f, m_dry->getCurrentValue()) : 0.5f;
        const juce::String modeText = m_mode != nullptr ? m_mode->getCurrentValueAsString() : "Off";
        g.drawFittedText("Wet " + juce::String((int)std::round(wet * 100.0f)) + "%  Dry " + juce::String((int)std::round(dry * 100.0f)) + "%  Freeze " + modeText, header.toNearestInt().reduced(8, 0), juce::Justification::centredRight, 1);

        auto chamber = panel.reduced(chamberInset, chamberInset).toNearestInt();
        g.setColour(juce::Colour(0xff18202a));
        g.fillRoundedRectangle(chamber.toFloat(), chamberCorner);

        g.saveState();
        g.reduceClipRegion(chamber);

        const float roomSize = m_roomSize != nullptr ? juce::jlimit(0.0f, 1.0f, m_roomSize->getCurrentValue()) : 0.3f;
        const float damping = m_damping != nullptr ? juce::jlimit(0.0f, 1.0f, m_damping->getCurrentValue()) : 0.5f;
        const float width = m_width != nullptr ? juce::jlimit(0.0f, 1.0f, m_width->getCurrentValue()) : 1.0f;
        const bool freezeOn = m_mode != nullptr && m_mode->getCurrentValue() >= 0.5f;

        const float energy = juce::jlimit(0.0f, 1.0f, wet * 0.7f + roomSize * 0.3f);
        const int baseEdges = juce::jlimit(4, 10, 4 + (int)std::round(roomSize * 6.0f));
        const int edges = juce::jlimit(4, 20, (int)std::round(baseEdges * (1.0f + roomSize)));
        const float tilt = 0.35f + roomSize;
        const float rotation = 0.2f + roomSize * 0.3f;
        const float radius = 0.5f + roomSize * 1.0f;
        const float height = 0.1f + damping * 0.6f;
        const float strokeThickness = juce::jmap(std::pow(roomSize, 0.6f), 0.0f, 1.0f, 0.55f, 2.5f);

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.07f));
        for (int i = 1; i < gridLines; ++i)
        {
            const float y = (float)chamber.getY() + chamber.getHeight() * ((float)i / (float)gridLines);
            g.drawHorizontalLine((int)y, (float)chamber.getX(), (float)chamber.getRight());
        }

        const int insetPx = juce::jlimit(6, 14, (int)std::round(14.0f - roomSize * 8.0f));
        auto inset = chamber.reduced(insetPx);
        if (inset.getWidth() > 24 && inset.getHeight() > 24)
        {
            const auto localArea = juce::Rectangle<int>(0, 0, inset.getWidth(), inset.getHeight());
            const float bodySeparation = chamber.getWidth() * 0.42f * width;
            const float bodyScale = 0.95f + roomSize * 0.4f;
            const float bodyAlpha = juce::jlimit(0.28f, 1.0f, 0.46f + energy * 0.44f);
            const auto bodyOrigin = inset.toFloat().getCentre();

            auto drawBody = [&](float xOffset, float alphaScale)
            {
                const float alpha = juce::jlimit(0.0f, 1.0f, bodyAlpha * alphaScale);
                g.saveState();
                g.addTransform(juce::AffineTransform::translation(bodyOrigin.x + xOffset - inset.getWidth() * 0.5f, bodyOrigin.y - inset.getHeight() * 0.5f));

                g.setColour(trackColour.withBrightness(1.3f).withAlpha(alpha * 0.34f));
                GUIHelpers::drawPolyObject(g, localArea, edges, tilt, juce::jlimit(0.0f, 1.0f, rotation + (freezeOn ? 0.12f : 0.0f)), radius, height, bodyScale, strokeThickness * 2.1f);

                g.setColour(trackColour.withBrightness(1.3f).withAlpha(alpha));
                GUIHelpers::drawPolyObject(g, localArea, edges, tilt, juce::jlimit(0.0f, 1.0f, rotation + (freezeOn ? 0.12f : 0.0f)), radius, height, bodyScale, strokeThickness * .75f);
                g.restoreState();
            };

            drawBody(-bodySeparation * 0.5f, 1.0f);
            drawBody(bodySeparation * 0.5f, 0.92f);
        }

        if (freezeOn)
        {
            g.setColour(trackColour.brighter(0.65f).withAlpha(0.34f));
            auto freezeArea = chamber.reduced(10);
            g.drawRoundedRectangle(freezeArea.toFloat(), 6.0f, strokeThickness);
            g.drawFittedText("FREEZE", freezeArea.removeFromBottom(16), juce::Justification::centred, 1);
        }

        g.restoreState();
    }

private:
    ReverbPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_roomSize, m_damping, m_wet, m_dry, m_width, m_mode;
};

ReverbPluginComponent::ReverbPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_graph = std::make_unique<ReverbSpaceGraphComponent>(*this, m_plugin->getAutomatableParameterByID("room size"), m_plugin->getAutomatableParameterByID("damping"), m_plugin->getAutomatableParameterByID("wet level"), m_plugin->getAutomatableParameterByID("dry level"), m_plugin->getAutomatableParameterByID("width"), m_plugin->getAutomatableParameterByID("mode"));

    m_roomSize = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("room size"), "Room");
    m_damping = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("damping"), "Damp");
    m_wet = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("wet level"), "Wet");
    m_dry = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("dry level"), "Dry");
    m_width = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("width"), "Width");
    m_mode = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("mode"), "Freeze");

    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_roomSize);
    addAndMakeVisible(*m_damping);
    addAndMakeVisible(*m_wet);
    addAndMakeVisible(*m_dry);
    addAndMakeVisible(*m_width);
    addAndMakeVisible(*m_mode);

    m_plugin->state.addListener(this);
}

ReverbPluginComponent::~ReverbPluginComponent() { m_plugin->state.removeListener(this); }

void ReverbPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void ReverbPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(4);

    auto graphArea = area.removeFromTop((int)(area.getHeight() * 0.46f));
    m_graph->setBounds(graphArea);

    area.removeFromTop(4);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    auto colW1 = row1.getWidth() / 3;
    m_roomSize->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_damping->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_width->setBounds(row1.reduced(2));

    auto colW2 = row2.getWidth() / 3;
    m_wet->setBounds(row2.removeFromLeft(colW2).reduced(2));
    m_dry->setBounds(row2.removeFromLeft(colW2).reduced(2));
    m_mode->setBounds(row2.reduced(2));
}

juce::ValueTree ReverbPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree ReverbPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", te::ReverbPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void ReverbPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String ReverbPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String ReverbPluginComponent::getPluginTypeName() const { return te::ReverbPlugin::xmlTypeName; }

ApplicationViewState &ReverbPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void ReverbPluginComponent::valueTreeChanged() {}

void ReverbPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    if (i == te::IDs::roomSize && m_roomSize)
        m_roomSize->updateLabel();
    else if (i == te::IDs::damp && m_damping)
        m_damping->updateLabel();
    else if (i == te::IDs::wet && m_wet)
        m_wet->updateLabel();
    else if (i == te::IDs::dry && m_dry)
        m_dry->updateLabel();
    else if (i == te::IDs::width && m_width)
        m_width->updateLabel();
    if (m_graph)
        m_graph->repaint();
}

void ReverbPluginComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) {}

void ReverbPluginComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) {}

void ReverbPluginComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) {}

/*
  ==============================================================================

    FilterPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Filter/FilterPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

#include <cmath>

namespace
{
constexpr float minPanelWidth = 50.0f;
constexpr float minPanelHeight = 40.0f;
constexpr float panelPadding = 4.0f;
constexpr float headerHeight = 22.0f;

int slopeToStages(int slopeValue) { return juce::jlimit(1, 4, slopeValue + 1); }

juce::String formatFrequencyLabel(float hz)
{
    if (hz >= 1000.0f)
    {
        const auto khz = hz / 1000.0f;
        if (khz >= 10.0f)
            return juce::String((int)std::round(khz)) + "kHz";
        return juce::String(khz, 2) + "kHz";
    }

    return juce::String((int)std::round(hz)) + "Hz";
}
} // namespace

class FilterPluginComponent::FilterTransferGraphComponent : public juce::Component
{
public:
    FilterTransferGraphComponent(FilterPluginComponent &owner, te::AutomatableParameter::Ptr freq, te::AutomatableParameter::Ptr resonance, te::AutomatableParameter::Ptr mode, te::AutomatableParameter::Ptr slope)
        : m_owner(owner),
          m_freq(std::move(freq)),
          m_resonance(std::move(resonance)),
          m_mode(std::move(mode)),
          m_slope(std::move(slope))
    {
    }

    void mouseDown(const juce::MouseEvent &e) override
    {
        m_draggingHandle = isHandleAtPosition(e.position);
        m_hoverHandle = m_draggingHandle;
        if (m_draggingHandle)
            updateParamsFromPosition(e.position);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (!m_draggingHandle)
            return;

        updateParamsFromPosition(e.position);
        repaint();
    }

    void mouseUp(const juce::MouseEvent &) override
    {
        m_draggingHandle = false;
        repaint();
    }

    void mouseMove(const juce::MouseEvent &e) override
    {
        const auto newHover = isHandleAtPosition(e.position);
        if (newHover != m_hoverHandle)
        {
            m_hoverHandle = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent &) override
    {
        if (!m_draggingHandle && m_hoverHandle)
        {
            m_hoverHandle = false;
            repaint();
        }
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
        g.drawFittedText("FILTER", header.reduced(8, 0), juce::Justification::centredLeft, 1);

        const int modeValue = m_mode != nullptr ? juce::jlimit(0, 1, (int)std::round(m_mode->getCurrentValue())) : 0;
        const int slopeValue = m_slope != nullptr ? juce::jlimit(0, 3, (int)std::round(m_slope->getCurrentValue())) : 1;
        const juce::String modeText = modeValue == NextFilterPlugin::highpass ? "Highpass" : "Lowpass";
        const juce::String slopeText = juce::String(slopeToStages(slopeValue) * 12) + " dB";
        g.drawFittedText(modeText + "  |  " + slopeText, header.reduced(8, 0), juce::Justification::centredRight, 1);

        auto graph = panel.reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff1a212b));
        g.fillRoundedRectangle(graph, 6.0f);

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.08f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = graph.getX() + graph.getWidth() * ((float)i / 8.0f);
            g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        }

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.1f));
        g.drawHorizontalLine((int)(graph.getY() + graph.getHeight() * 0.2f), graph.getX(), graph.getRight());
        g.drawHorizontalLine((int)(graph.getY() + graph.getHeight() * 0.5f), graph.getX(), graph.getRight());
        g.drawHorizontalLine((int)(graph.getY() + graph.getHeight() * 0.8f), graph.getX(), graph.getRight());

        const float freq = juce::jlimit(20.0f, 20000.0f, m_freq != nullptr ? m_freq->getCurrentValue() : 1000.0f);
        const float resonance = juce::jlimit(0.3f, 6.0f, m_resonance != nullptr ? m_resonance->getCurrentValue() : 0.71f);
        const int stages = slopeToStages(slopeValue);

        constexpr float minF = 20.0f;
        constexpr float maxF = 20000.0f;
        constexpr float minDb = -48.0f;
        constexpr float maxDb = 6.0f;

        auto mapFreqToX = [graph](float hz)
        {
            const float norm = std::log(hz / minF) / std::log(maxF / minF);
            return graph.getX() + graph.getWidth() * juce::jlimit(0.0f, 1.0f, norm);
        };

        auto mapDbToY = [graph](float db)
        {
            const float norm = juce::jmap(juce::jlimit(minDb, maxDb, db), minDb, maxDb, 1.0f, 0.0f);
            return graph.getY() + graph.getHeight() * norm;
        };

        juce::Path response;
        const int steps = 200;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float)i / (float)steps;
            const float hz = minF * std::pow(maxF / minF, t);
            const float ratio = hz / freq;

            const float den = std::sqrt((1.0f - ratio * ratio) * (1.0f - ratio * ratio) + (ratio / resonance) * (ratio / resonance));
            const float stageMag = modeValue == NextFilterPlugin::highpass ? (ratio * ratio) / juce::jmax(den, 0.000001f) : 1.0f / juce::jmax(den, 0.000001f);

            const float totalMag = std::pow(stageMag, (float)stages);
            const float db = juce::Decibels::gainToDecibels(juce::jmax(0.00001f, totalMag));
            const float x = mapFreqToX(hz);
            const float y = mapDbToY(db);

            if (i == 0)
                response.startNewSubPath(x, y);
            else
                response.lineTo(x, y);
        }

        juce::Path fill(response);
        fill.lineTo(graph.getRight(), graph.getBottom());
        fill.lineTo(graph.getX(), graph.getBottom());
        fill.closeSubPath();

        g.setColour(trackColour.withAlpha(0.20f));
        g.fillPath(fill);

        g.setColour(trackColour.brighter(0.55f).withAlpha(0.34f));
        g.strokePath(response, juce::PathStrokeType(3.0f));
        g.setColour(trackColour.brighter(0.2f).withAlpha(0.94f));
        g.strokePath(response, juce::PathStrokeType(1.2f));

        const float cutoffX = mapFreqToX(freq);
        const float cutoffY = yForResonance(graph, resonance);
        g.setColour(trackColour.withAlpha(0.72f));
        g.drawVerticalLine((int)cutoffX, graph.getY(), graph.getBottom());

        const bool handleActive = (m_draggingHandle || m_hoverHandle);
        const float radius = handleActive ? 7.0f : 6.0f;

        g.setColour(juce::Colour(0xff1f2229));
        g.fillEllipse(cutoffX - radius, cutoffY - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(trackColour);
        g.drawEllipse(cutoffX - radius, cutoffY - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        g.setColour(trackColour);
        g.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
        g.drawText("F", juce::Rectangle<float>(cutoffX - 8.0f, cutoffY - 22.0f, 16.0f, 14.0f).toNearestInt(), juce::Justification::centred, false);

        if (handleActive)
        {
            const auto freqText = formatFrequencyLabel(freq);
            const auto resText = "Q " + juce::String(resonance, 2);
            const auto labelText = "F  " + freqText + "  " + resText + "  " + slopeText;

            g.setFont(juce::FontOptions(11.0f));
            const int textWidth = juce::jmax(120, g.getCurrentFont().getStringWidth(labelText) + 12);
            const int textHeight = 18;

            float labelX = cutoffX - (float)textWidth * 0.5f;
            float labelY = cutoffY - 30.0f;
            labelX = juce::jlimit(graph.getX(), graph.getRight() - (float)textWidth, labelX);
            labelY = juce::jlimit(graph.getY(), graph.getBottom() - (float)textHeight, labelY);

            const auto bubble = juce::Rectangle<float>(labelX, labelY, (float)textWidth, (float)textHeight);
            g.setColour(juce::Colour(0xee14171d));
            g.fillRoundedRectangle(bubble, 5.0f);
            g.setColour(trackColour.withAlpha(0.9f));
            g.drawRoundedRectangle(bubble, 5.0f, 1.0f);
            g.setColour(juce::Colours::white.withAlpha(0.92f));
            g.drawText(labelText, bubble.toNearestInt(), juce::Justification::centred, false);
        }
    }

private:
    juce::Rectangle<float> getGraphArea() const
    {
        auto panel = getLocalBounds().toFloat().reduced(panelPadding);
        panel.removeFromTop(headerHeight);
        return panel.reduced(8.0f, 6.0f);
    }

    bool isHandleAtPosition(juce::Point<float> position) const
    {
        if (m_freq == nullptr)
            return false;

        const auto graph = getGraphArea();
        const float x = xForFrequency(graph, m_freq->getCurrentValue());
        const float y = yForResonance(graph, m_resonance != nullptr ? m_resonance->getCurrentValue() : 0.71f);
        return position.getDistanceFrom({x, y}) <= 22.0f;
    }

    void updateParamsFromPosition(juce::Point<float> position)
    {
        const auto graph = getGraphArea();
        if (m_freq != nullptr)
            m_freq->setParameter(frequencyForX(graph, position.x), juce::sendNotification);
        if (m_resonance != nullptr)
            m_resonance->setParameter(resonanceForY(graph, position.y), juce::sendNotification);
    }

    float xForFrequency(const juce::Rectangle<float> &area, float frequency) const
    {
        constexpr float minF = 20.0f;
        constexpr float maxF = 20000.0f;
        const float norm = std::log(juce::jlimit(minF, maxF, frequency) / minF) / std::log(maxF / minF);
        return area.getX() + area.getWidth() * juce::jlimit(0.0f, 1.0f, norm);
    }

    float frequencyForX(const juce::Rectangle<float> &area, float x) const
    {
        constexpr float minF = 20.0f;
        constexpr float maxF = 20000.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, (x - area.getX()) / juce::jmax(1.0f, area.getWidth()));
        return minF * std::pow(maxF / minF, norm);
    }

    float yForResonance(const juce::Rectangle<float> &area, float resonance) const
    {
        constexpr float minQ = 0.3f;
        constexpr float maxQ = 6.0f;
        const float norm = (std::log(juce::jlimit(minQ, maxQ, resonance)) - std::log(minQ)) / (std::log(maxQ) - std::log(minQ));
        return area.getBottom() - norm * area.getHeight();
    }

    float resonanceForY(const juce::Rectangle<float> &area, float y) const
    {
        constexpr float minQ = 0.3f;
        constexpr float maxQ = 6.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, (area.getBottom() - y) / juce::jmax(1.0f, area.getHeight()));
        return minQ * std::pow(maxQ / minQ, norm);
    }

    FilterPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_freq;
    te::AutomatableParameter::Ptr m_resonance;
    te::AutomatableParameter::Ptr m_mode;
    te::AutomatableParameter::Ptr m_slope;
    bool m_draggingHandle = false;
    bool m_hoverHandle = false;
};

FilterPluginComponent::FilterPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_freqPar = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextFilterPlugin::frequencyParamID), "Freq");
    m_freqPar->setKnobSkewFromMidPoint(1000.0);
    m_resonancePar = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextFilterPlugin::resonanceParamID), "Reso");
    m_modePar = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID(NextFilterPlugin::modeParamID), "Mode");
    m_slopePar = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID(NextFilterPlugin::slopeParamID), "Slope");

    m_graph = std::make_unique<FilterTransferGraphComponent>(*this, m_plugin->getAutomatableParameterByID(NextFilterPlugin::frequencyParamID), m_plugin->getAutomatableParameterByID(NextFilterPlugin::resonanceParamID), m_plugin->getAutomatableParameterByID(NextFilterPlugin::modeParamID), m_plugin->getAutomatableParameterByID(NextFilterPlugin::slopeParamID));

    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_freqPar);
    addAndMakeVisible(*m_resonancePar);
    addAndMakeVisible(*m_modePar);
    addAndMakeVisible(*m_slopePar);

    m_plugin->state.addListener(this);
}

FilterPluginComponent::~FilterPluginComponent() { m_plugin->state.removeListener(this); }

void FilterPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    auto top = area.removeFromTop((int)(area.getHeight() * 0.56f));
    m_graph->setBounds(top);

    area.removeFromTop(4);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    const int row1Col = row1.getWidth() / 2;
    m_freqPar->setBounds(row1.removeFromLeft(row1Col).reduced(2));
    m_resonancePar->setBounds(row1.reduced(2));

    const int row2Col = row2.getWidth() / 2;
    m_modePar->setBounds(row2.removeFromLeft(row2Col).reduced(2));
    m_slopePar->setBounds(row2.reduced(2));
}

void FilterPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void FilterPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    static const juce::Identifier freqId(NextFilterPlugin::frequencyParamID);
    static const juce::Identifier resId(NextFilterPlugin::resonanceParamID);
    static const juce::Identifier modeId(NextFilterPlugin::modeParamID);
    static const juce::Identifier slopeId(NextFilterPlugin::slopeParamID);

    if (i == freqId && m_freqPar)
        m_freqPar->updateLabel();
    else if (i == resId && m_resonancePar)
        m_resonancePar->updateLabel();

    if (i == freqId || i == resId || i == modeId || i == slopeId)
    {
        if (m_graph)
            m_graph->repaint();
    }
}

juce::ValueTree FilterPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree FilterPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", NextFilterPlugin::xmlTypeName, nullptr);
    defaultState.setProperty(NextFilterPlugin::frequencyParamID, 1400.0f, nullptr);
    defaultState.setProperty(NextFilterPlugin::resonanceParamID, 0.71f, nullptr);
    defaultState.setProperty(NextFilterPlugin::modeParamID, (float)NextFilterPlugin::lowpass, nullptr);
    defaultState.setProperty(NextFilterPlugin::slopeParamID, (float)NextFilterPlugin::slope24, nullptr);
    return defaultState;
}

void FilterPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String FilterPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String FilterPluginComponent::getPluginTypeName() const { return NextFilterPlugin::xmlTypeName; }

ApplicationViewState &FilterPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

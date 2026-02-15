#include "Plugins/Saturation/SaturationPluginComponent.h"

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

class SaturationPluginComponent::SaturationTransferGraphComponent : public juce::Component
{
public:
    SaturationTransferGraphComponent(SaturationPluginComponent &owner, te::AutomatableParameter::Ptr drive, te::AutomatableParameter::Ptr tone, te::AutomatableParameter::Ptr mix, te::AutomatableParameter::Ptr bias, te::AutomatableParameter::Ptr mode)
        : m_owner(owner),
          m_drive(std::move(drive)),
          m_tone(std::move(tone)),
          m_mix(std::move(mix)),
          m_bias(std::move(bias)),
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

        auto header = panel.removeFromTop(headerHeight).toNearestInt();
        g.setColour(trackColour.contrasting(0.9f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText("SATURATION", header.reduced(8, 0), juce::Justification::centredLeft, 1);

        const juce::String driveText = m_drive != nullptr ? m_drive->getCurrentValueAsString() : juce::String("12.0 dB");
        const juce::String modeText = m_mode != nullptr ? m_mode->getCurrentValueAsString() : juce::String("Soft");
        g.drawFittedText(modeText + "  |  Drive " + driveText, header.reduced(8, 0), juce::Justification::centredRight, 1);

        auto graph = panel.reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff1a212b));
        g.fillRoundedRectangle(graph, 6.0f);

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.07f));
        g.drawLine(graph.getX(), graph.getCentreY(), graph.getRight(), graph.getCentreY(), 1.0f);
        g.drawLine(graph.getCentreX(), graph.getY(), graph.getCentreX(), graph.getBottom(), 1.0f);

        const float driveDb = m_drive != nullptr ? juce::jlimit(0.0f, 36.0f, m_drive->getCurrentValue()) : 12.0f;
        const float driveGain = juce::Decibels::decibelsToGain(driveDb);
        const float mix = m_mix != nullptr ? juce::jlimit(0.0f, 1.0f, m_mix->getCurrentValue()) : 1.0f;
        const float tone = m_tone != nullptr ? juce::jlimit(0.0f, 1.0f, m_tone->getCurrentValue()) : 0.65f;
        const float bias = m_bias != nullptr ? juce::jlimit(-1.0f, 1.0f, m_bias->getCurrentValue()) : 0.0f;
        const int mode = m_mode != nullptr ? juce::jlimit(0, 2, (int)std::round(m_mode->getCurrentValue())) : 0;

        auto saturate = [mode](float x)
        {
            if (mode == NextSaturationPlugin::hardClip)
                return juce::jlimit(-1.0f, 1.0f, x);
            if (mode == NextSaturationPlugin::smooth)
                return (2.0f / juce::MathConstants<float>::pi) * std::atan(x);
            return std::tanh(x);
        };

        juce::Path curve;
        const int steps = 120;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float)i / (float)steps;
            const float xNorm = t * 2.0f - 1.0f;
            const float shaped = saturate(xNorm * driveGain + bias * 0.35f);
            const float yNorm = juce::jlimit(-1.0f, 1.0f, shaped);
            const float x = graph.getX() + t * graph.getWidth();
            const float y = graph.getCentreY() - yNorm * (graph.getHeight() * 0.42f);

            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        const float glow = juce::jmap(mix, 0.0f, 1.0f, 0.25f, 0.95f);
        g.setColour(trackColour.brighter(0.6f).withAlpha(glow * 0.35f));
        g.strokePath(curve, juce::PathStrokeType(3.0f));
        g.setColour(trackColour.brighter(0.25f).withAlpha(glow));
        g.strokePath(curve, juce::PathStrokeType(1.15f));

        const float markerY = graph.getY() + graph.getHeight() * (0.85f - tone * 0.7f);
        g.setColour(trackColour.withAlpha(0.62f));
        g.fillEllipse(graph.getRight() - 9.0f, markerY - 3.5f, 7.0f, 7.0f);
    }

private:
    SaturationPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_drive;
    te::AutomatableParameter::Ptr m_tone;
    te::AutomatableParameter::Ptr m_mix;
    te::AutomatableParameter::Ptr m_bias;
    te::AutomatableParameter::Ptr m_mode;
};

SaturationPluginComponent::SaturationPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_saturationPlugin = dynamic_cast<NextSaturationPlugin *>(m_plugin.get());

    m_graph = std::make_unique<SaturationTransferGraphComponent>(*this, m_plugin->getAutomatableParameterByID(NextSaturationPlugin::driveParamID), m_plugin->getAutomatableParameterByID(NextSaturationPlugin::toneParamID), m_plugin->getAutomatableParameterByID(NextSaturationPlugin::mixParamID), m_plugin->getAutomatableParameterByID(NextSaturationPlugin::biasParamID), m_plugin->getAutomatableParameterByID(NextSaturationPlugin::modeParamID));

    m_input = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::inputParamID), "Input");
    m_drive = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::driveParamID), "Drive");
    m_tone = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::toneParamID), "Tone");
    m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::mixParamID), "Mix");
    m_output = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::outputParamID), "Output");
    m_bias = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::biasParamID), "Bias");
    m_mode = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::modeParamID), "Mode");
    m_quality = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID(NextSaturationPlugin::qualityParamID), "Quality");

    m_inMeter = std::make_unique<LevelMeterComponent>([this]() -> te::LevelMeasurer * { return m_saturationPlugin != nullptr ? m_saturationPlugin->getInputLevelMeasurer() : nullptr; }, LevelMeterComponent::ChannelType::Stereo);

    m_outMeter = std::make_unique<LevelMeterComponent>([this]() -> te::LevelMeasurer * { return m_saturationPlugin != nullptr ? m_saturationPlugin->getOutputLevelMeasurer() : nullptr; }, LevelMeterComponent::ChannelType::Stereo);

    m_inLabel.setText("IN", juce::dontSendNotification);
    m_outLabel.setText("OUT", juce::dontSendNotification);
    m_inLabel.setJustificationType(juce::Justification::centred);
    m_outLabel.setJustificationType(juce::Justification::centred);
    m_inLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    m_outLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));

    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_input);
    addAndMakeVisible(*m_drive);
    addAndMakeVisible(*m_tone);
    addAndMakeVisible(*m_mix);
    addAndMakeVisible(*m_output);
    addAndMakeVisible(*m_bias);
    addAndMakeVisible(*m_mode);
    addAndMakeVisible(*m_quality);
    addAndMakeVisible(*m_inMeter);
    addAndMakeVisible(*m_outMeter);
    addAndMakeVisible(m_inLabel);
    addAndMakeVisible(m_outLabel);

    m_plugin->state.addListener(this);
}

SaturationPluginComponent::~SaturationPluginComponent() { m_plugin->state.removeListener(this); }

void SaturationPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void SaturationPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(4);

    auto leftMeterArea = area.removeFromLeft(19).reduced(1, 6);
    auto rightMeterArea = area.removeFromRight(19).reduced(1, 6);

    const int meterLabelH = 12;
    m_inLabel.setBounds(leftMeterArea.removeFromTop(meterLabelH));
    m_inMeter->setBounds(leftMeterArea);
    m_outLabel.setBounds(rightMeterArea.removeFromTop(meterLabelH));
    m_outMeter->setBounds(rightMeterArea);

    auto topArea = area.removeFromTop((int)(area.getHeight() * 0.47f));
    m_graph->setBounds(topArea.reduced(0, 2));

    area.removeFromTop(4);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    const int row1Col = row1.getWidth() / 5;
    m_input->setBounds(row1.removeFromLeft(row1Col).reduced(2));
    m_drive->setBounds(row1.removeFromLeft(row1Col).reduced(2));
    m_tone->setBounds(row1.removeFromLeft(row1Col).reduced(2));
    m_mix->setBounds(row1.removeFromLeft(row1Col).reduced(2));
    m_output->setBounds(row1.reduced(2));

    const int row2Col = row2.getWidth() / 3;
    m_bias->setBounds(row2.removeFromLeft(row2Col).reduced(2));
    m_mode->setBounds(row2.removeFromLeft(row2Col).reduced(2));
    m_quality->setBounds(row2.reduced(2));
}

juce::ValueTree SaturationPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree SaturationPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", NextSaturationPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void SaturationPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String SaturationPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String SaturationPluginComponent::getPluginTypeName() const { return NextSaturationPlugin::xmlTypeName; }

ApplicationViewState &SaturationPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void SaturationPluginComponent::valueTreeChanged() {}

void SaturationPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    static const juce::Identifier inputId(NextSaturationPlugin::inputParamID);
    static const juce::Identifier driveId(NextSaturationPlugin::driveParamID);
    static const juce::Identifier toneId(NextSaturationPlugin::toneParamID);
    static const juce::Identifier mixId(NextSaturationPlugin::mixParamID);
    static const juce::Identifier outputId(NextSaturationPlugin::outputParamID);
    static const juce::Identifier biasId(NextSaturationPlugin::biasParamID);

    if (i == inputId && m_input)
        m_input->updateLabel();
    else if (i == driveId && m_drive)
        m_drive->updateLabel();
    else if (i == toneId && m_tone)
        m_tone->updateLabel();
    else if (i == mixId && m_mix)
        m_mix->updateLabel();
    else if (i == outputId && m_output)
        m_output->updateLabel();
    else if (i == biasId && m_bias)
        m_bias->updateLabel();

    if (m_graph)
        m_graph->repaint();
}

void SaturationPluginComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) {}

void SaturationPluginComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) {}

void SaturationPluginComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) {}

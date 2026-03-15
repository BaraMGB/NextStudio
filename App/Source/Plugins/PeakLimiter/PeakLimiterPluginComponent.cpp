#include "Plugins/PeakLimiter/PeakLimiterPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

namespace
{
juce::String formatPeakValue(float db)
{
    if (db <= -99.0f)
        return "-inf dB";

    return juce::String(db, 1) + " dB";
}

} // namespace

class PeakLimiterPluginComponent::MeterComponent : public juce::Component
{
public:
    void setValues(float inputPeakDb, float outputPeakDb, float gainReductionDb)
    {
        if (juce::approximatelyEqual(m_inputPeakDb, inputPeakDb) && juce::approximatelyEqual(m_outputPeakDb, outputPeakDb) && juce::approximatelyEqual(m_gainReductionDb, gainReductionDb))
            return;

        m_inputPeakDb = inputPeakDb;
        m_outputPeakDb = outputPeakDb;
        m_gainReductionDb = gainReductionDb;
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        auto area = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(juce::Colour(0xff141920));
        g.fillRoundedRectangle(area, 8.0f);
        g.setColour(juce::Colour(0xff313847));
        g.drawRoundedRectangle(area, 8.0f, 1.0f);

        auto header = area.removeFromTop(22.0f);
        g.setColour(juce::Colour(0xffd5dde8));
        g.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
        g.drawFittedText("METER", header.toNearestInt(), juce::Justification::centred, 1);

        const auto meterArea = area.reduced(8.0f, 10.0f);
        if (meterArea.getWidth() <= 0.0f || meterArea.getHeight() <= 0.0f)
            return;

        auto drawMeter = [&](float x, float width, float valueDb, juce::Colour colour, bool invert = false)
        {
            auto lane = juce::Rectangle<float>(x, meterArea.getY(), width, meterArea.getHeight());
            g.setColour(juce::Colour(0xff202632));
            g.fillRoundedRectangle(lane, 4.0f);

            const float normalized = invert ? juce::jlimit(0.0f, 1.0f, valueDb / 24.0f) : juce::jlimit(0.0f, 1.0f, juce::jmap(valueDb, -60.0f, 0.0f, 0.0f, 1.0f));
            const float fillHeight = lane.getHeight() * normalized;
            auto fill = lane.withY(lane.getBottom() - fillHeight).withHeight(fillHeight).reduced(2.0f);

            if (fill.getHeight() > 0.0f)
            {
                g.setColour(colour);
                g.fillRoundedRectangle(fill, 3.0f);
            }
        };

        const float laneWidth = meterArea.getWidth() / 3.0f;
        drawMeter(meterArea.getX(), laneWidth - 6.0f, m_inputPeakDb, juce::Colour(0xff62c66d));
        drawMeter(meterArea.getX() + laneWidth, laneWidth - 6.0f, m_outputPeakDb, juce::Colour(0xff4bb9ff));
        drawMeter(meterArea.getX() + laneWidth * 2.0f, laneWidth - 6.0f, m_gainReductionDb, juce::Colour(0xffff9f43), true);

        g.setColour(juce::Colour(0xffaeb7c4));
        g.setFont(11.0f);
        auto labels = meterArea.withTrimmedTop(meterArea.getHeight() - 16.0f);
        g.drawFittedText("IN", juce::Rectangle<int>((int)meterArea.getX(), (int)labels.getY(), (int)laneWidth, 16), juce::Justification::centred, 1);
        g.drawFittedText("OUT", juce::Rectangle<int>((int)(meterArea.getX() + laneWidth), (int)labels.getY(), (int)laneWidth, 16), juce::Justification::centred, 1);
        g.drawFittedText("GR", juce::Rectangle<int>((int)(meterArea.getX() + laneWidth * 2.0f), (int)labels.getY(), (int)laneWidth, 16), juce::Justification::centred, 1);
    }

private:
    float m_inputPeakDb = -100.0f;
    float m_outputPeakDb = -100.0f;
    float m_gainReductionDb = 0.0f;
};

PeakLimiterPluginComponent::PeakLimiterPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_peakLimiter(dynamic_cast<PeakLimiterPlugin *>(p.get()))
{
    const auto inputParam = m_plugin->getAutomatableParameterByID(PeakLimiterPlugin::inputGainParamID);
    const auto ceilingParam = m_plugin->getAutomatableParameterByID(PeakLimiterPlugin::ceilingParamID);
    const auto releaseParam = m_plugin->getAutomatableParameterByID(PeakLimiterPlugin::releaseParamID);
    const auto linkParam = m_plugin->getAutomatableParameterByID(PeakLimiterPlugin::linkChannelsParamID);

    jassert(inputParam != nullptr);
    jassert(ceilingParam != nullptr);
    jassert(releaseParam != nullptr);
    jassert(linkParam != nullptr);

    if (inputParam == nullptr || ceilingParam == nullptr || releaseParam == nullptr || linkParam == nullptr)
        return;

    m_inputGainComp = std::make_unique<AutomatableParameterComponent>(inputParam, "Input");
    m_ceilingComp = std::make_unique<AutomatableParameterComponent>(ceilingParam, "Ceiling");
    m_releaseComp = std::make_unique<AutomatableParameterComponent>(releaseParam, "Release");
    m_linkChannelsComp = std::make_unique<AutomatableToggleComponent>(linkParam, "Stereo Link");
    m_meter = std::make_unique<MeterComponent>();

    addAndMakeVisible(*m_inputGainComp);
    addAndMakeVisible(*m_ceilingComp);
    addAndMakeVisible(*m_releaseComp);
    addAndMakeVisible(*m_linkChannelsComp);
    addAndMakeVisible(*m_meter);

    for (auto *label : {&m_inputMeterLabel, &m_outputMeterLabel, &m_gainReductionLabel})
    {
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffd5dde8));
        addAndMakeVisible(*label);
    }

    m_inputMeterLabel.setText("Input: -inf dB", juce::dontSendNotification);
    m_outputMeterLabel.setText("Output: -inf dB", juce::dontSendNotification);
    m_gainReductionLabel.setText("Reduction: 0.0 dB", juce::dontSendNotification);

    startTimerHz(30);
}

PeakLimiterPluginComponent::~PeakLimiterPluginComponent() { stopTimer(); }

void PeakLimiterPluginComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_editViewState.m_applicationState.getBackgroundColour2());

    auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    auto header = bounds.removeFromTop(30.0f);
    GUIHelpers::drawHeaderBox(g, header, getTrackColour(), m_editViewState.m_applicationState.getBorderColour(), m_editViewState.m_applicationState.getBackgroundColour1(), 18.0f, GUIHelpers::HeaderPosition::top, "PEAK LIMITER");
}

void PeakLimiterPluginComponent::resized()
{
    if (m_inputGainComp == nullptr || m_ceilingComp == nullptr || m_releaseComp == nullptr || m_linkChannelsComp == nullptr || m_meter == nullptr)
        return;

    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(34);

    auto meterColumn = area.removeFromRight(150).reduced(4);
    auto meterLabels = meterColumn.removeFromBottom(64);
    m_meter->setBounds(meterColumn);

    auto topRow = area.removeFromTop(area.getHeight() / 2);
    auto bottomRow = area;

    auto topWidth = topRow.getWidth() / 3;
    m_inputGainComp->setBounds(topRow.removeFromLeft(topWidth).reduced(2));
    m_ceilingComp->setBounds(topRow.removeFromLeft(topWidth).reduced(2));
    m_linkChannelsComp->setBounds(topRow.reduced(2));

    m_releaseComp->setBounds(bottomRow.reduced(2));

    m_inputMeterLabel.setBounds(meterLabels.removeFromTop(20));
    m_outputMeterLabel.setBounds(meterLabels.removeFromTop(20));
    m_gainReductionLabel.setBounds(meterLabels.removeFromTop(20));
}

void PeakLimiterPluginComponent::timerCallback()
{
    if (m_peakLimiter == nullptr || m_meter == nullptr)
        return;

    m_inputPeakDb = m_peakLimiter->getInputPeakDb();
    m_outputPeakDb = m_peakLimiter->getOutputPeakDb();
    m_gainReductionDb = m_peakLimiter->getGainReductionDb();

    const auto updateLabelText = [](juce::Label &label, const juce::String &text)
    {
        if (label.getText() != text)
            label.setText(text, juce::dontSendNotification);
    };

    updateLabelText(m_inputMeterLabel, "Input: " + formatPeakValue(m_inputPeakDb));
    updateLabelText(m_outputMeterLabel, "Output: " + formatPeakValue(m_outputPeakDb));
    updateLabelText(m_gainReductionLabel, "Reduction: " + juce::String(m_gainReductionDb, 1) + " dB");
    m_meter->setValues(m_inputPeakDb, m_outputPeakDb, m_gainReductionDb);
}

juce::ValueTree PeakLimiterPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree PeakLimiterPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", PeakLimiterPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void PeakLimiterPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String PeakLimiterPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String PeakLimiterPluginComponent::getPluginTypeName() const { return PeakLimiterPlugin::xmlTypeName; }

ApplicationViewState &PeakLimiterPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

/*
  ==============================================================================

    CompressorPluginComponent.cpp
    Created: 11 Feb 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Compressor/CompressorPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"

#include <array>

namespace
{
float gainToDb(float gain) { return juce::Decibels::gainToDecibels(gain, -100.0f); }

float mapDbToX(float db, const juce::Rectangle<float> &area, float minDb, float maxDb)
{
    auto norm = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    return juce::jmap(norm, 0.0f, 1.0f, area.getX(), area.getRight());
}

float mapDbToY(float db, const juce::Rectangle<float> &area, float minDb, float maxDb)
{
    auto norm = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    return juce::jmap(norm, 0.0f, 1.0f, area.getBottom(), area.getY());
}
} // namespace

CompressorTransferGraphComponent::CompressorTransferGraphComponent(te::AutomatableParameter::Ptr thresholdParam, te::AutomatableParameter::Ptr ratioParam, te::AutomatableParameter::Ptr outputParam)
    : m_thresholdParam(std::move(thresholdParam)),
      m_ratioParam(std::move(ratioParam)),
      m_outputParam(std::move(outputParam))
{
}

void CompressorTransferGraphComponent::paint(juce::Graphics &g)
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() < 20.0f || bounds.getHeight() < 20.0f)
        return;

    g.fillAll(juce::Colour(0xff101216));

    constexpr float leftMargin = 58.0f;
    constexpr float rightMargin = 10.0f;
    constexpr float topMargin = 18.0f;
    constexpr float bottomMargin = 34.0f;
    const auto area = bounds.withTrimmedLeft(leftMargin).withTrimmedRight(rightMargin).withTrimmedTop(topMargin).withTrimmedBottom(bottomMargin);
    g.setColour(juce::Colour(0xff1b2027));
    g.fillRect(area);

    constexpr float minDb = -48.0f;
    constexpr float maxDb = 12.0f;
    constexpr std::array<float, 6> gridDb = {-48.0f, -36.0f, -24.0f, -12.0f, 0.0f, 12.0f};

    g.setColour(juce::Colour(0xff2a3039));
    for (auto db : gridDb)
    {
        auto y = mapDbToY(db, area, minDb, maxDb);
        auto x = mapDbToX(db, area, minDb, maxDb);
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
        g.drawVerticalLine((int)x, area.getY(), area.getBottom());

        g.setColour(juce::Colour(0xff7f8a99));
        g.setFont(11.0f);
        g.drawFittedText(juce::String((int)db) + " dB", juce::Rectangle<int>((int)bounds.getX() + 20, (int)y - 7, (int)leftMargin - 24, 14), juce::Justification::centredLeft, 1);
        g.drawFittedText(juce::String((int)db), juce::Rectangle<int>((int)x - 13, (int)area.getBottom() + 2, 26, 12), juce::Justification::centred, 1);
        g.setColour(juce::Colour(0xff2a3039));
    }

    const float thresholdDb = m_thresholdParam != nullptr ? gainToDb(m_thresholdParam->getCurrentValue()) : -6.0f;
    const float ratioAmount = m_ratioParam != nullptr ? juce::jlimit(0.0f, 0.999f, m_ratioParam->getCurrentValue()) : 0.5f;
    const float effectiveRatio = 1.0f / juce::jmax(0.001f, 1.0f - ratioAmount);
    const float outputDb = m_outputParam != nullptr ? m_outputParam->getCurrentValue() : 0.0f;

    const float thresholdX = mapDbToX(thresholdDb, area, minDb, maxDb);
    g.setColour(juce::Colour(0xffc8a84b));
    g.drawVerticalLine((int)thresholdX, area.getY(), area.getBottom());

    juce::Path unityPath;
    unityPath.startNewSubPath(mapDbToX(minDb, area, minDb, maxDb), mapDbToY(minDb, area, minDb, maxDb));
    unityPath.lineTo(mapDbToX(maxDb, area, minDb, maxDb), mapDbToY(maxDb, area, minDb, maxDb));
    g.setColour(juce::Colour(0xffaab3c2).withAlpha(0.5f));
    g.strokePath(unityPath, juce::PathStrokeType(1.5f));

    juce::Path transferPath;
    constexpr int numPoints = 160;
    for (int i = 0; i < numPoints; ++i)
    {
        const auto inDb = juce::jmap((float)i, 0.0f, (float)(numPoints - 1), minDb, maxDb);
        float outDb = inDb;
        if (inDb > thresholdDb)
            outDb = thresholdDb + (inDb - thresholdDb) / effectiveRatio;

        outDb += outputDb;

        const auto x = mapDbToX(inDb, area, minDb, maxDb);
        const auto y = mapDbToY(outDb, area, minDb, maxDb);

        if (i == 0)
            transferPath.startNewSubPath(x, y);
        else
            transferPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff57c7ff));
    g.strokePath(transferPath, juce::PathStrokeType(2.2f));

    g.setColour(juce::Colour(0xffcad3df));
    g.setFont(12.0f);
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, bounds.getX() + 10.0f, area.getCentreY()));
    g.drawFittedText("Output", juce::Rectangle<int>((int)bounds.getX() - 28, (int)area.getCentreY() - 8, (int)area.getHeight(), 16), juce::Justification::centred, 1);
    g.restoreState();
    g.drawFittedText("Input (dB)", juce::Rectangle<int>((int)area.getX(), (int)bounds.getBottom() - 16, (int)area.getWidth(), 14), juce::Justification::centred, 1);

    g.setColour(juce::Colour(0xff3a424f));
    g.drawRect(area, 1.0f);
}

CompressorPluginComponent::CompressorPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_thresholdParam(m_plugin->getAutomatableParameterByID("threshold")),
      m_ratioParam(m_plugin->getAutomatableParameterByID("ratio")),
      m_attackParam(m_plugin->getAutomatableParameterByID("attack")),
      m_releaseParam(m_plugin->getAutomatableParameterByID("release")),
      m_outputParam(m_plugin->getAutomatableParameterByID("output gain")),
      m_transferGraph(m_thresholdParam, m_ratioParam, m_outputParam)
{
    m_thresholdComp = std::make_unique<AutomatableParameterComponent>(m_thresholdParam, "Thresh");
    m_ratioComp = std::make_unique<AutomatableParameterComponent>(m_ratioParam, "Ratio");
    m_attackComp = std::make_unique<AutomatableParameterComponent>(m_attackParam, "Attack");
    m_releaseComp = std::make_unique<AutomatableParameterComponent>(m_releaseParam, "Release");
    m_outputComp = std::make_unique<AutomatableParameterComponent>(m_outputParam, "Output");

    addAndMakeVisible(m_transferGraph);
    addAndMakeVisible(*m_thresholdComp);
    addAndMakeVisible(*m_ratioComp);
    addAndMakeVisible(*m_attackComp);
    addAndMakeVisible(*m_releaseComp);
    addAndMakeVisible(*m_outputComp);
    m_plugin->state.addListener(this);
}

CompressorPluginComponent::~CompressorPluginComponent() { m_plugin->state.removeListener(this); }

void CompressorPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void CompressorPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(6);

    auto graphArea = area.removeFromTop((int)(area.getHeight() * 0.48f));
    m_transferGraph.setBounds(graphArea);

    area.removeFromTop(4);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto row2 = area;

    auto colW1 = row1.getWidth() / 4;
    m_thresholdComp->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_ratioComp->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_attackComp->setBounds(row1.removeFromLeft(colW1).reduced(2));
    m_releaseComp->setBounds(row1.reduced(2));

    auto outputArea = row2.withSizeKeepingCentre(row2.getWidth() / 2, row2.getHeight());
    m_outputComp->setBounds(outputArea.reduced(2));
}

void CompressorPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    if (i == te::IDs::threshold)
        m_thresholdComp->updateLabel();
    else if (i == te::IDs::ratio)
        m_ratioComp->updateLabel();
    else if (i == te::IDs::attack)
        m_attackComp->updateLabel();
    else if (i == te::IDs::release)
        m_releaseComp->updateLabel();
    else if (i == te::IDs::outputDb)
        m_outputComp->updateLabel();

    m_transferGraph.repaint();
}

juce::ValueTree CompressorPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree CompressorPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", te::CompressorPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void CompressorPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String CompressorPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String CompressorPluginComponent::getPluginTypeName() const { return te::CompressorPlugin::xmlTypeName; }

ApplicationViewState &CompressorPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

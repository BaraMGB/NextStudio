/*
  ==============================================================================

    EqPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/EQ/EqPluginComponent.h"

#include "LowerRange/PluginChain/PresetHelpers.h"

#include <cmath>
#include <limits>
#include <vector>

namespace
{
constexpr float minGraphFreq = 20.0f;
constexpr float maxGraphFreq = 20000.0f;
constexpr float minGraphDb = -24.0f;
constexpr float maxGraphDb = 24.0f;
constexpr double previewSampleRate = 44100.0;

inline float gainFromDb(float db) { return std::pow(10.0f, db / 20.0f); }

inline double getBiquadMagnitude(const juce::IIRCoefficients &coeff, double frequency, double sampleRate)
{
    const double w = juce::MathConstants<double>::twoPi * frequency / sampleRate;
    const double cosW = std::cos(w);
    const double sinW = std::sin(w);
    const double cos2W = std::cos(2.0 * w);
    const double sin2W = std::sin(2.0 * w);

    const double b0 = coeff.coefficients[0];
    const double b1 = coeff.coefficients[1];
    const double b2 = coeff.coefficients[2];
    const double a1 = coeff.coefficients[3];
    const double a2 = coeff.coefficients[4];

    const double numRe = b0 + b1 * cosW + b2 * cos2W;
    const double numIm = -b1 * sinW - b2 * sin2W;
    const double denRe = 1.0 + a1 * cosW + a2 * cos2W;
    const double denIm = -a1 * sinW - a2 * sin2W;

    const double numMag = std::sqrt(numRe * numRe + numIm * numIm);
    const double denMag = std::sqrt(denRe * denRe + denIm * denIm);
    return numMag / juce::jmax(1.0e-12, denMag);
}
} // namespace

EqResponseGraphComponent::EqResponseGraphComponent(te::Plugin::Ptr plugin, te::AutomatableParameter::Ptr lowFreq, te::AutomatableParameter::Ptr lowGain, te::AutomatableParameter::Ptr lowQ, te::AutomatableParameter::Ptr midFreq1, te::AutomatableParameter::Ptr midGain1, te::AutomatableParameter::Ptr midQ1, te::AutomatableParameter::Ptr midFreq2, te::AutomatableParameter::Ptr midGain2, te::AutomatableParameter::Ptr midQ2, te::AutomatableParameter::Ptr highFreq, te::AutomatableParameter::Ptr highGain, te::AutomatableParameter::Ptr highQ)
    : m_plugin(plugin)
{
    m_bands[0] = BandHandle{lowFreq, lowGain, lowQ};
    m_bands[1] = BandHandle{midFreq1, midGain1, midQ1};
    m_bands[2] = BandHandle{midFreq2, midGain2, midQ2};
    m_bands[3] = BandHandle{highFreq, highGain, highQ};
}

void EqResponseGraphComponent::paint(juce::Graphics &g)
{
    const auto frameArea = getLocalBounds().toFloat().reduced(8.0f);
    const auto plotArea = getPlotArea();
    auto yLabelArea = frameArea;
    yLabelArea.setWidth(plotArea.getX() - frameArea.getX());
    auto xLabelArea = frameArea;
    xLabelArea.setY(plotArea.getBottom());
    xLabelArea.setX(plotArea.getX());
    xLabelArea.setHeight(frameArea.getBottom() - plotArea.getBottom());

    auto curveColour = juce::Colour(0xfff28b1e);
    if (m_plugin != nullptr)
        if (auto *ownerTrack = m_plugin->getOwnerTrack())
            curveColour = ownerTrack->getColour();

    g.setColour(juce::Colour(0xff1f2229));
    g.fillRoundedRectangle(frameArea, 7.0f);

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillRoundedRectangle(plotArea, 6.0f);

    g.setFont(juce::FontOptions(10.0f));
    constexpr std::array<float, 5> dbLabels = {24.0f, 12.0f, 0.0f, -12.0f, -24.0f};
    for (auto db : dbLabels)
    {
        const auto y = yForGainDb(plotArea, db);
        const bool major = (db == 0.0f || db == 24.0f || db == -24.0f);
        const auto alpha = major ? 0.17f : 0.10f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawHorizontalLine((int)y, plotArea.getX(), plotArea.getRight());

        auto row = juce::Rectangle<float>(yLabelArea.getX(), y - 7.0f, yLabelArea.getWidth() - 4.0f, 14.0f);
        g.setColour(juce::Colours::white.withAlpha(0.58f));
        g.drawText(juce::String((int)db), row.toNearestInt(), juce::Justification::centredRight, false);
    }

    constexpr std::array<float, 10> freqLabels = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    const auto formatFrequency = [](float hz)
    {
        if (hz >= 1000.0f)
        {
            if (hz >= 10000.0f)
                return juce::String((int)std::round(hz / 1000.0f)) + "k";
            return juce::String(hz / 1000.0f, 1) + "k";
        }

        return juce::String((int)std::round(hz));
    };

    for (auto frequency : freqLabels)
    {
        const auto x = xForFrequency(plotArea, frequency);
        const bool major = (frequency == 20.0f || frequency == 100.0f || frequency == 1000.0f || frequency == 10000.0f);
        g.setColour(juce::Colours::white.withAlpha(major ? 0.16f : 0.10f));
        g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());

        g.setColour(juce::Colours::white.withAlpha(0.60f));
        auto labelRect = juce::Rectangle<float>(x - 18.0f, xLabelArea.getY(), 36.0f, xLabelArea.getHeight());
        g.drawText(formatFrequency(frequency), labelRect.toNearestInt(), juce::Justification::centred, false);
    }

    juce::Path responsePath;
    if (m_bands[0].freq != nullptr && m_bands[0].gain != nullptr && m_bands[0].q != nullptr && m_bands[1].freq != nullptr && m_bands[1].gain != nullptr && m_bands[1].q != nullptr && m_bands[2].freq != nullptr && m_bands[2].gain != nullptr && m_bands[2].q != nullptr && m_bands[3].freq != nullptr && m_bands[3].gain != nullptr && m_bands[3].q != nullptr)
    {
        const auto lowCoeff = juce::IIRCoefficients::makeLowShelf(previewSampleRate, m_bands[0].freq->getCurrentValue(), m_bands[0].q->getCurrentValue(), gainFromDb(m_bands[0].gain->getCurrentValue()));
        const auto mid1Coeff = juce::IIRCoefficients::makePeakFilter(previewSampleRate, m_bands[1].freq->getCurrentValue(), m_bands[1].q->getCurrentValue(), gainFromDb(m_bands[1].gain->getCurrentValue()));
        const auto mid2Coeff = juce::IIRCoefficients::makePeakFilter(previewSampleRate, m_bands[2].freq->getCurrentValue(), m_bands[2].q->getCurrentValue(), gainFromDb(m_bands[2].gain->getCurrentValue()));
        const auto highCoeff = juce::IIRCoefficients::makeHighShelf(previewSampleRate, m_bands[3].freq->getCurrentValue(), m_bands[3].q->getCurrentValue(), gainFromDb(m_bands[3].gain->getCurrentValue()));

        const auto points = juce::jmax(160, (int)plotArea.getWidth() * 2);
        std::vector<float> sampledY((size_t)points, plotArea.getBottom());

        for (int i = 0; i < points; ++i)
        {
            const auto alpha = (float)i / (float)(points - 1);
            const auto frequency = juce::jmax(minGraphFreq, minGraphFreq * std::pow(maxGraphFreq / minGraphFreq, alpha));

            const auto mag = getBiquadMagnitude(lowCoeff, frequency, previewSampleRate) * getBiquadMagnitude(mid1Coeff, frequency, previewSampleRate) * getBiquadMagnitude(mid2Coeff, frequency, previewSampleRate) * getBiquadMagnitude(highCoeff, frequency, previewSampleRate);
            const auto gainDb = juce::Decibels::gainToDecibels((float)mag, minGraphDb);
            sampledY[(size_t)i] = yForGainDb(plotArea, gainDb);
        }

        for (int pass = 0; pass < 2; ++pass)
            for (int i = 1; i < points - 1; ++i)
                sampledY[(size_t)i] = sampledY[(size_t)i - 1] * 0.20f + sampledY[(size_t)i] * 0.60f + sampledY[(size_t)i + 1] * 0.20f;

        for (int i = 0; i < points; ++i)
        {
            const float x = plotArea.getX() + ((float)i / (float)(points - 1)) * plotArea.getWidth();
            const float y = sampledY[(size_t)i];

            if (i == 0)
                responsePath.startNewSubPath(x, y);
            else
            {
                const float prevX = plotArea.getX() + ((float)(i - 1) / (float)(points - 1)) * plotArea.getWidth();
                const float prevY = sampledY[(size_t)i - 1];
                const float midX = 0.5f * (prevX + x);
                const float midY = 0.5f * (prevY + y);
                responsePath.quadraticTo(prevX, prevY, midX, midY);

                if (i == points - 1)
                    responsePath.lineTo(x, y);
            }
        }
    }

    juce::Path fill(responsePath);
    fill.lineTo(plotArea.getRight(), plotArea.getBottom());
    fill.lineTo(plotArea.getX(), plotArea.getBottom());
    fill.closeSubPath();

    g.setColour(curveColour.withAlpha(0.20f));
    g.fillPath(fill);

    g.setColour(curveColour.brighter(0.30f).withAlpha(0.20f));
    g.strokePath(responsePath, juce::PathStrokeType(4.0f));

    g.setColour(curveColour.brighter(0.08f));
    g.strokePath(responsePath, juce::PathStrokeType(2.0f));

    for (int i = 0; i < (int)m_bands.size(); ++i)
    {
        const auto &band = m_bands[(size_t)i];
        if (band.freq == nullptr || band.gain == nullptr)
            continue;

        const auto x = xForFrequency(plotArea, band.freq->getCurrentValue());
        const auto y = yForGainDb(plotArea, band.gain->getCurrentValue());
        const auto isActive = (m_dragBandIndex == i || m_hoverBandIndex == i);
        const auto radius = isActive ? 7.0f : 6.0f;

        g.setColour(juce::Colour(0xff1f2229));
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(curveColour);
        g.drawEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        g.setColour(curveColour);
        g.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
        g.drawText(juce::String(i + 1), juce::Rectangle<float>(x - 8.0f, y - 22.0f, 16.0f, 14.0f).toNearestInt(), juce::Justification::centred, false);
    }

    g.setColour(juce::Colour(0x33ffffff));
    g.drawRoundedRectangle(frameArea, 7.0f, 1.0f);
}

void EqResponseGraphComponent::mouseDown(const juce::MouseEvent &e)
{
    m_dragBandIndex = getBandIndexAtPosition(e.position);
    m_hoverBandIndex = m_dragBandIndex;
    repaint();
}

void EqResponseGraphComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (m_dragBandIndex < 0 || m_dragBandIndex >= (int)m_bands.size())
        return;

    const auto area = getPlotArea();
    auto &band = m_bands[(size_t)m_dragBandIndex];
    if (band.freq == nullptr || band.gain == nullptr)
        return;

    const auto frequency = frequencyForX(area, e.position.x);
    const auto gainDb = gainDbForY(area, e.position.y);

    band.freq->setParameter(frequency, juce::sendNotification);
    band.gain->setParameter(gainDb, juce::sendNotification);
}

void EqResponseGraphComponent::mouseUp(const juce::MouseEvent &) { m_dragBandIndex = -1; }

void EqResponseGraphComponent::mouseMove(const juce::MouseEvent &e)
{
    const auto newHover = getBandIndexAtPosition(e.position);

    if (newHover != m_hoverBandIndex)
    {
        m_hoverBandIndex = newHover;
        repaint();
    }
}

void EqResponseGraphComponent::mouseExit(const juce::MouseEvent &)
{
    if (m_hoverBandIndex != -1)
    {
        m_hoverBandIndex = -1;
        repaint();
    }
}

void EqResponseGraphComponent::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
    auto bandIndex = m_hoverBandIndex;

    if (bandIndex < 0)
        bandIndex = getBandIndexAtPosition(e.position);

    if (bandIndex < 0 || bandIndex >= (int)m_bands.size())
        return;

    auto &band = m_bands[(size_t)bandIndex];
    if (band.q == nullptr)
        return;

    const float currentQ = band.q->getCurrentValue();
    const float wheelDelta = wheel.deltaY;
    const float qDelta = -wheelDelta * 0.75f;

    const auto qRange = band.q->valueRange;
    const float newQ = juce::jlimit(qRange.start, qRange.end, currentQ + qDelta);

    band.q->setParameter(newQ, juce::sendNotification);
    m_hoverBandIndex = bandIndex;
    repaint();
}

float EqResponseGraphComponent::xForFrequency(const juce::Rectangle<float> &area, float frequency) const
{
    const auto minFreq = std::log10(minGraphFreq);
    const auto maxFreq = std::log10(maxGraphFreq);
    const auto normalized = (std::log10(juce::jlimit(minGraphFreq, maxGraphFreq, frequency)) - minFreq) / (maxFreq - minFreq);
    return area.getX() + normalized * area.getWidth();
}

float EqResponseGraphComponent::yForGainDb(const juce::Rectangle<float> &area, float db) const
{
    const auto clamped = juce::jlimit(minGraphDb, maxGraphDb, db);
    const auto normalized = (clamped - minGraphDb) / (maxGraphDb - minGraphDb);
    return area.getBottom() - normalized * area.getHeight();
}

float EqResponseGraphComponent::frequencyForX(const juce::Rectangle<float> &area, float x) const
{
    const auto norm = juce::jlimit(0.0f, 1.0f, (x - area.getX()) / juce::jmax(1.0f, area.getWidth()));
    return minGraphFreq * std::pow(maxGraphFreq / minGraphFreq, norm);
}

float EqResponseGraphComponent::gainDbForY(const juce::Rectangle<float> &area, float y) const
{
    const auto norm = juce::jlimit(0.0f, 1.0f, (area.getBottom() - y) / juce::jmax(1.0f, area.getHeight()));
    return juce::jlimit(-20.0f, 20.0f, minGraphDb + norm * (maxGraphDb - minGraphDb));
}

int EqResponseGraphComponent::getBandIndexAtPosition(juce::Point<float> point) const
{
    const auto area = getPlotArea();
    float bestDistance = std::numeric_limits<float>::max();
    int bestIndex = -1;

    for (int i = 0; i < (int)m_bands.size(); ++i)
    {
        const auto &band = m_bands[(size_t)i];
        if (band.freq == nullptr || band.gain == nullptr)
            continue;

        const auto x = xForFrequency(area, band.freq->getCurrentValue());
        const auto y = yForGainDb(area, band.gain->getCurrentValue());
        const auto distance = point.getDistanceFrom({x, y});

        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestDistance <= 22.0f ? bestIndex : -1;
}

juce::Rectangle<float> EqResponseGraphComponent::getPlotArea() const
{
    auto frameArea = getLocalBounds().toFloat().reduced(8.0f);
    frameArea.removeFromLeft(34.0f);
    frameArea.removeFromBottom(18.0f);
    return frameArea;
}

EqPluginComponent::EqPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_lowFreqParam(m_plugin->getAutomatableParameterByID("Low-pass freq")),
      m_lowGainParam(m_plugin->getAutomatableParameterByID("Low-pass gain")),
      m_lowQParam(m_plugin->getAutomatableParameterByID("Low-pass Q")),
      m_midFreq1Param(m_plugin->getAutomatableParameterByID("Mid freq 1")),
      m_midGain1Param(m_plugin->getAutomatableParameterByID("Mid gain 1")),
      m_midQ1Param(m_plugin->getAutomatableParameterByID("Mid Q 1")),
      m_midFreq2Param(m_plugin->getAutomatableParameterByID("Mid freq 2")),
      m_midGain2Param(m_plugin->getAutomatableParameterByID("Mid gain 2")),
      m_midQ2Param(m_plugin->getAutomatableParameterByID("Mid Q 2")),
      m_hiFreqParam(m_plugin->getAutomatableParameterByID("High-pass freq")),
      m_hiGainParam(m_plugin->getAutomatableParameterByID("High-pass gain")),
      m_hiQParam(m_plugin->getAutomatableParameterByID("High-pass Q")),
      m_responseGraph(m_plugin, m_lowFreqParam, m_lowGainParam, m_lowQParam, m_midFreq1Param, m_midGain1Param, m_midQ1Param, m_midFreq2Param, m_midGain2Param, m_midQ2Param, m_hiFreqParam, m_hiGainParam, m_hiQParam)
{
    m_lowFreqComp = std::make_unique<AutomatableParameterComponent>(m_lowFreqParam, "L Freq");
    m_lowGainComp = std::make_unique<AutomatableParameterComponent>(m_lowGainParam, "L Gain");
    m_lowQComp = std::make_unique<AutomatableParameterComponent>(m_lowQParam, "L Q");

    m_midFreq1Comp = std::make_unique<AutomatableParameterComponent>(m_midFreq1Param, "M1 Freq");
    m_midGain1Comp = std::make_unique<AutomatableParameterComponent>(m_midGain1Param, "M1 Gain");
    m_midQ1Comp = std::make_unique<AutomatableParameterComponent>(m_midQ1Param, "M1 Q");

    m_midFreq2Comp = std::make_unique<AutomatableParameterComponent>(m_midFreq2Param, "M2 Freq");
    m_midGain2Comp = std::make_unique<AutomatableParameterComponent>(m_midGain2Param, "M2 Gain");
    m_midQ2Comp = std::make_unique<AutomatableParameterComponent>(m_midQ2Param, "M2 Q");

    m_hiFreqComp = std::make_unique<AutomatableParameterComponent>(m_hiFreqParam, "H Freq");
    m_hiGainComp = std::make_unique<AutomatableParameterComponent>(m_hiGainParam, "H Gain");
    m_hiQComp = std::make_unique<AutomatableParameterComponent>(m_hiQParam, "H Q");

    addAndMakeVisible(m_responseGraph);
    addAndMakeVisible(*m_lowFreqComp);
    addAndMakeVisible(*m_lowGainComp);
    addAndMakeVisible(*m_lowQComp);
    addAndMakeVisible(*m_midFreq1Comp);
    addAndMakeVisible(*m_midGain1Comp);
    addAndMakeVisible(*m_midQ1Comp);
    addAndMakeVisible(*m_midFreq2Comp);
    addAndMakeVisible(*m_midGain2Comp);
    addAndMakeVisible(*m_midQ2Comp);
    addAndMakeVisible(*m_hiFreqComp);
    addAndMakeVisible(*m_hiGainComp);
    addAndMakeVisible(*m_hiQComp);

    m_plugin->state.addListener(this);
}

void EqPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(6);

    auto graphArea = area.removeFromTop((int)(area.getHeight() * 0.52f));
    m_responseGraph.setBounds(graphArea);

    area.removeFromTop(6);

    auto bandArea = area;
    auto lowCol = bandArea.removeFromLeft(bandArea.getWidth() / 4).reduced(2);
    auto mid1Col = bandArea.removeFromLeft(bandArea.getWidth() / 3).reduced(2);
    auto mid2Col = bandArea.removeFromLeft(bandArea.getWidth() / 2).reduced(2);
    auto highCol = bandArea.reduced(2);

    const auto layoutColumn = [](juce::Rectangle<int> col, AutomatableParameterComponent *freq, AutomatableParameterComponent *gain, AutomatableParameterComponent *q)
    {
        auto third = col.getHeight() / 3;
        freq->setBounds(col.removeFromTop(third).reduced(1));
        gain->setBounds(col.removeFromTop(third).reduced(1));
        q->setBounds(col.reduced(1));
    };

    layoutColumn(lowCol, m_lowFreqComp.get(), m_lowGainComp.get(), m_lowQComp.get());
    layoutColumn(mid1Col, m_midFreq1Comp.get(), m_midGain1Comp.get(), m_midQ1Comp.get());
    layoutColumn(mid2Col, m_midFreq2Comp.get(), m_midGain2Comp.get(), m_midQ2Comp.get());
    layoutColumn(highCol, m_hiFreqComp.get(), m_hiGainComp.get(), m_hiQComp.get());
}

void EqPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    if (i == te::IDs::loFreq)
        m_lowFreqComp->updateLabel();
    if (i == te::IDs::loGain)
        m_lowGainComp->updateLabel();
    if (i == te::IDs::loQ)
        m_lowQComp->updateLabel();

    if (i == te::IDs::midFreq1)
        m_midFreq1Comp->updateLabel();
    if (i == te::IDs::midGain1)
        m_midGain1Comp->updateLabel();
    if (i == te::IDs::midQ1)
        m_midQ1Comp->updateLabel();

    if (i == te::IDs::midFreq2)
        m_midFreq2Comp->updateLabel();
    if (i == te::IDs::midGain2)
        m_midGain2Comp->updateLabel();
    if (i == te::IDs::midQ2)
        m_midQ2Comp->updateLabel();

    if (i == te::IDs::hiQ)
        m_hiQComp->updateLabel();
    if (i == te::IDs::hiFreq)
        m_hiFreqComp->updateLabel();
    if (i == te::IDs::hiGain)
        m_hiGainComp->updateLabel();

    m_responseGraph.repaint();
}

juce::ValueTree EqPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree EqPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "4bandEq", nullptr);
    return defaultState;
}

void EqPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String EqPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String EqPluginComponent::getPluginTypeName() const { return "4bandEq"; }

ApplicationViewState &EqPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

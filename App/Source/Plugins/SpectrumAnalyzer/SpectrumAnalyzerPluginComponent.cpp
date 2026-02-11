#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPluginComponent.h"

#include <algorithm>
#include <cmath>
#include <vector>

SpectrumAnalyzerPluginComponent::SpectrumAnalyzerPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_titleLabel.setText("SPECTRUM", juce::dontSendNotification);
    m_titleLabel.setJustificationType(juce::Justification::centred);
    m_titleLabel.setFont(juce::FontOptions(12, juce::Font::bold));
    m_titleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_titleLabel);

    if (auto *analyzer = getAnalyzer())
        analyzer->copySpectrum(m_spectrum);

    updateTimerState();
}

SpectrumAnalyzerPluginComponent::~SpectrumAnalyzerPluginComponent()
{
    stopTimer();
    m_timerRunning = false;
}

void SpectrumAnalyzerPluginComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds().reduced(5);
    const float cornerSize = 10.0f;

    g.setColour(m_editViewState.m_applicationState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, true, true, true);

    auto trackColour = getTrackColour();
    auto header = area.removeFromTop(20);
    g.setColour(trackColour);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, true, false, false);

    const auto labelColour = trackColour.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);
    m_titleLabel.setColour(juce::Label::textColourId, labelColour);

    auto graphFrame = area.reduced(8);
    auto yLabelArea = graphFrame.removeFromLeft(38);
    auto xLabelArea = graphFrame.removeFromBottom(18);
    auto graphArea = graphFrame.toFloat();

    if (graphArea.getWidth() <= 1.0f || graphArea.getHeight() <= 1.0f)
        return;

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillRoundedRectangle(graphArea, 6.0f);

    auto *analyzer = getAnalyzer();
    const double sr = analyzer != nullptr ? analyzer->getCurrentSampleRate() : 44100.0;

    g.setFont(juce::FontOptions(10.0f));

    const auto drawDbLabel = [&](float db)
    {
        const float y = yForDb(graphArea, db);
        auto row = juce::Rectangle<float>((float)yLabelArea.getX(), y - 7.0f, (float)yLabelArea.getWidth() - 4.0f, 14.0f);
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawText(juce::String((int)db), row.toNearestInt(), juce::Justification::centredRight, false);
    };

    drawDbLabel(0.0f);
    for (float db = -24.0f; db >= SpectrumAnalyzerPlugin::minDb; db -= 24.0f)
        drawDbLabel(db);

    for (float db = -12.0f; db >= SpectrumAnalyzerPlugin::minDb; db -= 12.0f)
    {
        const float y = yForDb(graphArea, db);
        const bool major = std::fmod(-db, 24.0f) < 0.1f;
        g.setColour(juce::Colours::white.withAlpha(major ? 0.17f : 0.09f));
        g.drawHorizontalLine((int)y, graphArea.getX(), graphArea.getRight());
    }

    const std::array<double, 10> markers{20.0, 40.0, 100.0, 250.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 16000.0};
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    for (double frequency : markers)
    {
        if (frequency > sr * 0.5)
            continue;

        const float x = xForFrequency(graphArea, frequency, sr);
        g.drawVerticalLine((int)x, graphArea.getY(), graphArea.getBottom());
    }

    const auto formatFrequency = [](double hz)
    {
        if (hz >= 1000.0)
        {
            const double khz = hz / 1000.0;
            if (khz >= 10.0)
                return juce::String((int)std::round(khz)) + "k";
            return juce::String(khz, 1) + "k";
        }

        return juce::String((int)std::round(hz));
    };

    g.setColour(juce::Colours::white.withAlpha(0.60f));
    for (double frequency : markers)
    {
        if (frequency > sr * 0.5)
            continue;

        const float x = xForFrequency(graphArea, frequency, sr);
        auto labelRect = juce::Rectangle<float>(x - 18.0f, (float)xLabelArea.getY(), 36.0f, (float)xLabelArea.getHeight());
        g.drawText(formatFrequency(frequency), labelRect.toNearestInt(), juce::Justification::centred, false);
    }

    const int pointCount = juce::jmax(160, (int)graphArea.getWidth() * 2);
    if (pointCount != m_pointCount || (int)m_sampledY.size() != pointCount)
    {
        m_pointCount = pointCount;
        m_sampledY.assign((size_t)pointCount, graphArea.getBottom());
    }

    for (int i = 0; i < pointCount; ++i)
    {
        const float binPos = ((float)i / (float)(pointCount - 1)) * (float)(SpectrumAnalyzerPlugin::numDisplayBins - 1);
        const int bin0 = juce::jlimit(0, SpectrumAnalyzerPlugin::numDisplayBins - 1, (int)std::floor(binPos));
        const int bin1 = juce::jlimit(0, SpectrumAnalyzerPlugin::numDisplayBins - 1, bin0 + 1);
        const float frac = binPos - (float)bin0;

        const float db = juce::jmap(frac, m_spectrum[(size_t)bin0], m_spectrum[(size_t)bin1]);
        m_sampledY[(size_t)i] = yForDb(graphArea, db);
    }

    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 1; i < pointCount - 1; ++i)
            m_sampledY[(size_t)i] = m_sampledY[(size_t)i - 1] * 0.20f + m_sampledY[(size_t)i] * 0.60f + m_sampledY[(size_t)i + 1] * 0.20f;
    }

    juce::Path path;
    for (int i = 0; i < pointCount; ++i)
    {
        const float x = graphArea.getX() + ((float)i / (float)(pointCount - 1)) * graphArea.getWidth();
        const float y = m_sampledY[(size_t)i];

        if (i == 0)
            path.startNewSubPath(x, y);
        else
        {
            const float prevX = graphArea.getX() + ((float)(i - 1) / (float)(pointCount - 1)) * graphArea.getWidth();
            const float prevY = m_sampledY[(size_t)i - 1];
            const float midX = 0.5f * (prevX + x);
            const float midY = 0.5f * (prevY + y);
            path.quadraticTo(prevX, prevY, midX, midY);

            if (i == pointCount - 1)
                path.lineTo(x, y);
        }
    }

    juce::Path fill(path);
    fill.lineTo(graphArea.getRight(), graphArea.getBottom());
    fill.lineTo(graphArea.getX(), graphArea.getBottom());
    fill.closeSubPath();

    g.setColour(trackColour.withAlpha(0.22f));
    g.fillPath(fill);

    g.setColour(trackColour.brighter(0.3f).withAlpha(0.20f));
    g.strokePath(path, juce::PathStrokeType(4.0f));

    g.setColour(trackColour.brighter(0.1f));
    g.strokePath(path, juce::PathStrokeType(2.0f));

    g.setColour(m_editViewState.m_applicationState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, getLocalBounds().reduced(5).toFloat(), cornerSize, true, true, true, true);
}

void SpectrumAnalyzerPluginComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    m_titleLabel.setBounds(area.removeFromTop(20));
    updateTimerState();
}

void SpectrumAnalyzerPluginComponent::visibilityChanged() { updateTimerState(); }

void SpectrumAnalyzerPluginComponent::parentHierarchyChanged() { updateTimerState(); }

void SpectrumAnalyzerPluginComponent::updateTimerState()
{
    const bool shouldRun = getAnalyzer() != nullptr && isShowing() && getWidth() > 48 && getHeight() > 48;

    if (shouldRun == m_timerRunning)
        return;

    if (shouldRun)
        startTimerHz(30);
    else
        stopTimer();

    m_timerRunning = shouldRun;
}

void SpectrumAnalyzerPluginComponent::timerCallback()
{
    if (!m_timerRunning)
        return;

    if (m_plugin != nullptr && !m_plugin->isEnabled())
    {
        bool needsClear = false;
        for (float db : m_spectrum)
        {
            if (db > SpectrumAnalyzerPlugin::minDb + 0.01f)
            {
                needsClear = true;
                break;
            }
        }

        if (needsClear)
        {
            m_spectrum.fill(SpectrumAnalyzerPlugin::minDb);
            repaint();
        }

        return;
    }

    if (auto *analyzer = getAnalyzer())
        analyzer->copySpectrum(m_spectrum);

    repaint();
}

juce::ValueTree SpectrumAnalyzerPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree SpectrumAnalyzerPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", SpectrumAnalyzerPlugin::xmlTypeName, nullptr);
    return defaultState;
}

void SpectrumAnalyzerPluginComponent::restorePluginState(const juce::ValueTree &state)
{
    if (auto *analyzer = getAnalyzer())
        analyzer->restorePluginStateFromValueTree(state);
}

juce::String SpectrumAnalyzerPluginComponent::getPresetSubfolder() const { return "SpectrumAnalyzer"; }

juce::String SpectrumAnalyzerPluginComponent::getPluginTypeName() const { return SpectrumAnalyzerPlugin::xmlTypeName; }

ApplicationViewState &SpectrumAnalyzerPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

float SpectrumAnalyzerPluginComponent::xForFrequency(juce::Rectangle<float> area, double frequency, double sampleRate) const
{
    const double minFrequency = SpectrumAnalyzerPlugin::minDisplayFrequency;
    const double maxFrequency = juce::jmax(minFrequency, sampleRate * 0.5);
    const double denominator = std::log(maxFrequency / minFrequency);
    if (denominator <= 0.0)
        return area.getX();

    const double clamped = juce::jlimit(minFrequency, maxFrequency, juce::jmax(frequency, minFrequency));
    const double normalized = std::log(clamped / minFrequency) / denominator;
    return area.getX() + (float)normalized * area.getWidth();
}

SpectrumAnalyzerPlugin *SpectrumAnalyzerPluginComponent::getAnalyzer() const noexcept { return dynamic_cast<SpectrumAnalyzerPlugin *>(m_plugin.get()); }

float SpectrumAnalyzerPluginComponent::yForDb(juce::Rectangle<float> area, float db) const
{
    const float clamped = juce::jlimit(SpectrumAnalyzerPlugin::minDb, 0.0f, db);
    const float normalized = (clamped - SpectrumAnalyzerPlugin::minDb) / -SpectrumAnalyzerPlugin::minDb;
    return area.getBottom() - normalized * area.getHeight();
}

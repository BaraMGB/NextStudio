/*
  ==============================================================================

    DelayPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/Delay/DelayPluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h"

#include <cmath>

class DelayPluginComponent::DelayStageGraphComponent : public juce::Component
{
public:
    DelayStageGraphComponent(DelayPluginComponent &owner, te::AutomatableParameter::Ptr mode, te::AutomatableParameter::Ptr sync, te::AutomatableParameter::Ptr division, te::AutomatableParameter::Ptr timeMs, te::AutomatableParameter::Ptr feedback, te::AutomatableParameter::Ptr mix, te::AutomatableParameter::Ptr stereoOffset, te::AutomatableParameter::Ptr pingPongAmount)
        : m_owner(owner),
          m_mode(std::move(mode)),
          m_sync(std::move(sync)),
          m_division(std::move(division)),
          m_timeMs(std::move(timeMs)),
          m_feedback(std::move(feedback)),
          m_mix(std::move(mix)),
          m_stereoOffset(std::move(stereoOffset)),
          m_pingPongAmount(std::move(pingPongAmount))
    {
    }

    void paint(juce::Graphics &g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.getWidth() < 40.0f || bounds.getHeight() < 30.0f)
            return;

        const auto trackColour = m_owner.getTrackColour();
        auto &appState = m_owner.m_editViewState.m_applicationState;
        auto panel = bounds.reduced(4.0f);
        GUIHelpers::drawHeaderBox(g, panel, trackColour, appState.getBorderColour(), appState.getBackgroundColour1(), 22.0f, GUIHelpers::HeaderPosition::top);

        auto header = panel.removeFromTop(22.0f);

        g.setColour(trackColour.contrasting(0.85f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText("DELAY SPACE", header.toNearestInt().reduced(8, 0), juce::Justification::centredLeft, 1);

        const juce::String modeText = m_mode != nullptr ? m_mode->getCurrentValueAsString() : "Mono";
        juce::String rightText;
        if (m_sync != nullptr && m_sync->getCurrentValue() >= 0.5f)
            rightText = "Sync " + (m_division != nullptr ? m_division->getCurrentValueAsString() : juce::String("1/8"));
        else
            rightText = m_timeMs != nullptr ? m_timeMs->getCurrentValueAsString() : juce::String("250 ms");

        g.drawFittedText(modeText + "  |  " + rightText, header.toNearestInt().reduced(8, 0), juce::Justification::centredRight, 1);

        auto graph = panel.reduced(8.0f, 6.0f);
        g.setColour(juce::Colour(0xff1a212b));
        g.fillRoundedRectangle(graph, 6.0f);

        const float mix = m_mix != nullptr ? juce::jlimit(0.0f, 1.0f, m_mix->getCurrentValue()) : 0.25f;
        const float mixVisual = std::pow(mix, 0.58f);
        const float feedback = m_feedback != nullptr ? juce::jlimit(0.0f, 0.95f, m_feedback->getCurrentValue()) : 0.35f;
        const float pingPong = m_pingPongAmount != nullptr ? juce::jlimit(0.0f, 1.0f, m_pingPongAmount->getCurrentValue()) : 1.0f;
        const float offsetNorm = m_stereoOffset != nullptr ? juce::jlimit(-1.0f, 1.0f, m_stereoOffset->getCurrentValue() / 100.0f) : 0.0f;
        const bool syncOn = m_sync != nullptr && m_sync->getCurrentValue() >= 0.5f;

        float timeNorm = 0.25f;
        if (syncOn)
        {
            auto divisionToQuarterNotes = [](int divisionIndex)
            {
                switch (juce::jlimit(0, 11, divisionIndex))
                {
                case 0:
                    return 4.0f; // 1/1
                case 1:
                    return 2.0f; // 1/2
                case 2:
                    return 1.0f; // 1/4
                case 3:
                    return 0.5f; // 1/8
                case 4:
                    return 0.25f; // 1/16
                case 5:
                    return 0.125f; // 1/32
                case 6:
                    return 1.5f; // 1/4D
                case 7:
                    return 0.75f; // 1/8D
                case 8:
                    return 0.375f; // 1/16D
                case 9:
                    return 2.0f / 3.0f; // 1/4T
                case 10:
                    return 1.0f / 3.0f; // 1/8T
                case 11:
                    return 1.0f / 6.0f; // 1/16T
                default:
                    return 0.5f;
                }
            };

            const int divisionIndex = m_division != nullptr ? juce::roundToInt(m_division->getCurrentValue()) : 3;
            const float quarterNotes = divisionToQuarterNotes(divisionIndex);

            const auto transportPos = m_owner.m_editViewState.m_edit.getTransport().getPosition();
            const auto bpm = (float)juce::jlimit(10.0, 400.0, m_owner.m_editViewState.m_edit.tempoSequence.getTempoAt(transportPos).getBpm());
            const float quarterNoteMs = 60000.0f / bpm;
            const float syncedMs = quarterNoteMs * quarterNotes;

            timeNorm = juce::jmap(juce::jlimit(20.0f, 2000.0f, syncedMs), 20.0f, 2000.0f, 0.0f, 1.0f);
        }
        else if (m_timeMs != nullptr)
            timeNorm = juce::jmap(juce::jlimit(20.0f, 2000.0f, m_timeMs->getCurrentValue()), 20.0f, 2000.0f, 0.0f, 1.0f);

        const bool stereoLike = (modeText == "Stereo" || modeText == "Dual");
        const bool pingPongMode = modeText == "PingPong";

        const float firstTapX = graph.getX() + graph.getWidth() * 0.08f;
        const float minTapSpread = graph.getWidth() * 0.06f;
        const float maxTapSpread = graph.getWidth() * 0.86f;
        const float tapSpread = juce::jmap(timeNorm, 0.0f, 1.0f, minTapSpread, maxTapSpread);
        const float laneSpreadPx = offsetNorm * graph.getWidth() * 0.14f;

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.07f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = graph.getX() + graph.getWidth() * ((float)i / 8.0f);
            g.drawVerticalLine((int)x, graph.getY(), graph.getBottom());
        }

        const float midY = graph.getCentreY();
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.12f));
        g.drawHorizontalLine((int)midY, graph.getX(), graph.getRight());

        auto drawTapLane = [&](float laneY, float laneDirection)
        {
            const int tapCount = 8;
            float amp = 0.006f + 0.994f * mixVisual;
            for (int i = 0; i < tapCount; ++i)
            {
                const float t = tapCount > 1 ? (float)i / (float)(tapCount - 1) : 0.0f;
                float x = firstTapX + tapSpread * t;

                if (stereoLike)
                    x += laneDirection * laneSpreadPx * (0.25f + 0.75f * t);

                const float radius = 1.2f + 9.0f * amp;
                const float alpha = juce::jlimit(0.008f, 0.92f, 0.02f + 0.9f * amp);
                g.setColour(trackColour.brighter(0.35f).withAlpha(alpha));
                g.fillEllipse(x - radius * 0.5f, laneY - radius * 0.5f, radius, radius);

                g.setColour(trackColour.withAlpha(alpha * 0.45f));
                g.drawEllipse(x - radius * 0.8f, laneY - radius * 0.8f, radius * 1.6f, radius * 1.6f, 1.0f);

                amp *= feedback;
            }
        };

        if (pingPongMode)
        {
            const float topY = graph.getY() + graph.getHeight() * 0.28f;
            const float bottomY = graph.getY() + graph.getHeight() * 0.72f;
            const int tapCount = 9;
            float amp = 0.008f + 0.992f * mixVisual;
            for (int i = 0; i < tapCount; ++i)
            {
                const float t = tapCount > 1 ? (float)i / (float)(tapCount - 1) : 0.0f;
                const float x = firstTapX + tapSpread * t;
                const bool onTop = (i % 2) == 0;
                const float y = onTop ? topY : bottomY;
                const float radius = 1.2f + 9.0f * amp;
                const float alpha = juce::jlimit(0.01f, 0.95f, 0.03f + 0.9f * amp);
                g.setColour(trackColour.brighter(0.45f).withAlpha(alpha));
                g.fillEllipse(x - radius * 0.5f, y - radius * 0.5f, radius, radius);

                const float previousT = tapCount > 1 ? (float)(i - 1) / (float)(tapCount - 1) : 0.0f;
                const float previousX = i == 0 ? firstTapX : firstTapX + tapSpread * previousT;
                const float previousY = i == 0 ? topY : ((i - 1) % 2 == 0 ? topY : bottomY);
                g.setColour(trackColour.withAlpha((0.06f + 0.34f * mixVisual) + pingPong * 0.28f));
                g.drawLine(previousX, previousY, x, y, 1.4f);

                amp *= feedback;
                amp *= juce::jmap(pingPong, 0.0f, 1.0f, 0.95f, 1.0f);
            }
        }
        else
        {
            const float topLane = graph.getY() + graph.getHeight() * 0.34f;
            const float bottomLane = graph.getY() + graph.getHeight() * 0.66f;
            drawTapLane(topLane, 1.0f);
            drawTapLane(bottomLane, -1.0f);
        }

        g.setColour(juce::Colour(0xffffffff).withAlpha(0.58f));
        g.setFont(10.0f);
        g.drawFittedText("L", juce::Rectangle<int>((int)graph.getX() + 4, (int)(graph.getY() + 4), 12, 12), juce::Justification::centred, 1);
        g.drawFittedText("R", juce::Rectangle<int>((int)graph.getX() + 4, (int)(graph.getBottom() - 16), 12, 12), juce::Justification::centred, 1);
    }

private:
    DelayPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_mode, m_sync, m_division, m_timeMs, m_feedback, m_mix, m_stereoOffset, m_pingPongAmount;
};

DelayPluginComponent::DelayPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    if (isNextDelay())
    {
        m_graph = std::make_unique<DelayStageGraphComponent>(*this, m_plugin->getAutomatableParameterByID("mode"), m_plugin->getAutomatableParameterByID("syncEnabled"), m_plugin->getAutomatableParameterByID("syncDivision"), m_plugin->getAutomatableParameterByID("timeMs"), m_plugin->getAutomatableParameterByID("feedback"), m_plugin->getAutomatableParameterByID("mix"), m_plugin->getAutomatableParameterByID("stereoOffsetMs"), m_plugin->getAutomatableParameterByID("pingPongAmount"));

        m_mode = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("mode"), "Mode");
        m_syncEnabled = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("syncEnabled"), "Sync");
        m_syncDivision = std::make_unique<AutomatableChoiceComponent>(m_plugin->getAutomatableParameterByID("syncDivision"), "Division");

        m_time = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("timeMs"), "Time");
        m_fbParCom = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
        m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix"), "Mix");
        m_stereoOffset = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("stereoOffsetMs"), "Offset");
        m_pingPongAmount = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("pingPongAmount"), "PingPong");
        m_hpCutoff = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("hpCutoff"), "HP");
        m_lpCutoff = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("lpCutoff"), "LP");

        addAndMakeVisible(*m_graph);
        addAndMakeVisible(*m_mode);
        addAndMakeVisible(*m_syncEnabled);
        addAndMakeVisible(*m_syncDivision);
        addAndMakeVisible(*m_time);
        addAndMakeVisible(*m_fbParCom);
        addAndMakeVisible(*m_mix);
        addAndMakeVisible(*m_stereoOffset);
        addAndMakeVisible(*m_pingPongAmount);
        addAndMakeVisible(*m_hpCutoff);
        addAndMakeVisible(*m_lpCutoff);
    }
    else
    {
        m_fbParCom = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("feedback"), "FB");
        addAndMakeVisible(*m_fbParCom);
        m_mix = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("mix proportion"), "Mix");
        addAndMakeVisible(*m_mix);

        auto timeVal = m_plugin->state.getPropertyAsValue(te::IDs::length, &m_editViewState.m_edit.getUndoManager());
        if (static_cast<double>(timeVal.getValue()) < 1.0)
            timeVal = 1.0;

        m_legacyTime = std::make_unique<NonAutomatableParameterComponent>(timeVal, "Time", 1, 1000);
        addAndMakeVisible(*m_legacyTime);
    }

    m_plugin->state.addListener(this);
}

DelayPluginComponent::~DelayPluginComponent() { m_plugin->state.removeListener(this); }

void DelayPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void DelayPluginComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    if (isNextDelay())
    {
        auto graphArea = bounds.removeFromTop((int)(bounds.getHeight() * 0.42f));
        m_graph->setBounds(graphArea);

        bounds.removeFromTop(4);
        const int rowHeight = juce::jmax(28, bounds.getHeight() / 3);

        auto row = bounds.removeFromTop(rowHeight);
        auto colW = row.getWidth() / 3;
        m_mode->setBounds(row.removeFromLeft(colW).reduced(2));
        m_syncEnabled->setBounds(row.removeFromLeft(colW).reduced(2));
        m_syncDivision->setBounds(row.reduced(2));

        row = bounds.removeFromTop(rowHeight);
        colW = row.getWidth() / 3;
        m_time->setBounds(row.removeFromLeft(colW).reduced(2));
        m_fbParCom->setBounds(row.removeFromLeft(colW).reduced(2));
        m_mix->setBounds(row.reduced(2));

        row = bounds.removeFromTop(rowHeight);
        colW = row.getWidth() / 4;
        m_stereoOffset->setBounds(row.removeFromLeft(colW).reduced(2));
        m_pingPongAmount->setBounds(row.removeFromLeft(colW).reduced(2));
        m_hpCutoff->setBounds(row.removeFromLeft(colW).reduced(2));
        m_lpCutoff->setBounds(row.reduced(2));
    }
    else
    {
        auto h = bounds.getHeight() / 12;
        m_fbParCom->setBounds(bounds.removeFromTop(h * 4));
        m_mix->setBounds(bounds.removeFromTop(h * 4));
        m_legacyTime->setBounds(bounds.removeFromTop(h * 4));
    }
}

juce::ValueTree DelayPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree DelayPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", getPluginTypeName(), nullptr);
    return defaultState;
}

void DelayPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String DelayPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String DelayPluginComponent::getPluginTypeName() const { return isNextDelay() ? NextDelayPlugin::xmlTypeName : juce::String("delay"); }

ApplicationViewState &DelayPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

void DelayPluginComponent::valueTreeChanged() {}

void DelayPluginComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    if (!isNextDelay())
    {
        if (i == te::IDs::feedback && m_fbParCom)
            m_fbParCom->updateLabel();
        if (i == te::IDs::mix && m_mix)
            m_mix->updateLabel();
        return;
    }

    const juce::Identifier timeId("timeMs");
    const juce::Identifier feedbackId("feedback");
    const juce::Identifier mixId("mix");
    const juce::Identifier offsetId("stereoOffsetMs");
    const juce::Identifier pingPongId("pingPongAmount");
    const juce::Identifier hpId("hpCutoff");
    const juce::Identifier lpId("lpCutoff");

    if (i == timeId && m_time)
        m_time->updateLabel();
    else if (i == feedbackId && m_fbParCom)
        m_fbParCom->updateLabel();
    else if (i == mixId && m_mix)
        m_mix->updateLabel();
    else if (i == offsetId && m_stereoOffset)
        m_stereoOffset->updateLabel();
    else if (i == pingPongId && m_pingPongAmount)
        m_pingPongAmount->updateLabel();
    else if (i == hpId && m_hpCutoff)
        m_hpCutoff->updateLabel();
    else if (i == lpId && m_lpCutoff)
        m_lpCutoff->updateLabel();

    if (m_graph)
        m_graph->repaint();
}

void DelayPluginComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) {}

void DelayPluginComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) {}

void DelayPluginComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) {}

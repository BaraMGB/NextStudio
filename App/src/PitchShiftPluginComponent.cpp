#include "PitchShiftPluginComponent.h"

#include "PitchShiftDisplay.h"
#include "PitchShiftDrag.h"
#include "PresetHelpers.h"

#include <cmath>

namespace
{
juce::Rectangle<int> toJuceRectangle(EffectEditorLayout::Rectangle area) { return {area.x, area.y, area.width, area.height}; }
} // namespace

class PitchShiftPluginComponent::PitchMapComponent
    : public juce::Component
    , public juce::SettableTooltipClient
    , private te::AutomatableParameter::Listener
{
public:
    PitchMapComponent(PitchShiftPluginComponent &owner, te::AutomatableParameter::Ptr parameter)
        : m_owner(owner), m_parameter(std::move(parameter)), m_drag(owner.getPlugin(), owner.m_editViewState.m_edit.getUndoManager())
    {
        setWantsKeyboardFocus(true);
        setTooltip(TRANS("Drag the pitch point up or down to select whole semitones."));
        if (m_parameter != nullptr)
            m_parameter->addListener(this);
    }

    ~PitchMapComponent() override
    {
        m_drag.finish();
        if (m_parameter != nullptr)
            m_parameter->removeListener(this);
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (!event.mods.isLeftButtonDown() || !isHandleAt(event.position) || !m_drag.begin())
            return;

        m_startValue = m_parameter->getCurrentBaseValue();
        m_startY = event.position.y;
        m_dragScaleHeight = getScaleBounds().getHeight();
        m_dragging = true;
        grabKeyboardFocus();
        updateHover(true);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override
    {
        if (!m_dragging)
            return;

        m_drag.update(PitchShiftDisplay::snappedSemitonesForDrag(m_startValue, event.position.y - m_startY, m_dragScaleHeight, te::PitchShiftPlugin::getMaximumSemitones()));
        repaint();
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        finishDrag();
        updateHover(isHandleAt(event.position));
    }

    void mouseMove(const juce::MouseEvent &event) override { updateHover(isHandleAt(event.position)); }
    void mouseExit(const juce::MouseEvent &) override { updateHover(false); }

    bool keyPressed(const juce::KeyPress &key) override
    {
        if (m_dragging && key == juce::KeyPress::escapeKey)
        {
            m_drag.cancel();
            m_dragging = false;
            updateHover(false);
            repaint();
            return true;
        }
        return false;
    }

    void focusLost(FocusChangeType) override { finishDrag(); }
    void enablementChanged() override { if (!isEnabled()) finishDrag(); }
    void visibilityChanged() override { if (!isShowing()) finishDrag(); }

    void paint(juce::Graphics &g) override
    {
        if (getWidth() < 60 || getHeight() < 75)
            return;

        auto panel = getLocalBounds().toFloat().reduced(4.0f);
        auto &appState = m_owner.m_editViewState.m_applicationState;
        const auto colour = m_owner.getTrackColour();
        constexpr float headerHeight = 22.0f;
        GUIHelpers::drawHeaderBox(g, panel, colour, appState.getBorderColour(), appState.getBackgroundColour1(), headerHeight, GUIHelpers::HeaderPosition::top);

        const auto header = panel.removeFromTop(headerHeight);
        g.setColour(colour.contrasting(0.85f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText("PITCH MAP", header.toNearestInt().reduced(4, 0), juce::Justification::centred, 1);

        auto graph = getGraphBounds();
        g.setColour(appState.getBackgroundColour2());
        g.fillRoundedRectangle(graph, 5.0f);

        const auto textColour = appState.getTextColour();
        const auto scale = getScaleBounds();
        const auto scaleTop = scale.getY();
        const auto scaleHeight = scale.getHeight();
        const auto axisX = scale.getX();
        const auto gridLeft = graph.getX() + 28.0f;
        const auto maximum = te::PitchShiftPlugin::getMaximumSemitones();
        auto yForPitch = [&](float semitones) { return scaleTop + scaleHeight * PitchShiftDisplay::positionForSemitones(semitones, maximum); };
        const auto zeroY = yForPitch(0.0f);
        const auto shift = m_parameter != nullptr ? m_parameter->getCurrentValue() : 0.0f;
        const auto outputY = yForPitch(shift);

        g.setFont(juce::FontOptions(10.0f));
        for (int octave = -2; octave <= 2; ++octave)
        {
            const int semitones = octave * 12;
            const auto y = yForPitch(static_cast<float>(semitones));
            g.setColour(textColour.withAlpha(semitones == 0 ? 0.35f : 0.12f));
            g.drawLine(gridLeft, y, graph.getRight() - 4.0f, y, 1.0f);

            g.setColour(textColour.withAlpha(semitones == 0 ? 0.9f : 0.65f));
            const auto label = semitones > 0 ? "+" + juce::String(semitones) : juce::String(semitones);
            g.drawText(label, juce::Rectangle<float>(graph.getX() + 2.0f, y - 6.0f, 23.0f, 12.0f).toNearestInt(), juce::Justification::centredRight, false);
        }

        g.setColour(textColour.withAlpha(0.2f));
        g.drawLine(axisX, scaleTop, axisX, scaleTop + scaleHeight, 1.0f);
        g.setColour(textColour.withAlpha(0.6f));
        g.drawEllipse(axisX - 4.0f, zeroY - 4.0f, 8.0f, 8.0f, 1.3f);

        if (std::abs(outputY - zeroY) > 2.0f)
        {
            g.setColour(colour.withAlpha(0.85f));
            g.drawLine(axisX, zeroY, axisX, outputY, 2.0f);
            const float direction = outputY < zeroY ? 1.0f : -1.0f;
            juce::Path arrow;
            arrow.addTriangle(axisX, outputY, axisX - 3.5f, outputY + direction * 6.0f, axisX + 3.5f, outputY + direction * 6.0f);
            g.fillPath(arrow);
        }

        const auto radius = (m_hover || m_dragging) ? 4.5f : 3.5f;
        g.setColour(colour.withAlpha((m_hover || m_dragging) ? 0.3f : 0.16f));
        g.fillEllipse(axisX - 7.0f, outputY - 7.0f, 14.0f, 14.0f);
        g.setColour(colour);
        g.fillEllipse(axisX - radius, outputY - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(textColour.withAlpha(0.85f));
        g.drawEllipse(axisX - radius, outputY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }

private:
    juce::Rectangle<float> getGraphBounds() const
    {
        auto panel = getLocalBounds().toFloat().reduced(4.0f);
        panel.removeFromTop(22.0f);
        return panel.reduced(6.0f);
    }

    juce::Rectangle<float> getScaleBounds() const
    {
        const auto graph = getGraphBounds();
        return {graph.getRight() - 12.0f, graph.getY() + 8.0f, 0.0f, juce::jmax(0.0f, graph.getHeight() - 16.0f)};
    }

    bool isHandleAt(juce::Point<float> position) const
    {
        if (!isEnabled() || m_parameter == nullptr || getWidth() < 60 || getHeight() < 75)
            return false;

        const auto scale = getScaleBounds();
        const auto y = scale.getY() + scale.getHeight() * PitchShiftDisplay::positionForSemitones(m_parameter->getCurrentValue(), te::PitchShiftPlugin::getMaximumSemitones());
        return position.getDistanceFrom({scale.getX(), y}) <= 12.0f;
    }

    void updateHover(bool hover)
    {
        if (m_hover != hover)
        {
            m_hover = hover;
            repaint();
        }
        setMouseCursor(m_hover || m_dragging ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
    }

    void finishDrag()
    {
        if (!m_dragging)
            return;

        m_drag.finish();
        m_dragging = false;
        updateHover(false);
        repaint();
    }

    void curveHasChanged(te::AutomatableParameter &) override {}
    void currentValueChanged(te::AutomatableParameter &) override { repaint(); }
    void parameterChanged(te::AutomatableParameter &, float) override { repaint(); }

    PitchShiftPluginComponent &m_owner;
    te::AutomatableParameter::Ptr m_parameter;
    PitchShiftDrag m_drag;
    float m_startValue = 0.0f;
    float m_startY = 0.0f;
    float m_dragScaleHeight = 0.0f;
    bool m_dragging = false;
    bool m_hover = false;
};

PitchShiftPluginComponent::PitchShiftPluginComponent(EditViewState &evs, te::Plugin::Ptr plugin)
    : PluginViewComponent(evs, plugin)
{
    const auto parameter = m_plugin->getAutomatableParameterByID("semitones up");
    m_graph = std::make_unique<PitchMapComponent>(*this, parameter);
    m_semitones = std::make_unique<AutomatableParameterComponent>(parameter, TRANS("Semitones"));
    addAndMakeVisible(*m_graph);
    addAndMakeVisible(*m_semitones);
}

PitchShiftPluginComponent::~PitchShiftPluginComponent() = default;

void PitchShiftPluginComponent::paint(juce::Graphics &g) { g.fillAll(m_editViewState.m_applicationState.getBackgroundColour2()); }

void PitchShiftPluginComponent::resized()
{
    const auto layout = EffectEditorLayout::pitchShifter(getWidth(), getHeight());
    m_graph->setBounds(toJuceRectangle(layout.graph));
    m_semitones->setBounds(toJuceRectangle(layout.parameter));
}

juce::ValueTree PitchShiftPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree PitchShiftPluginComponent::getFactoryDefaultState() { return te::PitchShiftPlugin::create(); }

void PitchShiftPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String PitchShiftPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String PitchShiftPluginComponent::getPluginTypeName() const { return te::PitchShiftPlugin::xmlTypeName; }

ApplicationViewState &PitchShiftPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

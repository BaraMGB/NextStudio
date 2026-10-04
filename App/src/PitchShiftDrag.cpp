#include "PitchShiftDrag.h"

#include "AutomationWriteGuard.h"

#include <cmath>

namespace
{
void applyPitch(tracktion_engine::AutomatableParameter &parameter, float value)
{
    auto *plugin = dynamic_cast<tracktion_engine::PitchShiftPlugin *>(parameter.getPlugin());
    jassert(plugin != nullptr);
    if (plugin == nullptr)
        return;

    // The graph owns the complete undo action. Keep Tracktion's parameter binding,
    // but suppress intermediate CachedValue undo records (also during undo/redo).
    // Otherwise they can restore the property without restoring the parameter base.
    struct ScopedCachedUndo
    {
        explicit ScopedCachedUndo(tracktion_engine::PitchShiftPlugin &p) : plugin(p)
        {
            plugin.semitonesValue.referTo(plugin.state, tracktion_engine::IDs::semitonesUp, nullptr);
        }
        ~ScopedCachedUndo()
        {
            plugin.semitonesValue.referTo(plugin.state, tracktion_engine::IDs::semitonesUp, plugin.getUndoManager());
        }
        tracktion_engine::PitchShiftPlugin &plugin;
    } scopedUndo(*plugin);

    AutomationWriteGuard::markTouched(&parameter);
    parameter.setParameter(value, juce::sendNotification);
}

class PitchShiftDragAction final : public juce::UndoableAction
{
public:
    PitchShiftDragAction(tracktion_engine::Plugin::Ptr plugin, tracktion_engine::AutomatableParameter::Ptr parameter, float previousValue, float nextValue)
        : m_plugin(std::move(plugin)), m_parameter(std::move(parameter)), m_previousValue(previousValue), m_nextValue(nextValue)
    {
    }

    bool perform() override { return apply(m_nextValue); }
    bool undo() override { return apply(m_previousValue); }
    int getSizeInUnits() override { return static_cast<int>(sizeof(*this)); }

private:
    bool apply(float value)
    {
        if (std::abs(m_parameter->getCurrentBaseValue() - value) > 0.0001f)
        {
            m_parameter->beginParameterChangeGesture();
            m_parameter->parameterChangeGestureBegin();
            applyPitch(*m_parameter, value);
            m_parameter->endParameterChangeGesture();
            m_parameter->parameterChangeGestureEnd();
        }
        return true;
    }

    tracktion_engine::Plugin::Ptr m_plugin;
    tracktion_engine::AutomatableParameter::Ptr m_parameter;
    float m_previousValue;
    float m_nextValue;
};
} // namespace

PitchShiftDrag::PitchShiftDrag(tracktion_engine::Plugin::Ptr plugin, juce::UndoManager &undoManager)
    : m_plugin(std::move(plugin)),
      m_parameter(m_plugin != nullptr ? m_plugin->getAutomatableParameterByID("semitones up") : nullptr),
      m_undoManager(undoManager)
{
}

PitchShiftDrag::~PitchShiftDrag() { finish(); }

bool PitchShiftDrag::begin()
{
    if (m_active || m_parameter == nullptr)
        return false;

    m_previousValue = m_parameter->getCurrentBaseValue();
    m_active = true;
    m_parameter->beginParameterChangeGesture();
    m_parameter->parameterChangeGestureBegin();
    return true;
}

void PitchShiftDrag::update(float semitones)
{
    if (!m_active || !std::isfinite(semitones))
        return;

    const auto maximum = tracktion_engine::PitchShiftPlugin::getMaximumSemitones();
    const auto value = juce::jlimit(-maximum, maximum, std::round(semitones));
    if (std::abs(m_parameter->getCurrentBaseValue() - value) > 0.0001f)
        applyPitch(*m_parameter, value);
}

void PitchShiftDrag::finish()
{
    if (!m_active)
        return;

    m_active = false;
    const auto nextValue = m_parameter->getCurrentBaseValue();
    m_parameter->endParameterChangeGesture();
    m_parameter->parameterChangeGestureEnd();
    if (std::abs(nextValue - m_previousValue) <= 0.0001f)
        return;

    const auto transactionName = TRANS("Change Pitch Shift");
    m_undoManager.beginNewTransaction(transactionName);
    m_undoManager.perform(new PitchShiftDragAction(m_plugin, m_parameter, m_previousValue, nextValue), transactionName);
    m_undoManager.beginNewTransaction();
}

void PitchShiftDrag::cancel()
{
    if (!m_active)
        return;

    if (std::abs(m_parameter->getCurrentBaseValue() - m_previousValue) > 0.0001f)
        applyPitch(*m_parameter, m_previousValue);
    m_parameter->endParameterChangeGesture();
    m_parameter->parameterChangeGestureEnd();
    m_active = false;
}

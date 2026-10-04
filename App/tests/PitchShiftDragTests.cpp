#include "PitchShiftDrag.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

bool closeTo(float a, float b) { return std::abs(a - b) < 0.0001f; }

class ParameterListener final : public tracktion_engine::AutomatableParameter::Listener
{
public:
    void curveHasChanged(tracktion_engine::AutomatableParameter &) override {}
    void parameterChanged(tracktion_engine::AutomatableParameter &, float) override { ++changes; }
    void parameterChangeGestureBegin(tracktion_engine::AutomatableParameter &) override { ++begins; }
    void parameterChangeGestureEnd(tracktion_engine::AutomatableParameter &) override { ++ends; }

    int changes = 0;
    int begins = 0;
    int ends = 0;
};

void testPitchDrag()
{
    namespace te = tracktion_engine;
    te::Engine engine("NextStudioPitchShiftDragTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    auto plugin = edit->getPluginCache().createNewPlugin(te::PitchShiftPlugin::xmlTypeName, {});
    REQUIRE(plugin != nullptr);
    if (plugin == nullptr)
        return;

    const auto parameter = plugin->getAutomatableParameterByID("semitones up");
    REQUIRE(parameter != nullptr);
    if (parameter == nullptr)
        return;

    auto &undo = edit->getUndoManager();
    parameter->setParameter(0.375f, juce::sendNotification);
    undo.clearUndoHistory();
    ParameterListener listener;
    parameter->addListener(&listener);
    {
        PitchShiftDrag drag(plugin, undo);
        drag.update(12.0f); // Not dragging: no change.
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 0.375f));
        REQUIRE(drag.begin());
        REQUIRE(!drag.begin());
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 0.375f)); // Mouse-down must not snap.
        drag.update(12.2f);
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 12.0f));
        REQUIRE(closeTo(static_cast<float>(plugin->state.getProperty(te::IDs::semitonesUp)), 12.0f));
        drag.update(-6.4f);
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), -6.0f));
        const int changes = listener.changes;
        drag.update(-6.4f);
        REQUIRE(listener.changes == changes);
        drag.update(std::numeric_limits<float>::quiet_NaN());
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), -6.0f));
        drag.update(200.0f);
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 24.0f));
        drag.update(-200.0f);
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), -24.0f));
        drag.update(7.1f);
        drag.finish();
        drag.finish();
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 7.0f));
        REQUIRE(listener.begins == 1 && listener.ends == 1);
        REQUIRE(undo.canUndo());
        REQUIRE(undo.undo());
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 0.375f));
        REQUIRE(closeTo(static_cast<float>(plugin->state.getProperty(te::IDs::semitonesUp)), 0.375f));
        REQUIRE(!undo.canUndo()); // All intermediate graph values form one undo step.
        REQUIRE(undo.redo());
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 7.0f));
        REQUIRE(!undo.canRedo());
        REQUIRE(listener.begins == 3 && listener.ends == 3);
    }

    undo.clearUndoHistory();
    {
        PitchShiftDrag drag(plugin, undo);
        REQUIRE(drag.begin());
        drag.finish();
        REQUIRE(!undo.canUndo()); // Clicking without dragging has no undo entry.
        REQUIRE(drag.begin());
        drag.update(8.0f);
        drag.update(7.0f);
        drag.finish();
        REQUIRE(!undo.canUndo()); // Returning to the original value is a no-op.
        REQUIRE(drag.begin());
        drag.update(-12.0f);
        drag.cancel();
        drag.cancel();
        REQUIRE(closeTo(parameter->getCurrentBaseValue(), 7.0f));
        REQUIRE(!undo.canUndo());
        REQUIRE(listener.begins == listener.ends);
    }

    {
        PitchShiftDrag drag(plugin, undo);
        REQUIRE(drag.begin());
        drag.update(5.0f);
    } // Closing/rebuilding the editor must end its gesture and retain one undo step.
    REQUIRE(listener.begins == listener.ends);
    REQUIRE(closeTo(parameter->getCurrentBaseValue(), 5.0f));
    REQUIRE(undo.undo());
    REQUIRE(closeTo(parameter->getCurrentBaseValue(), 7.0f));
    REQUIRE(!undo.canUndo());

    parameter->setParameter(1.125f, juce::sendNotification);
    REQUIRE(closeTo(parameter->getCurrentBaseValue(), 1.125f));
    REQUIRE(parameter->valueRange.interval == 0.0f); // Native knob/automation range stays continuous.
    parameter->removeListener(&listener);
    undo.clearUndoHistory();
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testPitchDrag();
    if (failures == 0)
        std::cout << "Pitch shift drag tests passed\n";
    return failures == 0 ? 0 : 1;
}

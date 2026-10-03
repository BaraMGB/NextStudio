#include "EqBandReset.h"

#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

bool closeTo(float actual, float expected)
{
    return std::abs(actual - expected) < 0.0001f;
}

class ParameterListener final : public tracktion_engine::AutomatableParameter::Listener
{
public:
    void curveHasChanged(tracktion_engine::AutomatableParameter &) override {}
    void parameterChanged(tracktion_engine::AutomatableParameter &, float) override { ++changeCount; }

    int changeCount = 0;
};

struct BandTest
{
    const char *frequencyId;
    const char *gainId;
    const char *qId;
    float defaultFrequency;
};

void testEveryBandUsesFactoryDefaultsAndOneUndoStep()
{
    namespace te = tracktion_engine;

    te::Engine engine("NextStudioEqBandResetTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    auto plugin = edit->getPluginCache().createNewPlugin(te::EqualiserPlugin::xmlTypeName, {});
    REQUIRE(plugin != nullptr);
    if (plugin == nullptr)
        return;

    const std::array<BandTest, 4> bands{{
        {"Low-pass freq", "Low-pass gain", "Low-pass Q", 80.0f},
        {"Mid freq 1", "Mid gain 1", "Mid Q 1", 3000.0f},
        {"Mid freq 2", "Mid gain 2", "Mid Q 2", 5000.0f},
        {"High-pass freq", "High-pass gain", "High-pass Q", 17000.0f},
    }};

    for (size_t bandIndex = 0; bandIndex < bands.size(); ++bandIndex)
    {
        const auto &band = bands[bandIndex];
        EqBandParameters parameters{
            plugin->getAutomatableParameterByID(band.frequencyId),
            plugin->getAutomatableParameterByID(band.gainId),
            plugin->getAutomatableParameterByID(band.qId),
        };

        REQUIRE(parameters.frequency != nullptr);
        REQUIRE(parameters.gain != nullptr);
        REQUIRE(parameters.q != nullptr);
        if (parameters.frequency == nullptr || parameters.gain == nullptr || parameters.q == nullptr)
            continue;

        REQUIRE(parameters.frequency->getDefaultValue().has_value());
        REQUIRE(parameters.gain->getDefaultValue().has_value());
        REQUIRE(parameters.q->getDefaultValue().has_value());
        REQUIRE(closeTo(*parameters.frequency->getDefaultValue(), band.defaultFrequency));
        REQUIRE(closeTo(*parameters.gain->getDefaultValue(), 0.0f));
        REQUIRE(closeTo(*parameters.q->getDefaultValue(), 0.5f));

        const std::array<float, 3> changedValues{
            band.defaultFrequency == 17000.0f ? 12000.0f : band.defaultFrequency + 200.0f,
            6.0f,
            2.0f,
        };
        parameters.frequency->setParameter(changedValues[0], juce::sendNotification);
        parameters.gain->setParameter(changedValues[1], juce::sendNotification);
        parameters.q->setParameter(changedValues[2], juce::sendNotification);
        edit->getUndoManager().clearUndoHistory();

        ParameterListener listener;
        parameters.frequency->addListener(&listener);
        parameters.gain->addListener(&listener);
        parameters.q->addListener(&listener);

        REQUIRE(resetEqBandToFactoryDefaults(parameters,
                                               edit->getUndoManager(),
                                               "Reset EQ Band " + juce::String((int)bandIndex + 1)));
        REQUIRE(closeTo(parameters.frequency->getCurrentBaseValue(), band.defaultFrequency));
        REQUIRE(closeTo(parameters.gain->getCurrentBaseValue(), 0.0f));
        REQUIRE(closeTo(parameters.q->getCurrentBaseValue(), 0.5f));
        REQUIRE(listener.changeCount == 3);

        REQUIRE(edit->getUndoManager().undo());
        REQUIRE(closeTo(parameters.frequency->getCurrentBaseValue(), changedValues[0]));
        REQUIRE(closeTo(parameters.gain->getCurrentBaseValue(), changedValues[1]));
        REQUIRE(closeTo(parameters.q->getCurrentBaseValue(), changedValues[2]));

        REQUIRE(edit->getUndoManager().redo());
        REQUIRE(closeTo(parameters.frequency->getCurrentBaseValue(), band.defaultFrequency));
        REQUIRE(closeTo(parameters.gain->getCurrentBaseValue(), 0.0f));
        REQUIRE(closeTo(parameters.q->getCurrentBaseValue(), 0.5f));

        parameters.frequency->removeListener(&listener);
        parameters.gain->removeListener(&listener);
        parameters.q->removeListener(&listener);
        edit->getUndoManager().clearUndoHistory();
    }
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testEveryBandUsesFactoryDefaultsAndOneUndoStep();

    if (failures != 0)
        return 1;

    std::cout << "EQ band reset tests passed\n";
    return 0;
}

#include "EqBandReset.h"

#include <cmath>

namespace
{
class EqBandResetAction final : public juce::UndoableAction
{
public:
    EqBandResetAction(tracktion_engine::Plugin::Ptr ownerPluginToUse,
                      std::array<tracktion_engine::AutomatableParameter::Ptr, 3> parametersToUse,
                      std::array<float, 3> previousValuesToUse,
                      std::array<float, 3> defaultValuesToUse)
        : ownerPlugin(std::move(ownerPluginToUse)),
          parameters(std::move(parametersToUse)),
          previousValues(previousValuesToUse),
          defaultValues(defaultValuesToUse)
    {
    }

    bool perform() override { return apply(defaultValues); }
    bool undo() override { return apply(previousValues); }
    int getSizeInUnits() override { return (int)sizeof(*this); }

private:
    bool apply(const std::array<float, 3> &values)
    {
        for (size_t index = 0; index < parameters.size(); ++index)
        {
            auto &parameter = parameters[index];
            if (parameter == nullptr)
                return false;

            parameter->beginParameterChangeGesture();
            parameter->setParameter(values[index], juce::sendNotification);
            parameter->endParameterChangeGesture();
        }

        return true;
    }

    tracktion_engine::Plugin::Ptr ownerPlugin;
    std::array<tracktion_engine::AutomatableParameter::Ptr, 3> parameters;
    std::array<float, 3> previousValues;
    std::array<float, 3> defaultValues;
};
} // namespace

bool resetEqBandToFactoryDefaults(const EqBandParameters &parameters,
                                  juce::UndoManager &undoManager,
                                  const juce::String &transactionName)
{
    std::array<tracktion_engine::AutomatableParameter::Ptr, 3> parameterList{
        parameters.frequency,
        parameters.gain,
        parameters.q,
    };
    std::array<float, 3> previousValues{};
    std::array<float, 3> defaults{};
    bool needsReset = false;
    tracktion_engine::Plugin::Ptr ownerPlugin;

    for (size_t index = 0; index < parameterList.size(); ++index)
    {
        const auto &parameter = parameterList[index];
        if (parameter == nullptr)
            return false;

        auto *parameterPlugin = parameter->getPlugin();
        if (parameterPlugin == nullptr || (ownerPlugin != nullptr && ownerPlugin.get() != parameterPlugin))
            return false;
        ownerPlugin = parameterPlugin;

        const auto defaultValue = parameter->getDefaultValue();
        if (!defaultValue.has_value())
            return false;

        previousValues[index] = parameter->getCurrentBaseValue();
        defaults[index] = *defaultValue;
        needsReset = needsReset || std::abs(previousValues[index] - defaults[index]) > 0.0001f;
    }

    if (!needsReset)
        return true;

    undoManager.beginNewTransaction(transactionName);
    const auto performed = undoManager.perform(new EqBandResetAction(std::move(ownerPlugin),
                                                                     std::move(parameterList),
                                                                     previousValues,
                                                                     defaults),
                                               transactionName);
    undoManager.beginNewTransaction();
    return performed;
}

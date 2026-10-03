/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <tracktion_engine/tracktion_engine.h>

struct EqBandParameters
{
    tracktion_engine::AutomatableParameter::Ptr frequency;
    tracktion_engine::AutomatableParameter::Ptr gain;
    tracktion_engine::AutomatableParameter::Ptr q;
};

[[nodiscard]] bool resetEqBandToFactoryDefaults(const EqBandParameters &parameters,
                                                 juce::UndoManager &undoManager,
                                                 const juce::String &transactionName);

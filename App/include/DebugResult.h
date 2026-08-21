#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio::Debug
{
struct Result
{
    bool ok{false};
    juce::String code;
    juce::String message;
    juce::StringPairArray fields;

    static Result success(const juce::String &code = "ok", const juce::String &message = {})
    {
        Result result;
        result.ok = true;
        result.code = code;
        result.message = message;
        return result;
    }

    static Result failure(const juce::String &code, const juce::String &message)
    {
        Result result;
        result.ok = false;
        result.code = code;
        result.message = message;
        return result;
    }

    /** Serialises one complete JSON object without embedded physical newlines.
        JUCE's JSON encoder provides the escaping contract for all field values. */
    juce::String toResponseLine() const
    {
        auto *root = new juce::DynamicObject();
        root->setProperty("status", ok ? "ok" : "error");
        root->setProperty("code", code);

        if (message.isNotEmpty())
            root->setProperty("message", message);

        auto *fieldObject = new juce::DynamicObject();
        const auto keys = fields.getAllKeys();
        const auto values = fields.getAllValues();
        for (int i = 0; i < fields.size(); ++i)
            fieldObject->setProperty(juce::Identifier(keys[i]), values[i]);
        root->setProperty("fields", fieldObject);

        return juce::JSON::toString(juce::var(root), true);
    }
};
} // namespace NextStudio::Debug

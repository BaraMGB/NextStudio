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

    juce::String toResponseLine() const
    {
        juce::StringArray tokens;
        tokens.add(ok ? "ok" : "error");

        if (code.isNotEmpty())
            tokens.add("code=" + quoteIfNeeded(code));

        if (message.isNotEmpty())
            tokens.add("message=" + quoteIfNeeded(message));

        for (int i = 0; i < fields.size(); ++i)
            tokens.add(fields.getAllKeys()[i] + "=" + quoteIfNeeded(fields.getAllValues()[i]));

        return tokens.joinIntoString(" ");
    }

private:
    static juce::String quoteIfNeeded(const juce::String &value)
    {
        if (value.containsAnyOf(" \t\"="))
            return "\"" + value.replace("\"", "\\\"") + "\"";

        return value;
    }
};
} // namespace NextStudio::Debug

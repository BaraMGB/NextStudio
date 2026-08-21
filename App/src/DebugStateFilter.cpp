#include "DebugStateFilter.h"

namespace NextStudio::AgentDebug
{
juce::String sanitiseStateString(const juce::String &value)
{
    if (value.isEmpty())
        return {};

    for (auto character : value)
    {
        if ((character < 32 && character != '\n' && character != '\r' && character != '\t') || character == 0xfffd)
            return "<filtered-binary-data>";
    }

    if (value.length() > maximumStateStringLength)
        return value.substring(0, maximumStateStringLength) + "…<truncated>";

    return value;
}
} // namespace NextStudio::AgentDebug

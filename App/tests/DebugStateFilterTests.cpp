#include "DebugStateFilter.h"

#include <iostream>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do { if (! (condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

void testFiltering()
{
    using namespace NextStudio::AgentDebug;

    REQUIRE(sanitiseStateString({}).isEmpty());
    REQUIRE(sanitiseStateString("normal text\nwith tab\t") == "normal text\nwith tab\t");

    juce::String binaryLike("prefix");
    binaryLike += juce::String::charToString(1);
    binaryLike += "suffix";
    REQUIRE(sanitiseStateString(binaryLike) == "<filtered-binary-data>");

    const auto longText = juce::String::repeatedString("x", maximumStateStringLength + 10);
    const auto filtered = sanitiseStateString(longText);
    REQUIRE(filtered.startsWith(juce::String::repeatedString("x", maximumStateStringLength)));
    REQUIRE(filtered.endsWith("<truncated>"));
    REQUIRE(!filtered.contains(longText));
}
} // namespace

int main()
{
    testFiltering();
    if (failures != 0)
    {
        std::cerr << failures << " state filter test(s) failed.\n";
        return 1;
    }
    std::cout << "All debug state filter tests passed.\n";
    return 0;
}

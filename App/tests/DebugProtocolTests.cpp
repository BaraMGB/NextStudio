#include "DebugProtocol.h"
#include "DebugResult.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(condition) \
    do { if (! (condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

void testCommandParsing()
{
    using namespace NextStudio::Debug;

    REQUIRE(parseCommandLine("ping").type == CommandType::ping);
    REQUIRE(parseCommandLine(" SYSTEM_STATE ").type == CommandType::systemState);
    REQUIRE(parseCommandLine("transport_state").type == CommandType::transportState);
    REQUIRE(parseCommandLine("exit").type == CommandType::quit);

    const auto screenshot = parseCommandLine("screenshot\t 800 ");
    REQUIRE(screenshot.type == CommandType::screenshot);
    REQUIRE(screenshot.argument == "800");

    const auto ensureTrack = parseCommandLine(R"({"command":"ensure-track","arguments":{"type":"midi","name":"Agent = \"Track\" Gr\u00fc\u00dfe"}})");
    REQUIRE(ensureTrack.type == CommandType::ensureTrack);
    REQUIRE(ensureTrack.jsonRequest);
    REQUIRE(ensureTrack.parseError.isEmpty());
    REQUIRE(ensureTrack.arguments.getDynamicObject() != nullptr);
    if (auto *arguments = ensureTrack.arguments.getDynamicObject())
        REQUIRE(arguments->getProperty("name").toString()
                == juce::String::fromUTF8("Agent = \"Track\" Gr\xc3\xbc\xc3\x9f" "e"));

    const auto malformed = parseCommandLine("{not-json}");
    REQUIRE(malformed.type == CommandType::unknown);
    REQUIRE(malformed.parseError.isNotEmpty());

    const auto invalidArguments = parseCommandLine(R"({"command":"ping","arguments":[]})");
    REQUIRE(invalidArguments.parseError.isNotEmpty());

    REQUIRE(parseCommandLine("").type == CommandType::unknown);
    REQUIRE(parseCommandLine("does-not-exist").type == CommandType::unknown);
}

void testJsonResponseRoundTrip()
{
    using namespace NextStudio::Debug;

    auto result = NextStudio::Debug::Result::success("strange=code", "line 1\nline 2\t\"quoted\" \\ slash 😀");
    const auto value = juce::String::fromUTF8("spaces = equals, quote=\", slash=\\, controls=\t\n, unicode=Gr\xc3\xbc\xc3\x9f" "e");
    result.fields.set("value", value);

    const auto line = result.toResponseLine();
    REQUIRE(! line.containsChar('\n'));
    REQUIRE(isResponseLine(line));

    const auto parsed = juce::JSON::parse(line);
    auto *root = parsed.getDynamicObject();
    REQUIRE(root != nullptr);
    if (root == nullptr)
        return;

    REQUIRE(root->getProperty("status").toString() == "ok");
    REQUIRE(root->getProperty("code").toString() == "strange=code");
    REQUIRE(root->getProperty("message").toString() == result.message);

    auto *fields = root->getProperty("fields").getDynamicObject();
    REQUIRE(fields != nullptr);
    if (fields != nullptr)
        REQUIRE(fields->getProperty("value").toString() == value);
}

void testResponseRecognitionRejectsMalformedInput()
{
    using namespace NextStudio::Debug;

    REQUIRE(! isResponseLine("not json"));
    REQUIRE(! isResponseLine("{}"));
    REQUIRE(! isResponseLine("{\"status\":\"maybe\"}"));
    REQUIRE(isResponseLine(NextStudio::Debug::Result::failure("bad", "failure").toResponseLine()));
}
} // namespace

int main()
{
    testCommandParsing();
    testJsonResponseRoundTrip();
    testResponseRecognitionRejectsMalformedInput();

    if (failures != 0)
    {
        std::cerr << failures << " debug protocol test(s) failed.\n";
        return 1;
    }

    std::cout << "All debug protocol tests passed.\n";
    return 0;
}

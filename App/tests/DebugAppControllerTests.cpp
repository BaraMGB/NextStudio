#include "DebugAppController.h"
#include "DebugProtocol.h"

#include <iostream>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do { if (! (condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

struct TempDirectory
{
    TempDirectory()
        : file(juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getNonexistentChildFile("nextstudio-controller-test", {}, false))
    {
        file.createDirectory();
    }
    ~TempDirectory() { file.deleteRecursively(); }
    juce::File file;
};

class FakeHost final : public NextStudio::Debug::DebugHost
{
public:
    explicit FakeHost(const juce::File &root)
        : state(root.getChildFile("settings/AppSettings.xml")), artifacts(root.getChildFile("artifacts"))
    {
        artifacts.createDirectory();
    }

    bool isDebugMode() const override { return true; }
    const ApplicationViewState &getApplicationState() const override { return state; }
    tracktion_engine::Edit *getCurrentEdit() const override { return nullptr; }
    EditViewState *getEditViewState() const override { return nullptr; }
    bool hasEditComponent() const override { return false; }
    bool hasHeaderComponent() const override { return false; }
    bool hasLowerRangeComponent() const override { return false; }
    juce::Rectangle<int> getScreenBounds() const override { return {0, 0, 100, 80}; }
    juce::Rectangle<int> getLocalBounds() const override { return {0, 0, 100, 80}; }
    juce::Image createSnapshot(const juce::Rectangle<int> &, float) const override { return {}; }
    juce::File getDebugArtifactsDirectory() const override { return artifacts; }
    juce::File writeStateDump() const override { return stateDumpResult; }
    juce::File captureSnapshot(int maxWidth) const override
    {
        lastSnapshotWidth = maxWidth;
        return screenshotResult;
    }
    bool play() override { return playResult; }
    bool stop() override { return stopResult; }
    tracktion_engine::AudioTrack *createAudioTrack(bool, const juce::String &) override { return nullptr; }
    void requestQuit() override { quitRequested = true; }

    ApplicationViewState state;
    juce::File artifacts;
    mutable juce::File stateDumpResult;
    mutable juce::File screenshotResult;
    mutable int lastSnapshotWidth = 0;
    bool playResult = false;
    bool stopResult = false;
    bool quitRequested = false;
};

NextStudio::Debug::Result execute(FakeHost &host, const juce::String &line)
{
    NextStudio::Debug::DebugAppController controller(host);
    return controller.execute(NextStudio::Debug::parseCommandLine(line));
}

void testValidationAndErrors()
{
    const TempDirectory temp;
    FakeHost host(temp.file);

    REQUIRE(execute(host, "{not-json}").code == "invalid-request");
    REQUIRE(execute(host, "ping trailing").code == "invalid-argument");
    REQUIRE(execute(host, "screenshot 12px").code == "invalid-argument");
    REQUIRE(execute(host, "screenshot 0").code == "invalid-argument");
    REQUIRE(execute(host, "screenshot 8193").code == "invalid-argument");
    REQUIRE(execute(host, R"({"command":"ensure-track","arguments":{"type":"other","name":"x"}})").code == "invalid-argument");
    REQUIRE(execute(host, R"({"command":"select-track","arguments":{"trackId":"missing"}})").code == "not-ready");
    REQUIRE(execute(host, "transport-state").code == "not-ready");
    REQUIRE(execute(host, "play").code == "not-ready");
}

void testArtifactSuccessAndFailure()
{
    const TempDirectory temp;
    FakeHost host(temp.file);

    REQUIRE(execute(host, "screenshot").code == "io-error");
    const auto screenshot = temp.file.getChildFile("screenshot.png");
    screenshot.replaceWithText("non-empty test artifact");
    host.screenshotResult = screenshot;
    const auto screenshotResult = execute(host, "screenshot 800");
    REQUIRE(screenshotResult.ok);
    REQUIRE(host.lastSnapshotWidth == 800);
    REQUIRE(screenshotResult.fields["path"] == screenshot.getFullPathName());

    REQUIRE(execute(host, "state-dump").code == "io-error");
    const auto dump = temp.file.getChildFile("state.json");
    dump.replaceWithText("{}");
    host.stateDumpResult = dump;
    REQUIRE(execute(host, "state-dump").ok);
}

void testQuitAndJsonResponse()
{
    const TempDirectory temp;
    FakeHost host(temp.file);
    const auto result = execute(host, "quit");
    REQUIRE(result.ok);
    REQUIRE(host.quitRequested);
    const auto response = juce::JSON::parse(result.toResponseLine());
    REQUIRE(response.getProperty("status", {}).toString() == "ok");
    REQUIRE(response.getProperty("fields", {}).getProperty("quitting", {}).toString() == "true");
}
} // namespace

int main()
{
    testValidationAndErrors();
    testArtifactSuccessAndFailure();
    testQuitAndJsonResponse();

    if (failures != 0)
    {
        std::cerr << failures << " controller test(s) failed.\n";
        return 1;
    }
    std::cout << "All debug app controller tests passed.\n";
    return 0;
}

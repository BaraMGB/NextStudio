#include "ApplicationViewState.h"

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
                   .getNonexistentChildFile("nextstudio-settings-test", {}, false))
    {
        file.createDirectory();
    }
    ~TempDirectory() { file.deleteRecursively(); }
    juce::File file;
};

void testExplicitSettingsFileIsUsedExclusively()
{
    const TempDirectory temp;
    const auto settings = temp.file.getChildFile("session/settings/AppSettings.xml");

    {
        ApplicationViewState state(settings);
        REQUIRE(state.getSettingsFile() == settings);
        state.m_setupComplete = true;
        state.setRootFolder(temp.file.getChildFile("workspace"));
        state.saveState();
    }

    REQUIRE(settings.existsAsFile());
    REQUIRE(settings.getSize() > 0);
    juce::XmlDocument document(settings);
    const auto xml = document.getDocumentElement();
    REQUIRE(xml != nullptr);
    if (xml != nullptr)
        REQUIRE(xml->hasTagName("AppSettings"));
}
} // namespace

int main()
{
    testExplicitSettingsFileIsUsedExclusively();
    if (failures != 0)
    {
        std::cerr << failures << " settings isolation test(s) failed.\n";
        return 1;
    }
    std::cout << "All settings isolation tests passed.\n";
    return 0;
}

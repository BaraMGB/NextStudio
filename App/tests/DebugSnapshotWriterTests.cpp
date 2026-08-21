#include "DebugSnapshotWriter.h"

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
                   .getNonexistentChildFile("nextstudio-snapshot-test", {}, false))
    {
        file.createDirectory();
    }
    ~TempDirectory() { file.deleteRecursively(); }
    juce::File file;
};

void testRejectsInvalidImage()
{
    const TempDirectory temp;
    const auto output = temp.file.getChildFile("invalid.png");
    REQUIRE(!NextStudio::Debug::writeValidatedPng({}, output));
    REQUIRE(!output.existsAsFile());
}

void testWritesReadablePng()
{
    const TempDirectory temp;
    const auto output = temp.file.getChildFile("valid.png");
    juce::Image image(juce::Image::ARGB, 32, 24, true);
    juce::Graphics graphics(image);
    graphics.fillAll(juce::Colours::cornflowerblue);

    REQUIRE(NextStudio::Debug::writeValidatedPng(image, output));
    REQUIRE(output.existsAsFile());
    REQUIRE(output.getSize() > 0);
    const auto decoded = juce::ImageFileFormat::loadFrom(output);
    REQUIRE(decoded.isValid());
    REQUIRE(decoded.getWidth() == 32);
    REQUIRE(decoded.getHeight() == 24);
}

void testRejectsUnopenableDestination()
{
    const TempDirectory temp;
    juce::Image image(juce::Image::ARGB, 2, 2, true);
    REQUIRE(!NextStudio::Debug::writeValidatedPng(image, temp.file));
}
} // namespace

int main()
{
    testRejectsInvalidImage();
    testWritesReadablePng();
    testRejectsUnopenableDestination();

    if (failures != 0)
    {
        std::cerr << failures << " snapshot writer test(s) failed.\n";
        return 1;
    }

    std::cout << "All snapshot writer tests passed.\n";
    return 0;
}

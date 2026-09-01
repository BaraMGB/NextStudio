/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ProjectLifecycle.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

#define REQUIRE_EQ(actual, expected) \
    do { if ((actual) != (expected)) { std::cerr << "FAIL: " << #actual << " == " << #expected << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

class ScopedTestDirectory
{
public:
    ScopedTestDirectory()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("NextStudioProjectLifecycleTests", {}, false))
    {
        REQUIRE(directory.createDirectory());
    }

    ~ScopedTestDirectory() { directory.deleteRecursively(); }

    juce::File file(const juce::String &name) const { return directory.getChildFile(name); }

private:
    juce::File directory;
};

void testProjectExtensionHandling()
{
    using namespace ProjectLifecycle;
    const juce::File root("/tmp");

    REQUIRE_EQ(withProjectExtension(root.getChildFile("Song")).getFileName(), juce::String("Song.tracktionedit"));
    REQUIRE_EQ(withProjectExtension(root.getChildFile("Song.txt")).getFileName(), juce::String("Song.tracktionedit"));
    REQUIRE_EQ(withProjectExtension(root.getChildFile("Song.tracktionedit")).getFileName(), juce::String("Song.tracktionedit"));
    REQUIRE_EQ(withProjectExtension(root.getChildFile("Song.tracktionedit.tracktionedit")).getFileName(), juce::String("Song.tracktionedit"));
    REQUIRE_EQ(withProjectExtension(root.getChildFile("Song.TRACKTIONEDIT")).getFileName(), juce::String("Song.tracktionedit"));
    const auto upperCaseProject = root.getChildFile("Song.TRACKTIONEDIT");
    REQUIRE_EQ(normaliseSaveTarget(upperCaseProject, upperCaseProject), upperCaseProject);
    REQUIRE_EQ(normaliseSaveTarget(root.getChildFile("New Song"), upperCaseProject).getFileName(), juce::String("New Song.tracktionedit"));
    REQUIRE_EQ(projectNameWithoutExtension(" Song.tracktionedit.TRACKTIONEDIT "), juce::String("Song"));
    REQUIRE(isValidProjectName("Song"));
    REQUIRE(isValidProjectName("My Song.tracktionedit"));
    REQUIRE(!isValidProjectName(""));
    REQUIRE(!isValidProjectName("  "));
    REQUIRE(!isValidProjectName("../Song"));
    REQUIRE(!isValidProjectName("Song?"));
    REQUIRE(!isValidProjectName("Song."));
    const auto persistentProject = root.getChildFile("Song.TRACKTIONEDIT");
    const auto temporaryProject = root.getChildFile("Song.nextTemp");
    REQUIRE(isPersistentProjectFile(persistentProject));
    REQUIRE(!isPersistentProjectFile(temporaryProject));
    REQUIRE(!shouldChooseSaveTarget(persistentProject, false));
    REQUIRE(shouldChooseSaveTarget(persistentProject, true));
    REQUIRE(shouldChooseSaveTarget(temporaryProject, false));

    ScopedTestDirectory testDirectory;
    const auto validTarget = withProjectExtension(testDirectory.file("New Song"));
    REQUIRE(isValidProjectTarget(validTarget));
    REQUIRE(!isValidProjectTarget(testDirectory.file("New Song.txt")));
    REQUIRE(!isValidProjectTarget(testDirectory.file("Bad?.tracktionedit")));

    const auto browserDirectory = testDirectory.file("Subfolder");
    REQUIRE(browserDirectory.createDirectory());
    REQUIRE(isProjectBrowserEntry(browserDirectory));
    REQUIRE(isProjectBrowserEntry(testDirectory.file("Song.TRACKTIONEDIT")));
    REQUIRE(!isProjectBrowserEntry(testDirectory.file("Song.wav")));
}

void testLoadFileInspection()
{
    using namespace ProjectLifecycle;
    ScopedTestDirectory testDirectory;

    REQUIRE_EQ(inspectLoadFile(testDirectory.file("Missing.tracktionedit"), false), LoadFileStatus::missing);

    const auto unsupported = testDirectory.file("Project.txt");
    REQUIRE(unsupported.replaceWithText("<EDIT/>"));
    REQUIRE_EQ(inspectLoadFile(unsupported, false), LoadFileStatus::unsupportedExtension);

    const auto empty = testDirectory.file("Empty.tracktionedit");
    REQUIRE(empty.create());
    REQUIRE_EQ(inspectLoadFile(empty, false), LoadFileStatus::empty);

    const auto corrupt = testDirectory.file("Corrupt.tracktionedit");
    REQUIRE(corrupt.replaceWithText("not a project"));
    REQUIRE_EQ(inspectLoadFile(corrupt, false), LoadFileStatus::invalidData);

    const auto wrongRoot = testDirectory.file("WrongRoot.tracktionedit");
    REQUIRE(wrongRoot.replaceWithText("<NOT_AN_EDIT/>"));
    REQUIRE_EQ(inspectLoadFile(wrongRoot, false), LoadFileStatus::invalidData);

    const auto xmlProject = testDirectory.file("Xml.tracktionedit");
    REQUIRE(xmlProject.replaceWithText("<?xml version=\"1.0\"?><EDIT name=\"Test\"/>"));
    REQUIRE_EQ(inspectLoadFile(xmlProject, false), LoadFileStatus::valid);

    const auto binaryProject = testDirectory.file("Binary.tracktionedit");
    {
        juce::FileOutputStream output(binaryProject);
        REQUIRE(output.openedOk());
        juce::ValueTree(juce::Identifier("EDIT")).writeToStream(output);
        output.flush();
    }
    REQUIRE_EQ(inspectLoadFile(binaryProject, false), LoadFileStatus::valid);

    const auto recovery = testDirectory.file("autosave.nextTemp");
    {
        juce::FileOutputStream output(recovery);
        REQUIRE(output.openedOk());
        juce::ValueTree(juce::Identifier("EDIT")).writeToStream(output);
        output.flush();
    }
    REQUIRE_EQ(inspectLoadFile(recovery, false), LoadFileStatus::unsupportedExtension);
    REQUIRE_EQ(inspectLoadFile(recovery, true), LoadFileStatus::valid);
}
} // namespace

int main()
{
    testProjectExtensionHandling();
    testLoadFileInspection();

    if (failures != 0)
    {
        std::cerr << failures << " project lifecycle test(s) failed.\n";
        return 1;
    }

    std::cout << "All project lifecycle tests passed.\n";
    return 0;
}

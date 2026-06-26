#include "InitialContentSetup.h"
#include "Samples707.h"
#include "Samples808.h"
#include "Samples909.h"
#include "ThemePresets.h"
#include <array>

namespace
{
bool ensureDirectory(const juce::File &directory, juce::StringArray &errors)
{
    if (directory.existsAsFile())
    {
        errors.add("Path exists as a file: " + directory.getFullPathName());
        return false;
    }

    if (directory.exists() && directory.isDirectory())
        return true;

    if (!directory.createDirectory() && !(directory.exists() && directory.isDirectory()))
    {
        errors.add("Unable to create directory: " + directory.getFullPathName());
        return false;
    }

    return true;
}

bool ensureWritable(const juce::File &directory, juce::StringArray &errors)
{
    const auto probeFile = directory.getNonexistentChildFile(".nextstudio_write_test_", ".tmp", false);
    if (!probeFile.replaceWithText("write-test"))
    {
        errors.add("No write access to directory: " + directory.getFullPathName());
        return false;
    }

    if (!probeFile.deleteFile() && probeFile.existsAsFile())
    {
        errors.add("Unable to clean up write-test file in: " + directory.getFullPathName());
        return false;
    }

    return true;
}

bool ensureContentLayout(const juce::File &root, juce::StringArray &errors)
{
    if (!ensureDirectory(root, errors))
        return false;

    static constexpr std::array<const char *, 5> requiredDirs{"Presets", "Clips", "Renders", "Samples", "Projects"};
    for (const auto *name : requiredDirs)
    {
        if (!ensureDirectory(root.getChildFile(name), errors))
            return false;
    }

    if (!ensureDirectory(root.getChildFile("Presets").getChildFile("Themes"), errors))
        return false;

    if (!ensureWritable(root, errors))
        return false;

    for (const auto *name : requiredDirs)
    {
        if (!ensureWritable(root.getChildFile(name), errors))
            return false;
    }

    if (!ensureWritable(root.getChildFile("Presets").getChildFile("Themes"), errors))
        return false;

    return true;
}

template <typename ResourceGetter>
void extractResourcesIfNeeded(const juce::File &targetDir, const char *const *resourceList, const char *const *filenames, int size, ResourceGetter getResourceFn)
{
    juce::StringArray errors;
    if (!ensureDirectory(targetDir, errors))
        return;

    for (int i = 0; i < size; ++i)
    {
        const auto destinationFile = targetDir.getChildFile(juce::File(filenames[i]).getFileName());
        if (destinationFile.existsAsFile())
            continue;

        int dataSize = 0;
        if (const auto *data = getResourceFn(resourceList[i], dataSize))
            destinationFile.replaceWithData(data, dataSize);
    }
}
} // namespace

namespace InitialContentSetup
{
bool validateAndPrepareRoot(const juce::File &root, juce::String &errorMessage)
{
    juce::StringArray errors;
    if (!ensureContentLayout(root, errors))
    {
        errorMessage = errors.joinIntoString("\n");
        return false;
    }

    errorMessage.clear();
    return true;
}

void populateBundledContent(const juce::File &root)
{
    juce::String ignoredError;
    if (!validateAndPrepareRoot(root, ignoredError))
        return;

    extractResourcesIfNeeded(root.getChildFile("Presets").getChildFile("Themes"), ThemePresets::namedResourceList, ThemePresets::originalFilenames, ThemePresets::namedResourceListSize, ThemePresets::getNamedResource);
    extractResourcesIfNeeded(root.getChildFile("Samples").getChildFile("707"), Samples707::namedResourceList, Samples707::originalFilenames, Samples707::namedResourceListSize, Samples707::getNamedResource);
    extractResourcesIfNeeded(root.getChildFile("Samples").getChildFile("808"), Samples808::namedResourceList, Samples808::originalFilenames, Samples808::namedResourceListSize, Samples808::getNamedResource);
    extractResourcesIfNeeded(root.getChildFile("Samples").getChildFile("909"), Samples909::namedResourceList, Samples909::originalFilenames, Samples909::namedResourceListSize, Samples909::getNamedResource);
}
} // namespace InitialContentSetup

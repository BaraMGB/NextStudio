#include "ThemePresetModel.h"
#include "ApplicationViewState.h"

#include <algorithm>

ThemePresetModel::ThemePresetModel(juce::File directory, juce::StringArray builtInNames)
    : m_directory(std::move(directory)),
      m_builtInNames(std::move(builtInNames))
{
}

std::vector<ThemePresetModel::Preset> ThemePresetModel::getPresets() const
{
    juce::Array<juce::File> files;
    m_directory.findChildFiles(files, juce::File::findFiles, false, "*.nxttheme");

    std::vector<Preset> result;
    result.reserve((size_t)files.size());

    for (const auto &file : files)
    {
        auto state = loadThemeState(file);
        if (!state.isValid())
            continue;

        const auto name = file.getFileNameWithoutExtension();
        result.push_back({name, file, state, isBuiltInName(name)});
    }

    std::sort(result.begin(), result.end(), [this](const Preset &a, const Preset &b)
              {
                  const auto aBuiltInIndex = m_builtInNames.indexOf(a.name, true);
                  const auto bBuiltInIndex = m_builtInNames.indexOf(b.name, true);

                  if (aBuiltInIndex >= 0 || bBuiltInIndex >= 0)
                  {
                      if (aBuiltInIndex < 0)
                          return false;
                      if (bBuiltInIndex < 0)
                          return true;
                      return aBuiltInIndex < bBuiltInIndex;
                  }

                  return a.name.compareNatural(b.name, true) < 0;
              });

    return result;
}

juce::Result ThemePresetModel::savePreset(const juce::String &name, const juce::ValueTree &state, bool overwrite, Preset *savedPreset) const
{
    const auto trimmedName = name.trim();
    if (auto validation = validateName(trimmedName); validation.failed())
        return validation;

    if (isBuiltInName(trimmedName))
        return juce::Result::fail("Built-in themes cannot be overwritten.");

    if (!state.isValid() || !state.hasType(IDs::ThemeState))
        return juce::Result::fail("The current theme data is invalid.");

    if (!m_directory.createDirectory() && !m_directory.isDirectory())
        return juce::Result::fail("The theme folder could not be created.");

    auto target = findFileForName(trimmedName);
    if (target == juce::File())
        target = m_directory.getChildFile(trimmedName).withFileExtension(".nxttheme");
    else if (!overwrite)
        return juce::Result::fail("A theme with this name already exists.");

    auto normalisedState = state.createCopy();
    ApplicationViewState::normaliseThemeState(normalisedState);
    auto xml = std::unique_ptr<juce::XmlElement>(normalisedState.createXml());
    if (xml == nullptr)
        return juce::Result::fail("The theme could not be serialised.");

    juce::TemporaryFile temporary(target);
    if (auto output = std::unique_ptr<juce::FileOutputStream>(temporary.getFile().createOutputStream()))
    {
        xml->writeTo(*output, {});
        output->flush();

        if (output->getStatus().failed())
            return output->getStatus();
    }
    else
    {
        return juce::Result::fail("The temporary theme file could not be opened.");
    }

    if (!temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail("The theme file could not be replaced.");

    if (savedPreset != nullptr)
        *savedPreset = {target.getFileNameWithoutExtension(), target, normalisedState, false};

    return juce::Result::ok();
}

juce::Result ThemePresetModel::validateName(const juce::String &name)
{
    const auto trimmed = name.trim();
    if (trimmed.isEmpty())
        return juce::Result::fail("Enter a theme name.");

    if (trimmed != name)
        return juce::Result::fail("Theme names cannot begin or end with spaces.");

    if (juce::File::createLegalFileName(trimmed) != trimmed || trimmed.containsAnyOf("/\\"))
        return juce::Result::fail("The theme name contains invalid filename characters.");

    return juce::Result::ok();
}

bool ThemePresetModel::areStatesEquivalent(const juce::ValueTree &a, const juce::ValueTree &b)
{
    if (!a.isValid() || !b.isValid() || !a.hasType(IDs::ThemeState) || !b.hasType(IDs::ThemeState))
        return false;

    if (a.getNumProperties() != b.getNumProperties())
        return false;

    for (int i = 0; i < a.getNumProperties(); ++i)
    {
        const auto property = a.getPropertyName(i);
        if (a[property] != b[property])
            return false;
    }

    return true;
}

bool ThemePresetModel::isBuiltInName(const juce::String &name) const
{
    return m_builtInNames.contains(name, true);
}

juce::File ThemePresetModel::findFileForName(const juce::String &name) const
{
    juce::Array<juce::File> files;
    m_directory.findChildFiles(files, juce::File::findFiles, false, "*.nxttheme");
    for (const auto &file : files)
        if (file.getFileNameWithoutExtension().equalsIgnoreCase(name))
            return file;

    return {};
}

juce::ValueTree ThemePresetModel::loadThemeState(const juce::File &file)
{
    if (auto xml = juce::XmlDocument::parse(file))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(IDs::ThemeState))
        {
            ApplicationViewState::normaliseThemeState(state);
            return state;
        }
    }

    return {};
}

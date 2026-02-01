/*
  ==============================================================================

    PresetHelpers.cpp
    Created: 24 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Utilities/Utilities.h" // For GUIHelpers logging if needed

namespace PresetHelpers
{

juce::String getPluginPresetFolder(te::Plugin &plugin)
{
    if (auto *ep = dynamic_cast<te::ExternalPlugin *>(&plugin))
    {
        if (ep->desc.manufacturerName.isNotEmpty())
            return juce::File::createLegalFileName(ep->desc.manufacturerName);
        return "External";
    }

    auto type = plugin.getPluginType();
    if (type == "volume")
        return "Volume";
    if (type == "4bandEq")
        return "EQ";
    if (type == "delay")
        return "Delay";
    if (type == "lowpass")
        return "Filter";
    if (type == "4osc")
        return "FourOSC";

    // Default fallback
    return "Misc";
}

juce::File getPresetDirectory(PluginPresetInterface &interface)
{
    // Access the global preset directory from ApplicationViewState via the interface
    auto userPresetDir = juce::File(interface.getApplicationViewState().m_presetDir.get());
    return userPresetDir.getChildFile(interface.getPresetSubfolder());
}

bool tryLoadInitPreset(PluginPresetInterface &interface)
{
    auto presetDir = getPresetDirectory(interface);
    auto presetFile = presetDir.getChildFile("init.nxtpreset");

    if (presetFile.existsAsFile())
    {
        // Use XmlDocument::parse for robust XML parsing
        if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(presetFile)))
        {
            juce::ValueTree presetState = juce::ValueTree::fromXml(*xml);

            // Validate that the preset matches the plugin type
            if (presetState.hasType(juce::Identifier("PLUGIN")) && presetState.getProperty("type") == interface.getPluginTypeName())
            {
                // Apply the state
                interface.restorePluginState(presetState);

                // Update interface metadata
                interface.setInitialPresetLoaded(true);
                interface.setLastLoadedPresetName("init");

                return true;
            }
            else
            {
                GUIHelpers::log("Error loading init preset: Type mismatch. Expected " + interface.getPluginTypeName());
            }
        }
        else
        {
            GUIHelpers::log("Error parsing init preset XML for " + interface.getPluginTypeName());
        }
    }
    return false;
}

} // namespace PresetHelpers
#pragma once

namespace juce
{
class Component;
}

namespace PluginWindowPinning
{
// Changes the X11/EWMH stacking state of the existing native peer. This must not
// recreate the peer because external plugin editors may own native child windows.
bool setLinuxAlwaysOnTop(juce::Component &component, bool pinned);
} // namespace PluginWindowPinning

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio
{
/** Detects Wine on Windows and keeps JUCE desktop windows on the legacy
    software renderer. JUCE 8 defaults to Direct2D, which currently requires
    Direct2D/DirectComposition functionality that stock Wine does not provide.
*/
class WineRendererFallback final
    : private juce::FocusChangeListener
    , private juce::AsyncUpdater
{
public:
    void start();
    void stop();

    bool isActive() const noexcept { return active; }
    void applyTo(juce::Component &component);

private:
    void globalFocusChanged(juce::Component *focusedComponent) override;
    void handleAsyncUpdate() override;
    void applyToDesktopComponents();

    bool active{false};
    bool missingSoftwareRendererLogged{false};
};
} // namespace NextStudio

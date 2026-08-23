#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio
{
/** Keeps JUCE desktop windows on the legacy software renderer in Windows
    environments where the Direct2D presentation path is unreliable, currently
    Wine and Remote Desktop sessions.
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

    static void configureFontFallback(juce::LookAndFeel &lookAndFeel);

private:
    void globalFocusChanged(juce::Component *focusedComponent) override;
    void handleAsyncUpdate() override;
    void applyToDesktopComponents();

    bool active{false};
    bool missingSoftwareRendererLogged{false};
    bool availableRenderersLogged{false};
};
} // namespace NextStudio

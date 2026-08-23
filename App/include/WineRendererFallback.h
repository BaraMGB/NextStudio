#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace NextStudio
{
/** Keeps JUCE desktop windows on the legacy software renderer when running
    under Wine, where the Direct2D presentation path is unreliable.
*/
class WineRendererFallback final
    : private juce::FocusChangeListener
    , private juce::AsyncUpdater
    , private juce::Timer
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
    void timerCallback() override;
    void applyToDesktopComponents();
    void flushPendingSoftwareRepaints();

    bool active{false};
    bool softwareRepaintTimerRequired{false};
    bool listeningForFocusChanges{false};
    bool missingSoftwareRendererLogged{false};
    bool softwareRendererEnabledLogged{false};
};
} // namespace NextStudio

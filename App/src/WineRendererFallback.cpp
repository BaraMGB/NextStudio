#include "WineRendererFallback.h"

#include "Logging.h"

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace NextStudio
{
namespace
{
bool isRunningUnderWine()
{
#if JUCE_WINDOWS
    static const auto result = []
    {
        juce::DynamicLibrary ntdll("ntdll.dll");
        return ntdll.getFunction("wine_get_version") != nullptr;
    }();
    return result;
#else
    return false;
#endif
}

bool isRemoteDesktopSession()
{
#if JUCE_WINDOWS
    return GetSystemMetrics(SM_REMOTESESSION) != 0 || GetSystemMetrics(SM_REMOTECONTROL) != 0;
#else
    return false;
#endif
}

bool isSoftwareRendererForcedByEnvironment()
{
    const auto value = juce::SystemStats::getEnvironmentVariable("NEXTSTUDIO_FORCE_SOFTWARE_RENDERER", {}).trim();
    return value == "1" || value.equalsIgnoreCase("true") || value.equalsIgnoreCase("yes");
}
} // namespace

void WineRendererFallback::start()
{
    const auto wine = isRunningUnderWine();
    const auto remoteDesktop = isRemoteDesktopSession();
    const auto forcedByEnvironment = isSoftwareRendererForcedByEnvironment();
    active = wine || remoteDesktop || forcedByEnvironment;

    if (!active)
        return;

    juce::StringArray reasons;
    if (wine)
        reasons.add("Wine");
    if (remoteDesktop)
        reasons.add("Remote Desktop");
    if (forcedByEnvironment)
        reasons.add("NEXTSTUDIO_FORCE_SOFTWARE_RENDERER");

    NS_LOG_INFO(app, "Forcing JUCE Software Renderer for desktop windows (" + reasons.joinIntoString(", ") + ")");
    NS_LOG_INFO(app, "Renderer fallback startup completed without desktop access");
}

void WineRendererFallback::stop()
{
    if (listeningForFocusChanges)
        juce::Desktop::getInstance().removeFocusChangeListener(this);

    cancelPendingUpdate();
    listeningForFocusChanges = false;
    active = false;
}

void WineRendererFallback::configureFontFallback(juce::LookAndFeel &lookAndFeel)
{
    if (!isRunningUnderWine())
        return;

    const auto typefaceNames = juce::Font::findAllTypefaceNames();

    for (const auto *candidate : {"Tahoma", "Arial", "Liberation Sans", "DejaVu Sans"})
    {
        if (!typefaceNames.contains(candidate, true))
            continue;

        lookAndFeel.setDefaultSansSerifTypefaceName(candidate);
        NS_LOG_INFO(app, "Wine font fallback configured: " + juce::String(candidate));
        return;
    }

    NS_LOG_WARN(app, "Wine font fallback could not find a suitable sans-serif typeface");
}

void WineRendererFallback::applyTo(juce::Component &component)
{
    if (!active)
        return;

    NS_LOG_INFO(app, "Renderer fallback applying to window: " + component.getName());

    if (!listeningForFocusChanges)
    {
        NS_LOG_INFO(app, "Registering renderer fallback focus monitoring");
        juce::Desktop::getInstance().addFocusChangeListener(this);
        listeningForFocusChanges = true;
        NS_LOG_INFO(app, "Renderer fallback focus monitoring started");
    }

    auto *peer = component.getPeer();
    if (peer == nullptr)
    {
        NS_LOG_WARN(app, "Renderer fallback found no peer for window: " + component.getName());
        return;
    }

    NS_LOG_INFO(app, "Querying available renderers for window: " + component.getName());
    const auto engines = peer->getAvailableRenderingEngines();

    if (!availableRenderersLogged)
    {
        availableRenderersLogged = true;
        NS_LOG_INFO(app, "Available JUCE rendering engines:");

        for (const auto &engine : engines)
            NS_LOG_INFO(app, "  " + engine);
    }

    const auto softwareRenderer = engines.indexOf("Software Renderer");

    if (softwareRenderer < 0)
    {
        if (!missingSoftwareRendererLogged)
        {
            missingSoftwareRendererLogged = true;
            NS_LOG_WARN(app, "Renderer fallback could not find JUCE Software Renderer");
        }
        return;
    }

    const auto currentRenderer = peer->getCurrentRenderingEngine();
    NS_LOG_INFO(app, "Current renderer index for " + component.getName() + ": " + juce::String(currentRenderer));

    if (currentRenderer != softwareRenderer)
    {
        NS_LOG_INFO(app, "Switching renderer for " + component.getName() + " to index " + juce::String(softwareRenderer));
        peer->setCurrentRenderingEngine(softwareRenderer);
        component.repaint();
        NS_LOG_INFO(app, "JUCE Software Renderer enabled for window: " + component.getName());
    }
    else
    {
        NS_LOG_INFO(app, "JUCE Software Renderer already active for window: " + component.getName());
    }
}

void WineRendererFallback::globalFocusChanged(juce::Component *) { triggerAsyncUpdate(); }

void WineRendererFallback::handleAsyncUpdate() { applyToDesktopComponents(); }

void WineRendererFallback::applyToDesktopComponents()
{
    auto &desktop = juce::Desktop::getInstance();

    for (int i = 0; i < desktop.getNumComponents(); ++i)
        if (auto *component = desktop.getComponent(i))
            applyTo(*component);
}
} // namespace NextStudio

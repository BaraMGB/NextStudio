#include "WineRendererFallback.h"

#include "Logging.h"

namespace NextStudio
{
namespace
{
bool isRunningUnderWine()
{
#if JUCE_WINDOWS
    juce::DynamicLibrary ntdll("ntdll.dll");
    return ntdll.getFunction("wine_get_version") != nullptr;
#else
    return false;
#endif
}
} // namespace

void WineRendererFallback::start()
{
    active = isRunningUnderWine();

    if (!active)
        return;

    NS_LOG_INFO(app, "Wine detected; forcing JUCE Software Renderer for desktop windows");
    juce::Desktop::getInstance().addFocusChangeListener(this);
    applyToDesktopComponents();
}

void WineRendererFallback::stop()
{
    if (active)
        juce::Desktop::getInstance().removeFocusChangeListener(this);

    cancelPendingUpdate();
    active = false;
}

void WineRendererFallback::applyTo(juce::Component &component)
{
    if (!active)
        return;

    auto *peer = component.getPeer();
    if (peer == nullptr)
        return;

    const auto engines = peer->getAvailableRenderingEngines();
    const auto softwareRenderer = engines.indexOf("Software Renderer");

    if (softwareRenderer < 0)
    {
        if (!missingSoftwareRendererLogged)
        {
            missingSoftwareRendererLogged = true;
            NS_LOG_WARN(app, "Wine renderer fallback could not find JUCE Software Renderer");
        }
        return;
    }

    if (peer->getCurrentRenderingEngine() != softwareRenderer)
    {
        peer->setCurrentRenderingEngine(softwareRenderer);
        NS_LOG_INFO(app, "JUCE Software Renderer enabled for window: " + component.getName());
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

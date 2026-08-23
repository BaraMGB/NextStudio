#include "WineRendererFallback.h"

#include "Logging.h"

#if JUCE_WINDOWS
#include <cstring>
#include <windows.h>
#endif

namespace NextStudio
{
namespace
{
#if JUCE_WINDOWS
HRESULT WINAPI unavailableCreateDxgiFactory2(UINT, REFIID, void **factory)
{
    if (factory != nullptr)
        *factory = nullptr;

    return E_NOTIMPL;
}

bool installWineDxgiFactoryGuard()
{
    const auto module = GetModuleHandleW(nullptr);
    if (module == nullptr)
        return false;

    auto *base = reinterpret_cast<unsigned char *>(module);
    const auto *dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    const auto *ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return false;

    const auto &importsDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importsDirectory.VirtualAddress == 0)
        return false;

    auto *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + importsDirectory.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor)
    {
        const auto *libraryName = reinterpret_cast<const char *>(base + descriptor->Name);
        if (_stricmp(libraryName, "dxgi.dll") != 0 || descriptor->OriginalFirstThunk == 0)
            continue;

        auto *lookup = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->OriginalFirstThunk);
        auto *address = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->FirstThunk);

        for (; lookup->u1.AddressOfData != 0; ++lookup, ++address)
        {
            if (IMAGE_SNAP_BY_ORDINAL(lookup->u1.Ordinal))
                continue;

            const auto *import = reinterpret_cast<const IMAGE_IMPORT_BY_NAME *>(base + lookup->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(import->Name), "CreateDXGIFactory2") != 0)
                continue;

            DWORD oldProtection = 0;
            if (VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), PAGE_READWRITE, &oldProtection) == FALSE)
                return false;

            address->u1.Function = reinterpret_cast<ULONG_PTR>(&unavailableCreateDxgiFactory2);

            DWORD ignored = 0;
            VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), oldProtection, &ignored);
            FlushInstructionCache(GetCurrentProcess(), &address->u1.Function, sizeof(address->u1.Function));
            return true;
        }
    }

    return false;
}
#endif

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

bool isEnabledEnvironmentValue(const juce::String &name)
{
    const auto value = juce::SystemStats::getEnvironmentVariable(name, {}).trim();
    return value == "1" || value.equalsIgnoreCase("true") || value.equalsIgnoreCase("yes");
}

bool isSoftwareRendererForcedByEnvironment()
{
    return isEnabledEnvironmentValue("NEXTSTUDIO_FORCE_SOFTWARE_RENDERER");
}

bool isDefaultRendererForcedByEnvironment()
{
    return isEnabledEnvironmentValue("NEXTSTUDIO_FORCE_DEFAULT_RENDERER");
}
} // namespace

void WineRendererFallback::start()
{
    const auto wine = isRunningUnderWine();
    const auto forcedByEnvironment = isSoftwareRendererForcedByEnvironment();
    const auto defaultRendererForced = isDefaultRendererForcedByEnvironment();
    active = !defaultRendererForced && (wine || forcedByEnvironment);

    if (defaultRendererForced)
    {
        NS_LOG_INFO(app, "Keeping JUCE default renderer (NEXTSTUDIO_FORCE_DEFAULT_RENDERER)");
        return;
    }

#if JUCE_WINDOWS
    if (wine)
    {
        softwareRepaintTimerRequired = installWineDxgiFactoryGuard();
        NS_LOG_DEBUG(app, softwareRepaintTimerRequired ? "Wine DXGI factory guard installed" : "Wine DXGI factory guard was not installed");
    }
#endif

    if (!active)
        return;

    juce::StringArray reasons;
    if (wine)
        reasons.add("Wine");
    if (forcedByEnvironment)
        reasons.add("NEXTSTUDIO_FORCE_SOFTWARE_RENDERER");

    NS_LOG_INFO(app, "Forcing JUCE Software Renderer for desktop windows (" + reasons.joinIntoString(", ") + ")");
}

void WineRendererFallback::stop()
{
    stopTimer();

    if (listeningForFocusChanges)
        juce::Desktop::getInstance().removeFocusChangeListener(this);

    cancelPendingUpdate();
    softwareRepaintTimerRequired = false;
    listeningForFocusChanges = false;
    missingSoftwareRendererLogged = false;
    softwareRendererEnabledLogged = false;
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

    if (softwareRepaintTimerRequired && !isTimerRunning())
        startTimerHz(60);

    if (!listeningForFocusChanges)
    {
        juce::Desktop::getInstance().addFocusChangeListener(this);
        listeningForFocusChanges = true;
    }

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
            NS_LOG_WARN(app, "Renderer fallback could not find JUCE Software Renderer");
        }
        return;
    }

    if (peer->getCurrentRenderingEngine() != softwareRenderer)
    {
        peer->setCurrentRenderingEngine(softwareRenderer);
        component.repaint();
    }

    if (!softwareRendererEnabledLogged)
    {
        softwareRendererEnabledLogged = true;
        NS_LOG_INFO(app, "JUCE Software Renderer enabled for desktop windows");
    }
}

void WineRendererFallback::globalFocusChanged(juce::Component *) { triggerAsyncUpdate(); }

void WineRendererFallback::handleAsyncUpdate() { applyToDesktopComponents(); }

void WineRendererFallback::timerCallback() { flushPendingSoftwareRepaints(); }

void WineRendererFallback::flushPendingSoftwareRepaints()
{
    juce::Array<juce::Component::SafePointer<juce::Component>> components;
    auto &desktop = juce::Desktop::getInstance();

    for (int i = 0; i < desktop.getNumComponents(); ++i)
        components.add(desktop.getComponent(i));

    for (const auto &component : components)
        if (component != nullptr)
            if (auto *peer = component->getPeer())
                peer->performAnyPendingRepaintsNow();
}

void WineRendererFallback::applyToDesktopComponents()
{
    auto &desktop = juce::Desktop::getInstance();

    for (int i = 0; i < desktop.getNumComponents(); ++i)
        if (auto *component = desktop.getComponent(i))
            applyTo(*component);
}
} // namespace NextStudio

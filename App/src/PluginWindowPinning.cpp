#include "PluginWindowPinning.h"

#include "../JuceLibraryCode/JuceHeader.h"

#if JUCE_LINUX
#include <X11/Xlib.h>
#include <dlfcn.h>

namespace
{
class X11Api
{
public:
    X11Api()
    {
        library = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
        if (library == nullptr)
            library = dlopen("libX11.so", RTLD_LAZY | RTLD_LOCAL);

        if (library == nullptr)
            return;

        openDisplay = load<OpenDisplay>("XOpenDisplay");
        closeDisplay = load<CloseDisplay>("XCloseDisplay");
        defaultRootWindow = load<DefaultRootWindow>("XDefaultRootWindow");
        internAtom = load<InternAtom>("XInternAtom");
        sendEvent = load<SendEvent>("XSendEvent");
        flush = load<Flush>("XFlush");
    }

    ~X11Api()
    {
        if (library != nullptr)
            dlclose(library);
    }

    bool isAvailable() const
    {
        return openDisplay != nullptr && closeDisplay != nullptr && defaultRootWindow != nullptr
               && internAtom != nullptr && sendEvent != nullptr && flush != nullptr;
    }

    using OpenDisplay = Display *(*)(const char *);
    using CloseDisplay = int (*)(Display *);
    using DefaultRootWindow = Window (*)(Display *);
    using InternAtom = Atom (*)(Display *, const char *, Bool);
    using SendEvent = Status (*)(Display *, Window, Bool, long, XEvent *);
    using Flush = int (*)(Display *);

    OpenDisplay openDisplay{};
    CloseDisplay closeDisplay{};
    DefaultRootWindow defaultRootWindow{};
    InternAtom internAtom{};
    SendEvent sendEvent{};
    Flush flush{};

private:
    template <typename Function>
    Function load(const char *name) const
    {
        return reinterpret_cast<Function>(dlsym(library, name));
    }

    void *library{};
};
} // namespace
#endif

bool PluginWindowPinning::setLinuxAlwaysOnTop(juce::Component &component, bool pinned)
{
#if JUCE_LINUX
    auto *peer = component.getPeer();
    if (peer == nullptr || peer->getNativeHandle() == nullptr)
        return false;

    static X11Api api;
    if (!api.isAvailable())
        return false;

    auto *display = api.openDisplay(nullptr);
    if (display == nullptr)
        return false;

    const auto window = static_cast<Window>(reinterpret_cast<uintptr_t>(peer->getNativeHandle()));
    const auto root = api.defaultRootWindow(display);
    const auto stateAtom = api.internAtom(display, "_NET_WM_STATE", False);
    const auto aboveAtom = api.internAtom(display, "_NET_WM_STATE_ABOVE", False);

    bool success = false;
    if (window != None && root != None && stateAtom != None && aboveAtom != None)
    {
        XEvent event{};
        event.xclient.type = ClientMessage;
        event.xclient.display = display;
        event.xclient.window = window;
        event.xclient.message_type = stateAtom;
        event.xclient.format = 32;
        event.xclient.data.l[0] = pinned ? 1 : 0; // _NET_WM_STATE_ADD / _NET_WM_STATE_REMOVE
        event.xclient.data.l[1] = static_cast<long>(aboveAtom);
        event.xclient.data.l[2] = None;
        event.xclient.data.l[3] = 1; // Normal application request.
        event.xclient.data.l[4] = 0;

        success = api.sendEvent(display,
                                root,
                                False,
                                SubstructureRedirectMask | SubstructureNotifyMask,
                                &event)
                  != 0;
        api.flush(display);
    }

    api.closeDisplay(display);
    return success;
#else
    juce::ignoreUnused(component, pinned);
    return false;
#endif
}

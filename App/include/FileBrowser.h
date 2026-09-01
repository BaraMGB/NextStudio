/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

#include "DirectoryBrowser.h"

/** General-purpose Home file browser.

    File activation and preview behaviour are configured by SidebarComponent;
    the browser itself has no Engine, Edit or sample-preview dependency.
*/
class FileBrowserComponent : public DirectoryBrowserComponent
{
public:
    explicit FileBrowserComponent(ApplicationViewState &avs);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBrowserComponent)
};

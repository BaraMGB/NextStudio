/* This file is original from the juce sources 
 * "juce::PluginListComponent::Scanner"
 * modified for NextStudio */

/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Utilities.h"

class PluginScanner    : private juce::Timer, public juce::ChangeBroadcaster
{
public:
    PluginScanner (te::Engine& en, juce::AudioPluginFormat& format, const juce::StringArray& filesOrIdentifiers,
             juce::PropertiesFile* properties, bool allowPluginsWhichRequireAsynchronousInstantiation, int threads,
             const juce::String& title, const juce::String& text);

    ~PluginScanner() override;

    juce::FileSearchPath getLastSearchPath (juce::PropertiesFile& properties, juce::AudioPluginFormat& format);

    void setLastSearchPath (juce::PropertiesFile& properties, juce::AudioPluginFormat& format, const juce::FileSearchPath& newPath);

    std::set<juce::String> m_initiallyBlacklistedFiles;
    std::unique_ptr<juce::PluginDirectoryScanner> m_dirScanner;

private:

    struct ScanJob  : public juce::ThreadPoolJob
    {
        ScanJob (PluginScanner& s)  : ThreadPoolJob ("pluginscan"), scanner (s) {}
        JobStatus runJob()
        {
            while (scanner.doNextScan() && ! shouldExit())
            {}

            return jobHasFinished;
        }
        PluginScanner& scanner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScanJob)
    };

    static void startScanCallback (int result, juce::AlertWindow* alert, PluginScanner* scanner);
    void warnUserAboutStupidPaths();
    static bool isStupidPath (const juce::File& f);
    void startScan();
    void finishedScan();
    void timerCallback() override;
    bool doNextScan();

    te::Engine& m_engine;
    juce::AudioPluginFormat& m_formatToScan;
    juce::StringArray m_filesOrIdentifiersToScan;
    juce::PropertiesFile* m_propertiesToUse;
    juce::AlertWindow m_pathChooserWindow, m_progressWindow;
    juce::FileSearchPathListComponent m_pathList;
    juce::String m_pluginBeingScanned;
    double m_progress = 0;
    const int m_numThreads;
    bool m_allowAsync, m_timerReentrancyCheck = false;
    std::atomic<bool> m_finished { false };
    std::unique_ptr<juce::ThreadPool> m_threadPool;
    juce::ScopedMessageBox m_messageBox;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginScanner)
};

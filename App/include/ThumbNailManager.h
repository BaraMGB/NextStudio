
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
namespace te = tracktion_engine;
class SimpleThumbnail : private juce::ChangeListener
{
public:
    SimpleThumbnail(te::Engine &engine, te::AudioFile &audioFile, std::function<void()> onChange);
    ~SimpleThumbnail() override;

    bool isValid() const { return m_wasSourceLoaded; }
    const juce::File &getSourceFile() const { return m_sourceFile; }

    void drawChannels(juce::Graphics &g, const juce::Rectangle<float> &area, double startTimeSeconds, double endTimeSeconds, int channelNumber, float verticalZoomFactor = 1.0f)
    {
        if (!m_wasSourceLoaded || area.isEmpty())
            return;

        if (m_useCustomDrawing)
        {
            drawChannelsCustom(g, area, startTimeSeconds, endTimeSeconds, channelNumber, verticalZoomFactor);
        }
        else
        {
            juce::Rectangle<int> intArea((int)area.getX(), (int)area.getY(), (int)area.getWidth(), (int)area.getHeight());
            m_audioThumbnail.drawChannel(g, intArea, startTimeSeconds, endTimeSeconds, channelNumber, verticalZoomFactor);
        }
    }

    double getLengthInSeconds() const { return m_lengthInSeconds; }
    int getNumChannels() const { return m_audioThumbnail.getNumChannels(); }
    void setUseCustomDrawing(bool useCustom) { m_useCustomDrawing = useCustom; }

private:
    void changeListenerCallback(juce::ChangeBroadcaster *) override;
    void drawChannelsCustom(juce::Graphics &g, const juce::Rectangle<float> &area, double startTimeSeconds, double endTimeSeconds, int channelNumber, float verticalZoomFactor);

    juce::AudioThumbnail m_audioThumbnail;
    juce::File m_sourceFile;
    std::function<void()> m_onChange;
    bool m_wasSourceLoaded = false;
    bool m_useCustomDrawing = false;
    double m_lengthInSeconds = 0.0;
};

class ThumbNailManager : public juce::ChangeBroadcaster
{
public:
    ThumbNailManager(te::Engine &engine)
        : m_audioEngine(engine)
    {
    }

    ~ThumbNailManager() = default;

    SimpleThumbnail *getOrCreateThumbnail(te::WaveAudioClip::Ptr wac)
    {
        te::AudioFile af = wac->getPlaybackFile();
        // Compressed audio needs an asynchronous WAV proxy. Its original audio
        // is already readable, so don't leave the clip blank while waiting.
        if (!af.isValid() && (wac->hasAnyTakes()
            || (!wac->usesTimeStretchedProxy() && wac->getAudioFile().getInfo().needsCachedProxy)))
            af = wac->getAudioFile();

        if (!af.isValid() || (!af.getFile().existsAsFile() && wac->usesSourceFile()))
            return nullptr;

        auto it = m_thumbnailMap.find(wac->itemID);
        if (it != m_thumbnailMap.end() && it->second->getSourceFile() == af.getFile())
            return it->second.get();

        m_thumbnailMap.erase(wac->itemID);
        auto thumbnail = std::make_unique<SimpleThumbnail>(m_audioEngine, af, [this] { sendChangeMessage(); });
        if (!thumbnail->isValid())
            return nullptr;

        auto *result = thumbnail.get();
        m_thumbnailMap.emplace(wac->itemID, std::move(thumbnail));
        return result;
    }

    void removeThumbnail(const te::EditItemID &clipID) { m_thumbnailMap.erase(clipID); }

    void clearThumbnails() { m_thumbnailMap.clear(); }

private:
    std::map<te::EditItemID, std::unique_ptr<SimpleThumbnail>> m_thumbnailMap;
    te::Engine &m_audioEngine;

    ThumbNailManager(const ThumbNailManager &) = delete;
    ThumbNailManager &operator=(const ThumbNailManager &) = delete;
};

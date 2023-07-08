
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

/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/
    
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class Thumbnail    : public juce::AudioThumbnailBase
{
public:
    Thumbnail (int originalSamplesPerThumbnailSample,
                        juce::AudioFormatManager& formatManager,
                        juce::AudioThumbnailCache& cacheToUse);

    ~Thumbnail() override;

    void clear() override;
    void clearChannelData();

    void reset (int newNumChannels, double newSampleRate, juce::int64 totalSamplesInSource = 0) override;
    void createChannels (int length);

    //==============================================================================
    bool loadFrom (juce::InputStream& rawInput) override;
    void saveTo (juce::OutputStream& output) const override;

    //==============================================================================
    bool setSource (juce::InputSource*) override;
    void setReader (juce::AudioFormatReader*, juce::int64) override;

    void releaseResources();

    juce::int64 getHashCode() const override;

    void addBlock (juce::int64 startSample, const juce::AudioBuffer<float>& incoming,
                   int startOffsetInBuffer, int numSamples) override;

    //==============================================================================
    int getNumChannels() const noexcept override;
    double getTotalLength() const noexcept override;
    bool isFullyLoaded() const noexcept override;
    double getProportionComplete() const noexcept;
    juce::int64 getNumSamplesFinished() const noexcept override;
    float getApproximatePeak() const override;
    void getApproximateMinMax (double startTime, double endTime, int channelIndex,
                               float& minValue, float& maxValue) const noexcept override;

    void drawChannel (juce::Graphics&, juce::Rectangle<int> area, bool useHighRes,
                      tracktion::TimeRange time, int channelNum, float verticalZoomFactor);

    void drawChannels (juce::Graphics&, juce::Rectangle<int> area, bool useHighRes,
                       tracktion::TimeRange time, float verticalZoomFactor);

private:
    //==============================================================================
    juce::AudioFormatManager& formatManagerToUse;
    juce::AudioThumbnailCache& cache;

    class LevelDataSource;
    struct MinMaxValue;
    class ThumbData;
    class CachedWindow;

    std::unique_ptr<LevelDataSource> source;
    std::unique_ptr<CachedWindow> window;
    juce::OwnedArray<ThumbData> channels;

    tracktion::SampleCount totalSamples = 0, numSamplesFinished = 0;
    int samplesPerThumbSample = 0;
    int numChannels = 0;
    double sampleRate = 0;
    juce::CriticalSection lock, sourceLock;

    bool setDataSource (LevelDataSource*);
    void setLevels (const MinMaxValue* const* values, int thumbIndex, int numChans, int numValues);

    void drawChannel (juce::Graphics&, const juce::Rectangle<int>& area, double startTime,
                      double endTime, int channelNum, float verticalZoomFactor) override;
    void drawChannels (juce::Graphics&, const juce::Rectangle<int>& area, double startTimeSeconds,
                       double endTimeSeconds, float verticalZoomFactor) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Thumbnail)
};


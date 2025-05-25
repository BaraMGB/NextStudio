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

#include "ThumbNailManager.h"

SimpleThumbnail::SimpleThumbnail(te::Engine& engine, te::AudioFile& audioFile)
: m_audioThumbnail(128, engine.getAudioFileFormatManager().readFormatManager, engine.getAudioFileManager().getAudioThumbnailCache())
{
    auto* inputSource = new juce::FileInputSource(audioFile.getFile());
    m_wasSourceLoaded = m_audioThumbnail.setSource(inputSource);
    m_lengthInSeconds = audioFile.getLength();
}

void SimpleThumbnail::drawChannelsCustom(juce::Graphics& g, const juce::Rectangle<float>& area, 
                                         double startTimeSeconds, double endTimeSeconds, 
                                         int channelNumber, float verticalZoomFactor)
{
    if (!m_wasSourceLoaded || area.isEmpty() || channelNumber < 0 || channelNumber >= m_audioThumbnail.getNumChannels())
        return;

    const juce::Rectangle<float> channelArea = area;
    const float centreY = channelArea.getY() + channelArea.getHeight() / 2.0f;

    const float timeRange = (float)(endTimeSeconds - startTimeSeconds);
    const int numPoints = (int)area.getWidth() + 1;
    const float timePerPixel = timeRange / (numPoints - 1);

    juce::Path waveformPath;
    bool pathStarted = false;

    for (int i = 0; i < numPoints; ++i)
    {
        const float x = area.getX() + (i * area.getWidth() / (numPoints - 1));
        const double time = startTimeSeconds + i * timePerPixel;

        float minValue = 0.0f;
        float maxValue = 0.0f;

        m_audioThumbnail.getApproximateMinMax(time, time + timePerPixel, channelNumber, minValue, maxValue);

        const float topY = centreY - channelArea.getHeight() * 0.5f * maxValue * verticalZoomFactor;

        if (!pathStarted)
        {
            waveformPath.startNewSubPath(x, topY);
            pathStarted = true;
        }
        else
    {
            waveformPath.lineTo(x, topY);
        }
    }

    for (int i = numPoints - 1; i >= 0; --i)
    {
        const float x = area.getX() + (i * area.getWidth() / (numPoints - 1));
        const double time = startTimeSeconds + i * timePerPixel;

        float minValue = 0.0f;
        float maxValue = 0.0f;

        m_audioThumbnail.getApproximateMinMax(time, time + timePerPixel, channelNumber, minValue, maxValue);

        const float bottomY = centreY - channelArea.getHeight() * 0.5f * minValue * verticalZoomFactor;

        waveformPath.lineTo(x, bottomY);
    }

    waveformPath.closeSubPath();

    g.fillPath(waveformPath);
}

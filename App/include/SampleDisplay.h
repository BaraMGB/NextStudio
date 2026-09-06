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

struct SampleView : public juce::Component
{
    explicit SampleView(te::Edit &edit);

    void setFile(const te::AudioFile &file);
    void setColour(juce::Colour colour) { m_colour = colour; }
    void paint(juce::Graphics &g) override;

    te::SmartThumbnail &getSmartThumbnail() { return m_smartThumbnail; }

private:
    te::Edit &m_edit;
    te::AudioFile m_audioFile;
    juce::Colour m_colour;
    te::SmartThumbnail m_smartThumbnail;
};

class MarkerComponent : public juce::Component
{
public:
    enum MarkerType
    {
        Start,
        End
    };

    static constexpr float MARKER_WIDTH = 20.0f;
    static constexpr float HANDLE_SIZE = 8.0f;
    static constexpr float LINE_WIDTH = 2.0f;
    static constexpr float TIME_THRESHOLD = 0.001;

    MarkerComponent(MarkerType type, juce::Colour colour);
    void setPosition(double time, double totalLength, float componentWidth);
    void setPosition(double time, double totalLength, float componentWidth, double minTime = 0.0, double maxTime = -1.0);
    void updateFromTime(double time, double totalLength, float componentWidth);
    bool isOverPosition(const juce::Point<float> &position) const;
    double getTime() const { return m_time; }

    std::function<void(double newTime)> onPositionChanged;

private:
    MarkerType m_type;
    juce::Colour m_colour;
    double m_time = 0.0;
    juce::DrawableRectangle m_line;
    juce::DrawableRectangle m_handle;

    void resized() override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    bool m_isDragging = false;
    double m_totalLength = 0.0;
    float m_componentWidth = 0.0;
    double m_minTime = 0.0;
    double m_maxTime = -1.0;
    double timeFromPosition(const juce::Point<float> &position) const;
};

class ApplicationViewState;

class SampleDisplayBase : public juce::Component
{
public:
    static constexpr int CURSOR_UPDATE_FPS = 15;

    SampleDisplayBase(te::Edit &edit, ApplicationViewState &appViewState);

    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &) override;
    void setFile(const te::AudioFile &file);
    void setColour(juce::Colour colour);

    void setStartEndPositions(double start, double end);
    void clearStartEndMarkers();
    void refreshMarkers();

    std::function<void(double start, double end)> onMarkerPositionChanged;

protected:
    virtual std::optional<double> getPlaybackPosition() = 0;
    virtual bool canScrub() const { return false; }
    virtual void beginScrub() {}
    virtual void seek(double) {}
    virtual void endScrub() {}

    void updateCursorPosition();
    double getSampleLength() const { return m_totalLength; }

private:
    ApplicationViewState &m_appViewState;
    juce::DrawableRectangle cursor;
    MarkerComponent m_startMarker;
    MarkerComponent m_endMarker;
    te::LambdaTimer cursorUpdater;
    SampleView m_sampleView;

    double m_startPosition = -1.0;
    double m_endPosition = -1.0;
    double m_totalLength = 0.0;

    void updateStartEndMarkers();
    double positionToTime(const juce::Point<float> &position) const;
};

class TransportSampleDisplay final : public SampleDisplayBase
{
public:
    TransportSampleDisplay(te::TransportControl &transport, ApplicationViewState &appViewState);

protected:
    std::optional<double> getPlaybackPosition() override;
    bool canScrub() const override { return true; }
    void beginScrub() override;
    void seek(double sampleTime) override;
    void endScrub() override;

private:
    te::TransportControl &m_transport;
};

class TriggeredSampleDisplay final : public SampleDisplayBase
{
public:
    TriggeredSampleDisplay(te::Edit &edit, ApplicationViewState &appViewState);

    void trigger(double excerptStart, double excerptLength);
    void release(bool openEnded);
    void stopPlayback();

protected:
    std::optional<double> getPlaybackPosition() override;

private:
    double m_triggerTimeMs = 0.0;
    double m_excerptStart = 0.0;
    double m_excerptLength = 0.0;
    bool m_isPlaying = false;
};

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

#include "SampleDisplay.h"
#include "ApplicationViewState.h"
#include "Utilities.h"

//==============================================================================
// MarkerComponent Implementation
//==============================================================================

MarkerComponent::MarkerComponent(MarkerType type, juce::Colour colour)
    : m_type(type),
      m_colour(colour)
{
    m_line.setFill(colour);
    m_handle.setFill(colour);
    addAndMakeVisible(m_line);
    addAndMakeVisible(m_handle);

    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void MarkerComponent::setPosition(double time, double totalLength, float componentWidth) { setPosition(time, totalLength, componentWidth, 0.0, totalLength); }

void MarkerComponent::setPosition(double time, double totalLength, float componentWidth, double minTime, double maxTime)
{
    m_time = time;
    m_totalLength = totalLength;
    m_componentWidth = componentWidth;
    m_minTime = minTime;
    m_maxTime = (maxTime < 0) ? totalLength : maxTime;
    updateFromTime(time, totalLength, componentWidth);
}

void MarkerComponent::updateFromTime(double time, double totalLength, float componentWidth)
{
    m_time = time;
    m_totalLength = totalLength;
    m_componentWidth = componentWidth;

    if (m_totalLength <= 0.0 || m_componentWidth <= 0.0)
    {
        setVisible(false);
        return;
    }

    setVisible(true);

    // Position this component at the correct x position
    float x = m_componentWidth * (float)(m_time / m_totalLength);

    // Adjust position for end marker to keep handle visible
    float componentX = x;
    float compWidth = MARKER_WIDTH;

    if (m_type == End)
        componentX = x - compWidth; // Shift left so handle is visible

    auto gap = 2;
    setBounds(componentX, gap, compWidth, getParentHeight() - gap * 2);

    auto r = getLocalBounds().toFloat();

    // Update line (positioned at center of component)
    float lineX = (m_type == Start) ? 0.0f : (float)(compWidth - LINE_WIDTH);
    m_line.setRectangle(r.withWidth(LINE_WIDTH).withX(lineX));

    // Update handle position based on type
    float handleSize = HANDLE_SIZE;
    juce::Rectangle<float> handleRect;

    if (m_type == Start)
    {
        // Start handle - top right
        handleRect = juce::Rectangle<float>(LINE_WIDTH, r.getY(), handleSize, handleSize);
    }
    else
    {
        // End handle - bottom left (positioned within component bounds)
        handleRect = juce::Rectangle<float>(compWidth - handleSize - LINE_WIDTH, r.getBottom() - handleSize, handleSize, handleSize);
    }

    m_handle.setRectangle(handleRect);
}

bool MarkerComponent::isOverPosition(const juce::Point<float> &position) const
{
    if (!isVisible() || m_totalLength <= 0.0)
        return false;

    // Check if position is within this component's bounds
    return getLocalBounds().toFloat().contains(position);
}

void MarkerComponent::resized() { updateFromTime(m_time, m_totalLength, m_componentWidth); }

void MarkerComponent::mouseDown(const juce::MouseEvent &e) { m_isDragging = true; }

void MarkerComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (!m_isDragging || m_totalLength <= 0.0)
        return;

    double newTime = timeFromPosition(e.position);

    // Apply constraints based on marker type and bounds
    newTime = juce::jlimit(m_minTime, m_maxTime, newTime);

    if (std::abs(newTime - m_time) > TIME_THRESHOLD) // Only update if significant change
    {
        m_time = newTime;
        updateFromTime(m_time, m_totalLength, m_componentWidth);

        if (onPositionChanged)
            onPositionChanged(m_time);
    }
}

void MarkerComponent::mouseUp(const juce::MouseEvent &e) { m_isDragging = false; }

double MarkerComponent::timeFromPosition(const juce::Point<float> &position) const
{
    if (m_totalLength <= 0.0 || m_componentWidth <= 0)
        return 0.0;

    // Use parent component width for calculation
    float globalX = getX() + position.x;

    // For end marker, adjust for the offset positioning
    if (m_type == End)
        globalX += (getWidth() - LINE_WIDTH); // Add the offset we applied in positioning

    float proportion = juce::jlimit(0.0f, 1.0f, globalX / m_componentWidth);
    return proportion * m_totalLength;
}

//==============================================================================
// SampleDisplay Implementations
//==============================================================================

SampleDisplayBase::SampleDisplayBase(te::Edit &edit, ApplicationViewState &appViewState)
    : m_appViewState(appViewState),
      m_startMarker(MarkerComponent::Start, appViewState.getPrimeColour()),
      m_endMarker(MarkerComponent::End, appViewState.getPrimeColour()),
      m_sampleView(edit)
{
    addAndMakeVisible(m_sampleView);
    cursorUpdater.setCallback(
        [this]
        {
            updateCursorPosition();

            if (m_sampleView.getSmartThumbnail().isGeneratingProxy() || m_sampleView.getSmartThumbnail().isOutOfDate())
                repaint();
        });
    cursor.setFill(findColour(juce::Label::textColourId));
    addAndMakeVisible(cursor);
    cursor.setVisible(false);

    addAndMakeVisible(m_startMarker);
    addAndMakeVisible(m_endMarker);

    m_startMarker.onPositionChanged = [this](double newTime)
    {
        m_startPosition = newTime;
        if (m_endPosition >= 0.0 && m_startPosition > m_endPosition)
        {
            m_startPosition = m_endPosition;
            m_startMarker.setPosition(m_startPosition, m_totalLength, (float)getWidth(), 0.0, m_endPosition);
        }
        if (onMarkerPositionChanged)
            onMarkerPositionChanged(m_startPosition, m_endPosition);
    };

    m_endMarker.onPositionChanged = [this](double newTime)
    {
        m_endPosition = newTime;
        if (m_startPosition >= 0.0 && m_endPosition < m_startPosition)
        {
            m_endPosition = m_startPosition;
            m_endMarker.setPosition(m_endPosition, m_totalLength, (float)getWidth(), m_startPosition, m_totalLength);
        }
        if (onMarkerPositionChanged)
            onMarkerPositionChanged(m_startPosition, m_endPosition);
    };
}

void SampleDisplayBase::resized()
{
    m_sampleView.setBounds(getLocalBounds());
    updateStartEndMarkers();
    updateCursorPosition();
}

void SampleDisplayBase::setFile(const tracktion_engine::AudioFile &file)
{
    m_sampleView.setFile(file);

    if (file.isValid() && file.getSampleRate() > 0.0)
    {
        m_totalLength = file.getLengthInSamples() / file.getSampleRate();
        cursorUpdater.startTimerHz(CURSOR_UPDATE_FPS);
    }
    else
    {
        m_totalLength = 0.0;
        cursorUpdater.stopTimer();
        cursor.setVisible(false);
        clearStartEndMarkers();
    }

    updateStartEndMarkers();
    updateCursorPosition();
    repaint();
}

void SampleDisplayBase::setColour(juce::Colour colour) { m_sampleView.setColour(colour); }

void SampleDisplayBase::updateCursorPosition()
{
    const auto playbackPosition = getPlaybackPosition();
    if (!playbackPosition.has_value() || m_totalLength <= 0.0 || getWidth() <= 0)
    {
        cursor.setVisible(false);
        return;
    }

    const double proportion = juce::jlimit(0.0, 1.0, *playbackPosition / m_totalLength);
    auto r = getLocalBounds().toFloat();
    const float cursorWidth = 2.0f;
    const float x = juce::jlimit(0.0f, juce::jmax(0.0f, r.getWidth() - cursorWidth), r.getWidth() * float(proportion));
    cursor.setRectangle(r.withWidth(cursorWidth).withX(x));
    cursor.setVisible(true);
}

void SampleDisplayBase::mouseDown(const juce::MouseEvent &e)
{
    if (m_startMarker.isOverPosition(e.position) || m_endMarker.isOverPosition(e.position) || !canScrub())
        return;

    beginScrub();
    mouseDrag(e);
}

void SampleDisplayBase::mouseDrag(const juce::MouseEvent &e)
{
    if (!canScrub() || getWidth() <= 0)
        return;

    seek(positionToTime(e.position));
}

void SampleDisplayBase::mouseUp(const juce::MouseEvent &)
{
    if (canScrub())
        endScrub();
}

void SampleDisplayBase::setStartEndPositions(double start, double end)
{
    if (start < 0.0 || end < 0.0 || m_totalLength <= 0.0)
    {
        clearStartEndMarkers();
        return;
    }

    if (start > end)
        std::swap(start, end);

    start = juce::jlimit(0.0, m_totalLength, start);
    end = juce::jlimit(0.0, m_totalLength, end);

    m_startPosition = start;
    m_endPosition = end;

    m_startMarker.setPosition(m_startPosition, m_totalLength, (float)getWidth(), 0.0, m_endPosition);
    m_endMarker.setPosition(m_endPosition, m_totalLength, (float)getWidth(), m_startPosition, m_totalLength);
}

void SampleDisplayBase::clearStartEndMarkers()
{
    m_startPosition = -1.0;
    m_endPosition = -1.0;
    updateStartEndMarkers();
}

void SampleDisplayBase::refreshMarkers() { updateStartEndMarkers(); }

void SampleDisplayBase::updateStartEndMarkers()
{
    if (m_startPosition >= 0.0 && m_totalLength > 0.0)
    {
        m_startMarker.setPosition(m_startPosition, m_totalLength, (float)getWidth(), 0.0, (m_endPosition >= 0.0) ? m_endPosition : m_totalLength);
        m_startMarker.setVisible(true);
    }
    else
    {
        m_startMarker.setVisible(false);
    }

    if (m_endPosition >= 0.0 && m_totalLength > 0.0)
    {
        m_endMarker.setPosition(m_endPosition, m_totalLength, (float)getWidth(), (m_startPosition >= 0.0) ? m_startPosition : 0.0, m_totalLength);
        m_endMarker.setVisible(true);
    }
    else
    {
        m_endMarker.setVisible(false);
    }
}

double SampleDisplayBase::positionToTime(const juce::Point<float> &position) const
{
    if (m_totalLength <= 0.0 || getWidth() <= 0)
        return 0.0;

    const float proportion = juce::jlimit(0.0f, 1.0f, position.x / (float)getWidth());
    return proportion * m_totalLength;
}

TransportSampleDisplay::TransportSampleDisplay(te::TransportControl &transport, ApplicationViewState &appViewState)
    : SampleDisplayBase(transport.edit, appViewState),
      m_transport(transport)
{
}

std::optional<double> TransportSampleDisplay::getPlaybackPosition()
{
    if (!m_transport.isPlaying())
        return std::nullopt;

    const auto loopRange = m_transport.getLoopRange();
    const double sampleTime = (m_transport.getPosition() - loopRange.getStart()).inSeconds();
    if (sampleTime > getSampleLength())
        return std::nullopt;

    return juce::jmax(0.0, sampleTime);
}

void TransportSampleDisplay::beginScrub() { m_transport.setUserDragging(true); }

void TransportSampleDisplay::seek(double sampleTime)
{
    const auto loopStart = m_transport.getLoopRange().getStart();
    m_transport.position = loopStart + tracktion::TimeDuration::fromSeconds(sampleTime);
}

void TransportSampleDisplay::endScrub() { m_transport.setUserDragging(false); }

TriggeredSampleDisplay::TriggeredSampleDisplay(te::Edit &edit, ApplicationViewState &appViewState)
    : SampleDisplayBase(edit, appViewState)
{
}

void TriggeredSampleDisplay::trigger(double excerptStart, double excerptLength)
{
    m_excerptStart = juce::jmax(0.0, excerptStart);
    m_excerptLength = juce::jmax(0.0, excerptLength);
    m_triggerTimeMs = juce::Time::getMillisecondCounterHiRes();
    m_isPlaying = m_excerptLength > 0.0;
    updateCursorPosition();
}

void TriggeredSampleDisplay::release(bool openEnded)
{
    if (!openEnded)
        stopPlayback();
}

void TriggeredSampleDisplay::stopPlayback()
{
    m_isPlaying = false;
    updateCursorPosition();
}

std::optional<double> TriggeredSampleDisplay::getPlaybackPosition()
{
    if (!m_isPlaying)
        return std::nullopt;

    const double elapsedSeconds = (juce::Time::getMillisecondCounterHiRes() - m_triggerTimeMs) / 1000.0;
    if (elapsedSeconds >= m_excerptLength)
    {
        m_isPlaying = false;
        return std::nullopt;
    }

    return m_excerptStart + elapsedSeconds;
}

SampleView::SampleView(te::Edit &edit)
    : m_edit(edit),
      m_audioFile(edit.engine, {}),
      m_smartThumbnail(m_edit.engine, te::AudioFile(m_edit.engine), *this, nullptr)
{
    setInterceptsMouseClicks(false, false);
}

void SampleView::setFile(const tracktion_engine::AudioFile &file)
{
    m_audioFile = file;
    m_smartThumbnail.setNewFile(file);
}

void SampleView::paint(juce::Graphics &g)
{
    auto r = getLocalBounds();
    const auto colour = m_colour != juce::Colour() ? m_colour : findColour(juce::Label::textColourId);

    if (m_smartThumbnail.isGeneratingProxy())
    {
        g.setColour(colour);
        g.drawText("Creating proxy: " + juce::String(juce::roundToInt(m_smartThumbnail.getProxyProgress() * 100.0f)) + "%", r, juce::Justification::centred);
    }
    else
    {
        const float brightness = m_smartThumbnail.isOutOfDate() ? 0.4f : 0.66f;
        g.setColour(colour.withMultipliedBrightness(brightness));
        m_smartThumbnail.drawChannels(g, r, {tracktion::TimePosition::fromSeconds(0.0), tracktion::TimeDuration::fromSeconds(m_smartThumbnail.getTotalLength())}, 1.0f);
    }
}

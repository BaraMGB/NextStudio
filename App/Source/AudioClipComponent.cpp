#include "AudioClipComponent.h"

AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
{
    updateThumbnail ();
    setPaintingIsUnclipped(true);
}

void AudioClipComponent::paint (juce::Graphics& g)
{
    ClipComponent::paint (g);

    auto viewportOffset = -(m_editViewState.timeToX(
                                0
                              , getParentComponent()->getWidth()));

    auto viewportEndX =  getParentComponent()->getWidth();
    auto clipstartX = m_editViewState.timeToX(
                m_clip->getPosition().getStart()
              , getParentComponent()->getWidth());
    auto clipendX = clipstartX + getWidth();

    auto left = clipstartX < 0 ? -clipstartX : 0;
    auto right = clipendX > viewportEndX ? clipendX - viewportEndX : 0;


    if (m_editViewState.m_drawWaveforms && thumbnail != nullptr)
    {

        drawWaveform(g
                     , *getWaveAudioClip()
                     , *thumbnail
                     , juce::Colours::black.withAlpha(0.5f)
                     , left - viewportOffset
                     , getWidth() - right - viewportOffset
                     , 12
                     , getHeight() - 14
                     , viewportOffset);
    }
}

void AudioClipComponent::resized()
{

}

void AudioClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void AudioClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    m_posAtMouseDown =  m_clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    ClipComponent::mouseDown(e);
    m_lastOffset = 0.0;
    m_oldDistTime = 0.0;
}

void AudioClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto distanceBeats = m_editViewState.xToBeats(
                e.getDistanceFromDragStartX(),getParentWidth());
    auto snapType = m_editViewState.getBestSnapType (
                m_editViewState.m_viewX1
              , m_editViewState.m_viewX2
              , getParentWidth ());
    const auto distanceTime = e.mods.isShiftDown ()
                            ? m_editViewState.beatToTime(
                                 distanceBeats  - m_editViewState.m_viewX1)
                            : m_editViewState.getSnapedTime (
                                  m_editViewState.beatToTime(
                                      distanceBeats  - m_editViewState.m_viewX1)
                                      , snapType);
    auto distTimeDelta = distanceTime - m_oldDistTime;

    //shrink left
    if (m_mouseDownX < 10 && m_clipWidthMouseDown > 30)
    {
        const auto newTime = m_clip->getPosition().getStart() + distTimeDelta;
        const auto newOffset = m_clip->getPosition().getOffset() + distTimeDelta;

        if ((distTimeDelta > 0
         || m_clip->getPosition().getOffset() > 0 )
         && !(newTime > m_clip->getPosition().getEnd()))
        {
            m_clip->setStart(juce::jmax(0.0, newTime), false, false);
            if (newOffset < 0)
            {
                m_clip->setStart(juce::jmax(0.0
                                 , m_clip->getPosition().getStart() - newOffset)
                                 , false, false);
                m_lastOffset = newOffset;
            }
            m_clip->setOffset(newOffset);
        }
        else
        {
            m_posAtMouseDown = m_clip->getPosition();
            m_lastOffset = 0.0;
        }
        m_oldDistTime = distanceTime;
    }
    //shrink right
    else if (m_mouseDownX > m_clipWidthMouseDown - 10 && m_clipWidthMouseDown > 30)
    {
        auto snapType = m_editViewState.getBestSnapType (
                    m_editViewState.m_viewX1
                  , m_editViewState.m_viewX2
                  , getParentWidth ());
        auto snapedTime = m_editViewState.getSnapedTime (
                    m_posAtMouseDown.getEnd ()
                  , snapType);
        m_clip->setEnd(snapedTime + distanceTime, true);
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}

tracktion_engine::WaveAudioClip *AudioClipComponent::getWaveAudioClip()
{
    return dynamic_cast<te::WaveAudioClip*> (m_clip.get());
}

void AudioClipComponent::drawWaveform(juce::Graphics& g,
                                      te::AudioClipBase& c,
                                      te::SmartThumbnail& thumb,
                                      juce::Colour colour,
                                      const int left,
                                      const int right,
                                      int y,
                                      int h,
                                      int xOffset)
{
    auto getTimeRangeForDrawing =
            [this] (const int left, const int right) -> te::EditTimeRange
    {
        if (auto p = getParentComponent())
        {
            double t1 = m_editViewState.xToTime (left, p->getWidth());
            double t2 = m_editViewState.xToTime (right, p->getWidth());

            return { t1, t2 };
        }

        return {};
    };

    jassert (left <= right);
    const auto gain = c.getGain();
    const auto pan = thumb.getNumChannels() == 1 ? 0.0f : c.getPan();

    const float pv = pan * gain;
    const float gainL = (gain - pv);
    const float gainR = (gain + pv);

    const bool usesTimeStretchedProxy = c.usesTimeStretchedProxy();

    const auto clipPos = c.getPosition();
    auto offset = clipPos.getOffset();
    auto speedRatio = c.getSpeedRatio();

    g.setColour (colour);

    if (usesTimeStretchedProxy)
    {
        const juce::Rectangle<int> area(left + xOffset, y, right - left, h);

        if (! thumb.isOutOfDate())
        {
            drawChannels(g,
                         thumb,
                         area,
                         false,
                         getTimeRangeForDrawing(left, right),
                         c.isLeftChannelActive(),
                         c.isRightChannelActive(),
                         gainL,
                         gainR);
        }
    }
    else if (c.getLoopLength() == 0)
    {
        auto region = getTimeRangeForDrawing (left, right);

        auto t1 = (region.getStart() + offset) * speedRatio;
        auto t2 = (region.getEnd()   + offset) * speedRatio;
        bool useHighres = true;
        drawChannels(g,
                     thumb,
                     {left + xOffset, y, right - left, h},
                     useHighres,
                     {t1, t2},
                     c.isLeftChannelActive(),
                     c.isRightChannelActive(),
                     gainL,
                     gainR);
    }
}

void AudioClipComponent::drawChannels(juce::Graphics& g,
                                      te::SmartThumbnail& thumb,
                                      juce::Rectangle<int> area,
                                      bool useHighRes,
                                      te::EditTimeRange time,
                                      bool useLeft,
                                      bool useRight,
                                      float leftGain,
                                      float rightGain)
{
    if (useLeft && useRight && thumb.getNumChannels() > 1)
    {
        thumb.drawChannel(g,
                          area.removeFromTop(area.getHeight() / 2),
                          useHighRes,
                          time,
                          0,
                          leftGain);
        thumb.drawChannel(g, area, useHighRes, time, 1, rightGain);
    }
    else if (useLeft)
    {
        thumb.drawChannel (g, area, useHighRes, time, 0, leftGain);
    }
    else if (useRight)
    {
        thumb.drawChannel (g, area, useHighRes, time, 1, rightGain);
    }
}

void AudioClipComponent::updateThumbnail()
{
    if (auto* wac = getWaveAudioClip())
    {
        te::AudioFile af (wac->getAudioFile());

        if (af.getFile().existsAsFile() || (! wac->usesSourceFile()))
        {
            if (af.isValid())
            {
                const te::AudioFile proxy(
                            (wac->hasAnyTakes() && wac->isShowingTakes())
                            ? wac->getAudioFile()
                            : wac->getPlaybackFile());

                if (thumbnail == nullptr)
                {
                    thumbnail = std::make_unique<te::SmartThumbnail>(
                                wac->edit.engine, proxy, *this, &wac->edit);
                }
                else
                {
                    thumbnail->setNewFile (proxy);
                }
            }
            else
            {
                thumbnail = nullptr;
            }
        }
    }
}

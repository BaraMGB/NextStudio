#include "AudioClipComponent.h"

AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, std::move(c))
{
    updateThumbnail ();
    setPaintingIsUnclipped(true);
}

void AudioClipComponent::paint (juce::Graphics& g)
{
    ClipComponent::paint (g);

    auto rect = getDrawingRect ();

    if (m_editViewState.m_drawWaveforms && thumbnail)
    {
        auto colour = m_clip->getColour().withLightness(0.6f);
        drawWaveform(g
                   , *getWaveAudioClip()
                   , *thumbnail
                   , colour 
                   , rect.getX()
                   , rect.getRight ()
                   , rect.getY ()
                   , rect.getHeight ()
                   , getViewportOffset ());
    }
}

int AudioClipComponent::getViewportOffset()
{
    return Helpers::invert(m_editViewState.timeToX(0
                                        , getParentComponent()->getWidth()
                                        , m_editViewState.m_viewX1
                                        , m_editViewState.m_viewX2));
}

int AudioClipComponent::getViewportEnd()
{
    int viewportEndX =  getParentComponent()->getWidth();

    return viewportEndX;
}

int AudioClipComponent::getDrawingStartX()
{
    auto left = ((getX() < 0
                         ? Helpers::invert (getX())
                         : getMargin())
                - getViewportOffset ());

    return left;
}

int AudioClipComponent::getDrawingEndX()
{
    auto clipendX = getX() + getWidth();
    auto cutAtViewportEnd = (clipendX > getViewportEnd()
                          ?  clipendX - getViewportEnd ()
                          :  getMargin());
    auto right = getWidth () - cutAtViewportEnd - getViewportOffset ();

    return right;
}

juce::Range<int> AudioClipComponent::getDrawingRange()
{
    auto left = getDrawingStartX();
    auto right = juce::jmax(left, getDrawingEndX());

    return {left, right};
}

juce::Rectangle<int> AudioClipComponent::getDrawingRect()
{
    auto headerHeight = static_cast<int>(m_editViewState.m_clipHeaderHeight);
    auto drawingRect = juce::Rectangle<int> (
                        getDrawingRange ().getStart ()
                      , headerHeight 
                      , getDrawingRange().getLength ()
                      , getHeight() - headerHeight - 2);
    return drawingRect;
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
            [this] (const int l, const int r) -> tracktion::core::TimeRange
    {
        if (auto p = getParentComponent())
        {
            auto t1 = EngineHelpers::getTimePos(m_editViewState.xToTime (l, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
            auto t2 = EngineHelpers::getTimePos(m_editViewState.xToTime (r, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
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

    bool showBothChannels = getHeight () > 100;

    if (usesTimeStretchedProxy)
    {
        const juce::Rectangle<int> area(left + xOffset, y, right - left, h);

        if (!thumb.isOutOfDate())
        {
            drawChannels(g
                       , thumb
                       , area
                       , false
                       , getTimeRangeForDrawing(left, right)
                       , c.isLeftChannelActive() && showBothChannels
                       , c.isRightChannelActive()
                       , gainL
                       , gainR);
        }
    }
    else if (c.getLoopLength().inSeconds() == 0)
    {
        auto region = getTimeRangeForDrawing (left, right);

        auto t1 = EngineHelpers::getTimePos((region.getStart().inSeconds() + offset.inSeconds()) * speedRatio);
        auto t2 = EngineHelpers::getTimePos((region.getEnd().inSeconds()   + offset.inSeconds()) * speedRatio);
        bool useHighres = true;
        drawChannels(g
                   , thumb
                   , {left + xOffset, y, right - left, h}
                   , useHighres
                   , {t1, t2}
                   , c.isLeftChannelActive()
                   , c.isRightChannelActive() && showBothChannels
                   , gainL
                   , gainR);
    }
}

void AudioClipComponent::drawChannels(juce::Graphics& g
                                    , te::SmartThumbnail& thumb
                                    , juce::Rectangle<int> area
                                    , bool useHighRes
                                    , tracktion::core::TimeRange time
                                    , bool useLeft
                                    , bool useRight
                                    , float leftGain
                                    , float rightGain)
{
    if (useLeft && useRight && thumb.getNumChannels() > 1)
    {
        thumb.drawChannel(g
                        , area.removeFromTop(area.getHeight() / 2)
                        , useHighRes
                        , time
                        , 0
                        , leftGain);
        thumb.drawChannel(g, area, useHighRes, time, 1, rightGain);
    }
    else if (useLeft)
        thumb.drawChannel (g, area, useHighRes, time, 0, leftGain);
    else if (useRight)
        thumb.drawChannel (g, area, useHighRes, time, 1, rightGain);
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

                if (!thumbnail)
                {
                    thumbnail = std::make_unique<te::SmartThumbnail>(
                                wac->edit.engine
                              , proxy
                              , *this
                              , &wac->edit);
                }
                else thumbnail->setNewFile (proxy);
            }
            else thumbnail = nullptr;
        }
    }
}

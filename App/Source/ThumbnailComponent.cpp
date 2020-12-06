#include "ThumbnailComponent.h"


ThumbnailComponent::ThumbnailComponent (EditViewState &evs)
    : m_editViewState (evs)
    , m_thumbnailCache (5)
    , m_thumbnail (256, m_formatManager, m_thumbnailCache)
    , m_drawOffset (0.0)
    , m_drawRightOffset (0.0)
{
    setInterceptsMouseClicks (false, false);
    m_thumbnail.addChangeListener (this);
    m_formatManager.registerBasicFormats ();
}

ThumbnailComponent::~ThumbnailComponent ()
{
}

void ThumbnailComponent::changeListenerCallback (juce::ChangeBroadcaster *source)
{
    if (source == &m_thumbnail)
    {
        thumbnailChanged();
    }
}

void ThumbnailComponent::thumbnailChanged()
{
    repaint ();
}

void ThumbnailComponent::paint(juce::Graphics &g)
{
    juce::Rectangle<int> thumbnailBounds (0, 0, getWidth (), getHeight ());

    auto leftX = getBoundsInParent ().getX ();
    auto rightX = getParentWidth ()
                - (getBoundsInParent ().getX ()
                + getBoundsInParent ().getWidth ());


    auto leftT = (m_thumbnail.getTotalLength ()
                      - m_drawOffset
                      - m_drawRightOffset )
            * leftX / getParentWidth ();
    auto rightT = (m_thumbnail.getTotalLength ()
                   - m_drawOffset
                   - m_drawRightOffset )
            * rightX / getParentWidth ();
    g.setColour (juce::Colours::black.withAlpha (0.7f));

    m_thumbnail.drawChannel (g
                             , thumbnailBounds
                             , m_drawOffset + leftT
                             , m_thumbnail.getTotalLength()
                                 - rightT
                                 - m_drawRightOffset
                             , 0
                             , 1.0f);
}

void ThumbnailComponent::setFile (const juce::File &f)
{
    auto* reader = m_formatManager.createReaderFor (f);

    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource
                (new juce::AudioFormatReaderSource (reader, true));
        m_thumbnail.setSource (new juce::FileInputSource (f));
        m_readerSource.reset (newSource.release());
    }
}

void ThumbnailComponent::resized ()
{
    repaint ();
}

void ThumbnailComponent::setDrawOffset (double drawOffset)
{
    m_drawOffset = drawOffset;
}

void ThumbnailComponent::setDrawOffsetRight (double drawOffsetRight)
{
    m_drawRightOffset = drawOffsetRight;
}

#include "TrackOverlayComponent.h"

void TrackOverlayComponent::drawImages(int offset)
{
    m_mouseOffset = offset;
    setVisible (true);
    repaint();
}

void TrackOverlayComponent::addOverlayImageList(juce::Array<TrackOverlayComponent::OverlayImage> il)
{
    m_imageList.clear ();
    m_imageList = std::move(il);
}

void TrackOverlayComponent::paint(juce::Graphics &g)
{
    for (const auto& clipImage : m_imageList)
    {
        if (clipImage.m_isValid)
        {
            g.setColour(juce::Colour(0x66666666));
            g.drawImageAt(clipImage.m_image, clipImage.m_xPos + m_mouseOffset, 0);
            g.setColour(juce::Colour(0xffffffff));
            g.drawRect(clipImage.m_xPos + m_mouseOffset, 0, clipImage.m_image.getWidth(), getHeight());
        }
        else
        {
            g.setColour (juce::Colour(0xff444444));
            g.fillRect (clipImage.m_xPos + m_mouseOffset, 0, clipImage.m_image.getWidth(), getHeight());
            g.setColour (juce::Colours::black);
            g.drawText ("not allowed", clipImage.m_xPos + m_mouseOffset, 0, clipImage.m_image.getWidth (), getHeight (),juce::Justification::centred,false);
        }
    }
}

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"


namespace te = tracktion_engine;


class TrackOverlayComponent : public juce::Component
{
public:
    TrackOverlayComponent()= default;
    struct OverlayImage
    {
        OverlayImage(juce::Image i
                    , int x
                    , bool isValid)
            : m_image(std::move(i)) , m_xPos(x), m_isValid(isValid){}

        juce::Image m_image;
        int         m_xPos;
        bool        m_isValid;
    };
    void paint(juce::Graphics& g) override;

    void drawImages(int offset);
    void addResizingImage(int startX, int endX)
    {
		
        auto widthX = endX - startX;
        juce::Image image (juce::Image::ARGB, widthX, getHeight(), true);
        juce::Graphics g (image);

		g.setColour (juce::Colour(0x99ffffff));
        g.fillRect (juce::Rectangle<int>{0, 0, widthX, getHeight()});

        OverlayImage oli {image, startX, true};
        m_imageList.add(oli);
    }
    void addOverlayImageList(juce::Array<OverlayImage> il);

    juce::Array<OverlayImage> m_imageList;
private:

    int m_mouseOffset{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackOverlayComponent)
};

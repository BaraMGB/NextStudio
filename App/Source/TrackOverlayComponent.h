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
    void addOverlayImageList(juce::Array<OverlayImage> il);

private:

    juce::Array<OverlayImage> m_imageList;
    int m_mouseOffset{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackOverlayComponent)
};

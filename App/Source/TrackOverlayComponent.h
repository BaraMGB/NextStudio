#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"


namespace te = tracktion_engine;


class TrackOverlayComponent : public juce::Component
{
public:
    struct OverlayImage
    {
        OverlayImage(juce::Image i
                    , int x
                    , bool isValid)
            : m_image(i) , m_xPos(x), m_isValid(isValid){}

        juce::Image m_image;
        int         m_xPos;
        bool        m_isValid;
    };
    void paint(juce::Graphics& g) override;

    void drawImages(int offset);
    void addOverlayImage(OverlayImage overlay);
    void addOverlayImageList(juce::Array<OverlayImage> il);
    void clearOverlayImageList();
    void setIsValid(bool isValid);
    bool isValid();

private:
    juce::Array<OverlayImage> m_imageList;
    int m_mouseOffset;
};

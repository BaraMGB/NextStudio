/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>

class InlineColourEditor : public juce::Component
{
public:
    InlineColourEditor();

    void setCurrentColour(juce::Colour colour, juce::NotificationType notification = juce::dontSendNotification);
    juce::Colour getCurrentColour() const noexcept { return m_colour; }
    void setReferenceColour(juce::Colour colour);
    void setThemeColours(juce::Colour background, juce::Colour text, juce::Colour border, juce::Colour accent);

    int getPreferredHeight() const noexcept { return 206; }

    static juce::String formatHexColour(juce::Colour colour);
    static bool parseHexColour(const juce::String &text, juce::Colour &result);

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    bool keyPressed(const juce::KeyPress &key) override;
    void focusHexEditor();

    std::function<void(juce::Colour)> onColourChanged;
    std::function<void()> onReset;

private:
    class WheelPassingTextEditor : public juce::TextEditor
    {
    public:
        void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
        {
            juce::Component::mouseWheelMove(event, wheel);
        }
    };

    void updateFromPoint(juce::Point<float> point);
    void updateControls();
    void commitHexText();
    void notifyColourChanged();
    void rebuildSaturationValueImage();
    juce::Colour colourForCurrentHsv() const;

    juce::Colour m_colour{juce::Colours::white};
    juce::Colour m_referenceColour{juce::Colours::white};
    juce::Colour m_background{juce::Colour(0xff202020)};
    juce::Colour m_text{juce::Colours::white};
    juce::Colour m_border{juce::Colours::black};
    juce::Colour m_accent{juce::Colours::yellow};
    float m_hue = 0.0f;
    float m_saturation = 0.0f;
    float m_value = 1.0f;
    bool m_hexIsValid = true;
    bool m_keyboardEditsHue = false;

    juce::Rectangle<float> m_svBounds;
    juce::Rectangle<float> m_hueBounds;
    juce::Rectangle<float> m_currentPreviewBounds;
    juce::Rectangle<float> m_referencePreviewBounds;
    juce::Image m_svImage;
    float m_imageHue = -1.0f;

    juce::Label m_hexLabel;
    WheelPassingTextEditor m_hexEditor;
    juce::Label m_previewLabel;
    juce::TextButton m_resetButton{"Reset"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InlineColourEditor)
};

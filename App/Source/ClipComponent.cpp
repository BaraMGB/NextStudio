#include "ClipComponent.h"
#include "EditComponent.h"



//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : m_editViewState(evs)
    , m_clip(std::move(c))
{
    addAndMakeVisible(m_nameLabel);
    m_nameLabel.setText(m_clip->getName(), juce::dontSendNotification);
    m_nameLabel.setInterceptsMouseClicks(false, false);
    m_nameLabel.setJustificationType(juce::Justification::centredLeft);
    m_nameLabel.setMinimumHorizontalScale(1.0f);
}

void ClipComponent::paint (juce::Graphics& g)
{
   
    auto area = getVisibleBounds(); 
    auto header = area.withHeight(m_editViewState.m_clipHeaderHeight);
    auto isSelected = m_editViewState.m_selectionManager.isSelected (m_clip);

    auto clipColor = getClip ()->getColour ();
    auto innerGlow = clipColor.brighter(0.5f);
    auto borderColour = clipColor.darker(0.95f);
    auto backgroundColor = borderColour.withAlpha(0.6f);
    
    area.removeFromBottom(1);
    g.setColour(backgroundColor);
    g.fillRect(area.reduced(1, 1));
 
    g.setColour(innerGlow);
    g.drawRect(header);
    g.drawRect(area);
    g.setColour(clipColor);
    if (isSelected)
        g.setColour(clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f));

    g.fillRect(header.reduced(2,2));

    m_nameLabel.setColour(juce::Label::ColourIds::backgroundColourId, juce::Colour(0x00ffffff));
    m_nameLabel.setColour(juce::Label::ColourIds::textColourId, clipColor.withLightness(.85f));
    if (clipColor.getPerceivedBrightness() > 0.5f)
        m_nameLabel.setColour(juce::Label::ColourIds::textColourId, clipColor.darker(0.95f));
}

juce::Rectangle<int> ClipComponent::getVisibleBounds()
{
    auto area = getLocalBounds();

    auto s = getBoundsInParent().getX();
    auto e = getBoundsInParent().getRight();
    int gap = 5;
    if (s < - gap)
        area.removeFromLeft(std::abs(s + gap));
    if (e > getParentWidth() + gap)
        area.removeFromRight(e - getParentWidth() - gap);
   
    return area;
}

double ClipComponent::xToTime(const int x) const
{
    return m_editViewState.xToTime(x
                                   , getParentWidth()
                                   , m_editViewState.m_viewX1
                                   , m_editViewState.m_viewX2
                                     );
}

void ClipComponent::resized()
{
    auto area = getLocalBounds();
    area = area.removeFromTop(m_editViewState.m_clipHeaderHeight);
    m_nameLabel.setBounds(area);
}

tracktion_engine::Track::Ptr ClipComponent::getTrack(const tracktion_engine::Clip::Ptr& clip)
{
    for (auto t : m_editViewState.m_edit.getTrackList ())
    {
        if (auto at = dynamic_cast<te::AudioTrack*>(t))
        {
            for (auto c : at->getClips ())
            {
                if (c == clip.get())
                {
                    return t;
                }
            }
        }
    }
    return nullptr;
}

void ClipComponent::showContextMenu()
{
    juce::PopupMenu m;
    m.addItem(1, "Delete clip");
    m.addItem(2, "Copy clip");

    const int result = m.show();

    if (result == 0)
    {
        // user dismissed the menu without picking anything
    }
    else if (result == 1)
    {
        EngineHelpers::deleteSelectedClips (m_editViewState);
        return;
        // user picked item 1
    }
    else if (result == 2)
    {
        tracktion_engine::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();
        clipContent->addClip(0, m_clip->state);
        te::Clipboard::getInstance()->setContent(
                    std::move(clipContent));
    }
}



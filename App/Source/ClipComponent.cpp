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
    m_nameLabel.setJustificationType(juce::Justification::topLeft);
    auto bg = m_clip->getColour().darker(0.7);
    m_nameLabel.setColour(juce::Label::ColourIds::backgroundColourId, bg.withAlpha(0.6f));
}

void ClipComponent::paint (juce::Graphics& g)
{
   
    auto area = getVisibleBounds(); 
    auto isSelected = m_editViewState.m_selectionManager.isSelected (m_clip);

    auto clipColor = getClip ()->getColour ();
    auto innerGlow = clipColor.brighter(0.5f);
    auto selectedColour = juce::Colour(0xffcccccc);
    auto borderColour = juce::Colour(0xff000000);

    g.setColour (borderColour);
    g.fillRect (area);

    g.setColour (innerGlow);
    area.reduce (1, 1);
    g.fillRect (area);

    g.setColour (clipColor);
    area.reduce (1, 1);
    g.fillRect (area);

    if (isSelected)
    {
        g.setColour (selectedColour);
        g.drawRect (area.expanded (2, 2), 2);
    }
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
void ClipComponent::mouseEnter(const juce::MouseEvent &clipEvent)
{
    repaint();
}
void ClipComponent::mouseMove(const juce::MouseEvent &clipEvent)
{
    repaint();

    if (clipEvent.getPosition().getX() < 10 && getWidth () > 30)
    {
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    }
    else if (clipEvent.getPosition().getX() > getWidth() - 10
         &&  getWidth () > 30)
    {
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}

void ClipComponent::mouseDown (const juce::MouseEvent&event)
{
    toFront (true);
    m_isCtrlDown = false;
    m_clickedTime = xToTime(event.x);

    if (juce::ModifierKeys::getCurrentModifiers().isCtrlDown())
    {
        m_isCtrlDown = true;
        for (auto &t: m_editViewState.m_selectionManager.getItemsOfType<te::Track>())
        {
            t->deselect ();
        }
        m_editViewState.m_selectionManager.addToSelection(getClip());
        m_editViewState.m_selectionManager.addToSelection (getClip ()->getTrack ());
    }
    else if (!m_editViewState.m_selectionManager.isSelected (m_clip))
    {
        m_editViewState.m_selectionManager.selectOnly(getClip());
        m_editViewState.m_selectionManager.addToSelection (getClip ()->getTrack ());
    }

    if (event.mods.isRightButtonDown())
    {
        showContextMenu();
        return;
    }

    if (event.getPosition().getX() < 10 && getWidth () > 30)
        m_resizeLeft = true;
    else if (event.getPosition().getX() > getWidth() - 10
         &&  getWidth () > 30)
        m_resizeRight = true;
}
double ClipComponent::xToTime(const int x) const
{
    return m_editViewState.xToTime(x
                                   , getParentWidth()
                                   , m_editViewState.m_viewX1
                                   , m_editViewState.m_viewX2
                                     );
}
void ClipComponent::mouseDrag(const juce::MouseEvent & event)
{
    if (event.mouseWasDraggedSinceMouseDown ())
    {
        juce::DragAndDropContainer* dragC =
                juce::DragAndDropContainer::findParentDragContainerFor(this);

        m_isShiftDown = event.mods.isShiftDown();

        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging("Clip", this
                                 , juce::Image(juce::Image::ARGB,1,1,true), false);
            m_isDragging = true;
        }
    }
}
void ClipComponent::mouseUp(const juce::MouseEvent& event)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);

    m_editViewState.m_edit.getTransport().setUserDragging(false);

    if (auto se = dynamic_cast<SongEditorView*>(
                getParentComponent ()->getParentComponent ()))
        se->turnoffAllTrackOverlays ();

    if (m_isDragging)
        m_isDragging = false;

	else if (m_editViewState.m_selectionManager.getItemsOfType<te::Clip>().size () > 1
          && !event.mods.isAnyModifierKeyDown ())
    {
        m_editViewState.m_selectionManager.selectOnly (m_clip);
    }
}
void ClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ClipComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(2, 2);
    m_nameLabel.setBounds(area.removeFromTop(20));
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
bool ClipComponent::isResizeLeft() const 
{
    return m_resizeLeft;
}
bool ClipComponent::isResizeRight() const 
{
    return m_resizeRight;
}


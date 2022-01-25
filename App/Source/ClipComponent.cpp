#include "ClipComponent.h"
#include "EditComponent.h"



//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : m_editViewState(evs)
    , m_clip(std::move(c))
{
    setPaintingIsUnclipped (true);
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour (juce::Colour(0x50000000));
    g.fillRect (area);

    area.reduce (1,1);
    g.setColour (m_clip->getColour().brighter (.5));
    g.fillRect (area);

    area.reduce (1,1);
    g.setColour(m_clip->getColour());
    g.fillRect (area);

    area.reduce (1,1);
    g.setColour (juce::Colour(0x55000000));
    area = area.removeFromTop (m_editViewState.m_trackHeightMinimized/2);
    g.fillRect (area);

    if (m_editViewState.m_selectionManager.isSelected (m_clip))
    {
        auto a = getLocalBounds();
        g.setColour (juce::Colour(0x99ffffff));
        auto edge = juce::Path();
        edge.addRectangle(a.reduced(3));
        g.strokePath(edge, juce::PathStrokeType(3));
    }

    g.setColour (juce::Colour(0x99ffffff));
    g.drawText (m_clip->getName (), area, juce::Justification::centredLeft);
}

void ClipComponent::mouseMove(const juce::MouseEvent &clipEvent)
{
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
    m_clickedTime = getTime(event.x);

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
}
double ClipComponent::getTime(const int x) const
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
        te::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();
        auto currentIndex = getTrack (m_clip)->getIndexInEditTrackList ();
        for (auto selectedClip
             : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        {
            int clipOffset = 0;
            if (selectedClip != m_clip.get ())
            {
                auto idx = getTrack (selectedClip)->getIndexInEditTrackList ();
                clipOffset = idx - currentIndex;
            }
            clipContent->addClip(clipOffset, selectedClip->state);
        }
        te::Clipboard::getInstance()->setContent(std::move(clipContent));

        juce::DragAndDropContainer* dragC =
                juce::DragAndDropContainer::findParentDragContainerFor(this);
        m_isShiftDown = false;
        if (event.mods.isShiftDown())
        {
            m_isShiftDown = true;
        }
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
    m_editViewState.m_edit.getTransport().setUserDragging(false);
    if (auto se = dynamic_cast<SongEditorView*>(
                getParentComponent ()->getParentComponent ()))
    {
        se->turnoffAllTrackOverlays ();
    }
    setMouseCursor (juce::MouseCursor::NormalCursor);
    if (m_isDragging)
    {
        m_isDragging = false;
    }else if (m_editViewState.m_selectionManager.getItemsOfType<te::Clip>().size () > 1
          && !event.mods.isAnyModifierKeyDown ())
    {
        m_editViewState.m_selectionManager.selectOnly (m_clip);
    }
    if (m_updateRegion)
    {
        m_updateRegion = false;
        auto track = getClip ()->getClipTrack ();
        track->deleteRegion (
                    getClip ()->getPosition ().time
                    , &m_editViewState.m_selectionManager);
        track->addClip (getClip ());
    }
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


#include "ClipComponent.h"
#include "EditComponent.h"



//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : m_editViewState(evs)
    , m_clip(std::move(c))
{
}

void ClipComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(m_clip->getColour());
    g.fillRect(area);
    area.reduce(1, 1);
    g.setColour(m_clip->getColour().darker());
    g.fillRect(area.removeFromTop(10));
    g.setColour (juce::Colours::black);

    if (m_editViewState.m_selectionManager.isSelected (m_clip.get()))
    {
        g.setColour (juce::Colours::white);
    }

    g.drawRect (getLocalBounds());
}

void ClipComponent::mouseMove(const juce::MouseEvent &e)
{
        if (e.getPosition().getX() < 10 && getWidth () > 30)
        {
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        }
        else if (e.getPosition().getX() > getWidth() - 10 && getWidth () > 30)
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
}

void ClipComponent::mouseDown (const juce::MouseEvent&event)
{
    toFront (true);
    m_isCtrlDown = false;
    m_clickPosTime = m_editViewState.beatToTime(
                m_editViewState.xToBeats(event.x, getParentWidth()));

    if (event.mods.getCurrentModifiers().isCtrlDown())
    {
        m_isCtrlDown = true;
        m_editViewState.m_selectionManager.addToSelection(getClip());
    }
    else if (!m_editViewState.m_selectionManager.isSelected (m_clip))
    {
        m_editViewState.m_selectionManager.selectOnly(getClip());
    }

    if (event.mods.isRightButtonDown())
    {
        showContextMenu();
        return;
    }
    else
    {
        m_clipPosAtMouseDown = m_clip->edit.tempoSequence.timeToBeats(
                    m_clip->getPosition().getStart());
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }


}

void ClipComponent::mouseDrag(const juce::MouseEvent & event)
{
    if (event.mouseWasDraggedSinceMouseDown ())
    {
        GUIHelpers::log ("MouseDrag");
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
        //editViewState.edit.getTransport ().setUserDragging (true);
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
    if (auto ec = dynamic_cast<EditComponent*>(
                getParentComponent ()->getParentComponent ()))
    {
        ec->turnoffAllTrackOverlays ();
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
    auto track = getClip ()->getClipTrack ();
    track->deleteRegion (
                getClip ()->getPosition ().time
              , &m_editViewState.m_selectionManager);
    track->addClip (getClip ());

}

tracktion_engine::Track::Ptr ClipComponent::getTrack(tracktion_engine::Clip::Ptr clip)
{
    for (auto t : m_editViewState.m_edit.getTrackList ())
    {
        if (auto audiotrack = dynamic_cast<te::AudioTrack*>(t))
        {
            for (auto c : audiotrack->getClips ())
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


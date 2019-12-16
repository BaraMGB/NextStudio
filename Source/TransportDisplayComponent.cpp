/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.5

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "TransportDisplayComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TransportDisplayComponent::TransportDisplayComponent () :
    bars (new DraggableLabel(1, 1, 999, ".", Justification::centredRight, true)),
    beat (new DraggableLabel(1, 1, 4,   ".", Justification::centredRight, false)),
    quat (new DraggableLabel(1, 1, 4,   ".", Justification::centredRight, false)),
    cent (new DraggableLabel(1, 1, 99,  "",  Justification::centredRight, false))
{
    m_DragLabels.clear();

    m_DragLabels.add(bars.get());
    addAndMakeVisible(bars.get());
    bars.get()->addListener(this);

    m_DragLabels.add(beat.get());
    addAndMakeVisible(beat.get());
    beat.get()->addListener(this);

    m_DragLabels.add(quat.get());
    addAndMakeVisible(quat.get());
    quat.get()->addListener(this);

    m_DragLabels.add(cent.get());
    addAndMakeVisible(cent.get());
    cent.get()->addListener(this);

    setSize (600, 400);
}

TransportDisplayComponent::~TransportDisplayComponent()
{
}

//==============================================================================
void TransportDisplayComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff323e44));
}

void TransportDisplayComponent::resized()
{
    FlexBox flexbox{ FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::center,
        FlexBox::AlignItems::flexStart, FlexBox::JustifyContent::flexStart };

    int w = 35;
    for (auto i : m_DragLabels)
    {
        flexbox.items.add(FlexItem(w, getHeight(), *i));
    }
    flexbox.performLayout(getLocalBounds());
}

void TransportDisplayComponent::labelTextChanged(Label* label)
{
    DraggableLabel * dragLabel = dynamic_cast<DraggableLabel*> (label);
    if (dragLabel)
    {

        // Our DragLabel flipped over it's min or max value
        if (dragLabel->isOverflow())
        {
            //iterate over m_Drables
            for (auto i = 0; i < m_DragLabels.size(); i++)
            {
                if (   dragLabel == m_DragLabels.getReference(i)
                    && dragLabel != *m_DragLabels.begin())
                {
                       //Count down or up the predecessor
                        m_DragLabels.getReference(i-1)->count(dragLabel->overflowCount());
                }
            }
        }
    }
    dragLabel = nullptr;
    delete dragLabel;
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TransportDisplayComponent"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]


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
    m_bars(1, 1, 999,  ".", Justification::centredRight,1),
    m_beat (1, 1, 4,   ".", Justification::centredRight,2),
    m_quat (1, 1, 4,   ".", Justification::centredRight,3),
    m_cent (1, 1, 99,  "" , Justification::centredRight,4)
{
    addAndMakeVisible(m_bars);
    addAndMakeVisible(m_beat);
    addAndMakeVisible(m_quat);
    addAndMakeVisible(m_cent);

    m_cent.addChangeListener(&m_quat);
    m_quat.addChangeListener(&m_beat);
    m_beat.addChangeListener(&m_bars);

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
    flexbox.items.add(FlexItem(w, getHeight(), m_bars));
    flexbox.items.add(FlexItem(w, getHeight(), m_beat));
    flexbox.items.add(FlexItem(w, getHeight(), m_quat));
    flexbox.items.add(FlexItem(w, getHeight(), m_cent));
    flexbox.performLayout(getLocalBounds());
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


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

#include "HeaderComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
HeaderComponent::HeaderComponent(int height, int width)

{
    addAndMakeVisible(m_loadButton);
    m_loadButton.setButtonText("Load");
    m_loadButton.addListener(this);

    addAndMakeVisible(m_saveButton);
    m_saveButton.setButtonText("Save");
    m_saveButton.addListener(this);

    addAndMakeVisible(m_playButton);
    m_playButton.setButtonText("Play");
    m_playButton.addListener(this);

    addAndMakeVisible(m_stopButton);
    m_stopButton.setButtonText("Stop");
    m_stopButton.addListener(this);

    addAndMakeVisible(m_recordButton);
    m_recordButton.setButtonText("Record");
    m_recordButton.addListener(this);

    addAndMakeVisible(m_transportDisplay);

    

    setSize (height, width);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

HeaderComponent::~HeaderComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void HeaderComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void HeaderComponent::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    FlexBox flexbox{ FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::center,
 FlexBox::AlignItems::flexStart, FlexBox::JustifyContent::flexStart };
    int border = area.getHeight() / 6;
    int buttonheight = getHeight() - border;


    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_loadButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_saveButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight / 2));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_playButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_stopButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_recordButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(m_transportDisplay.getWidth(), buttonheight, m_transportDisplay).withMargin(FlexItem::Margin(3.0f)));
    

    flexbox.performLayout(getLocalBounds());
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void HeaderComponent::buttonClicked(Button* button)
{
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="HeaderComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]


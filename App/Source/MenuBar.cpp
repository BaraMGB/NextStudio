/*
  ==============================================================================

    MenuBar.cpp
    Created: 23 Feb 2020 5:36:47pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MenuBar.h"

//==============================================================================
MenuBar::MenuBar()
= default;

MenuBar::~MenuBar()
= default;

void MenuBar::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181818));
}

void MenuBar::resized()
{
}

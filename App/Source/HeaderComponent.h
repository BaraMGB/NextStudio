/*
  ==============================================================================

    HeaderComponent.h
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


//==============================================================================
/*
*/

class PositionDisplayComponent  : public Component
{
public:
    PositionDisplayComponent(tracktion_engine::Edit &edit)
        : m_edit(edit)
    {
        Helpers::addAndMakeVisible (*this, {   &bpmLabel,
                                               &sigLabel,
                                               &barBeatTickLabel,
                                               &timeLabel,
                                               &loopInLabel,
                                               &loopOutLabel  });
        bpmLabel.setJustificationType (juce::Justification::centred);
        sigLabel.setJustificationType (juce::Justification::centred);
        barBeatTickLabel.setJustificationType (juce::Justification::centred);
        barBeatTickLabel.setFont (28);
        timeLabel.setJustificationType (juce::Justification::centred);
        loopInLabel.setJustificationType (juce::Justification::centred);
        loopOutLabel.setJustificationType (juce::Justification::centred);

        bpmLabel.setInterceptsMouseClicks (false, false);
        sigLabel.setInterceptsMouseClicks (false, false);
        barBeatTickLabel.setInterceptsMouseClicks (false, false);
        timeLabel.setInterceptsMouseClicks (false, false);
        loopInLabel.setInterceptsMouseClicks (false, false);
        loopOutLabel.setInterceptsMouseClicks (false, false);

        update ();
    }

    void paint(Graphics &g) override
    {
        g.setColour ( Colour(0xff111111));
        g.fillAll ();
    }

    void update()
    {
        auto pos = tracktion_engine::getCurrentPositionInfo (m_edit);
        PlayHeadHelpers::TimeCodeStrings positionStr(pos);
        bpmLabel.setText (positionStr.bpm, juce::NotificationType::dontSendNotification);
        sigLabel.setText (positionStr.signature, juce::NotificationType::dontSendNotification);
        barBeatTickLabel.setText (positionStr.beats, juce::NotificationType::dontSendNotification);
        timeLabel.setText (positionStr.time, juce::NotificationType::dontSendNotification);
        loopInLabel.setText (positionStr.loopIn, juce::NotificationType::dontSendNotification);
        loopOutLabel.setText (positionStr.loopOut, juce::NotificationType::dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds ();
        auto leftColumb = area.removeFromLeft (getWidth ()/4);

        bmpRect = leftColumb.removeFromTop (leftColumb.getHeight ()/2);
        sigRect = leftColumb;

        auto rightColumb = area.removeFromRight (getWidth ()/4);

        loopInrect = rightColumb.removeFromTop (rightColumb.getHeight ()/2);
        loopOutRect = rightColumb;
        barBeatTickRect = area.removeFromTop ( (getHeight ()/3) * 2);
        timeRect = area;

        //------------------------------

        bpmLabel.setBounds (bmpRect);
        sigLabel.setBounds (sigRect);
        barBeatTickLabel.setBounds (barBeatTickRect);
        timeLabel.setBounds (timeRect);
        loopInLabel.setBounds (loopInrect);
        loopOutLabel.setBounds (loopOutRect);
    }

    void mouseDown(const MouseEvent &event) override
    {
         m_MouseDownPosition = event.getMouseDownPosition ();
         m_bpmAtMd = m_edit.tempoSequence.getTempos ()[0]->getBpm ();
         m_barsBeatsAtMd = m_edit.tempoSequence.timeToBeats (m_edit.getTransport ().getCurrentPosition ());
         m_timeAtMouseDown = m_edit.getTransport ().getCurrentPosition ();
         m_numAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->numerator;
         m_denAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->denominator;
         m_loopInAtMouseDown = m_edit.getTransport ().getLoopRange ().getStart ();
         m_loopOutAtMouseDown = m_edit.getTransport ().getLoopRange ().getEnd ();
         tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
    }

    void mouseDrag(const MouseEvent &event) override
    {
        if (bmpRect.contains (m_MouseDownPosition))
        {
            //m_edit.getTransport().setUserDragging (true);

            auto r = bmpRect;
            if (r.removeFromLeft (r.getWidth ()/2).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                auto tempo = m_edit.tempoSequence.getTempos ()[0];
                tempo->setBpm ( m_bpmAtMd - (event.getDistanceFromDragStartY ()));

            }
            else
            {
                event.source.enableUnboundedMouseMovement (true);
                auto tempo = m_edit.tempoSequence.getTempos ()[0];
                tempo->setBpm (m_bpmAtMd - (event.getDistanceFromDragStartY () /100.0));
            }
            //set the Position back to the Beat Position on Mouse down
            tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_edit.tempoSequence.beatsToTime ( m_barsBeatsAtMd));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else if (sigRect.contains (m_MouseDownPosition))
        {

        }
        else if (barBeatTickRect.contains (m_MouseDownPosition))
        {
            auto r = barBeatTickRect;
            if (r.removeFromLeft (barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_timeAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
            else if(r.removeFromLeft (barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_timeAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY ()));

                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
            else
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_timeAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
        }
        else if (timeRect.contains (m_MouseDownPosition))
        {
            auto r = timeRect;
            if (r.removeFromLeft (timeRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime (m_timeAtMouseDown - (event.getDistanceFromDragStartY () * 60));


                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
            else if (r.removeFromLeft (timeRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_timeAtMouseDown - event.getDistanceFromDragStartY ());


                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
            else
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime (m_timeAtMouseDown - (event.getDistanceFromDragStartY () * 0.001));


                m_edit.getTransport ().setCurrentPosition (pos.getTime ());
            }
        }
        else if (loopInrect.contains (m_MouseDownPosition))
        {
            auto r = loopInrect;
            if (r.removeFromLeft (loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime (m_loopInAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

                m_edit.getTransport ().setLoopIn (pos.getTime ());

                           }
            else if(r.removeFromLeft (loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_loopInAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY ()));

                m_edit.getTransport ().setLoopIn (pos.getTime ());
            }
            else
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_loopInAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

                m_edit.getTransport ().setLoopIn (pos.getTime ());
            }
        }
        else if (loopOutRect.contains (m_MouseDownPosition))
        {
            auto r = loopOutRect;
            if (r.removeFromLeft (loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime (m_loopOutAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

                m_edit.getTransport ().setLoopOut (pos.getTime ());

                           }
            else if(r.removeFromLeft (loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_loopOutAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY ()));

                m_edit.getTransport ().setLoopOut (pos.getTime ());
            }
            else
            {
                event.source.enableUnboundedMouseMovement (true);
                tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
                pos.setTime ( m_loopOutAtMouseDown);
                pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

                m_edit.getTransport ().setLoopOut (pos.getTime ());
            }
        }
    }

    void mouseUp(const MouseEvent &event) override
    {
        m_edit.getTransport ().setUserDragging (false);
    }

private:
    tracktion_engine::Edit& m_edit;
    juce::Rectangle<int> bmpRect,
                         sigRect,
                         barBeatTickRect,
                         timeRect,
                         loopInrect,
                         loopOutRect;
    juce::Label          bpmLabel,
                         sigLabel,
                         barBeatTickLabel,
                         timeLabel,
                         loopInLabel,
                         loopOutLabel;
    juce::Point<int>     m_MouseDownPosition;
    double m_bpmAtMd;
    double m_barsBeatsAtMd;
    double m_ppqTimeAtMd;
    double m_timeAtMouseDown;
    double m_loopInAtMouseDown, m_loopOutAtMouseDown;
    int m_numAtMouseDown, m_denAtMouseDown;

};




class HeaderComponent    : public Component
                         , public Button::Listener
                         , private Timer
{
public:
    HeaderComponent(tracktion_engine::Edit &);
    ~HeaderComponent();

    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void timerCallback() override;


private:
    TextButton m_newButton, m_loadButton, m_saveButton, m_pluginsButton,
               m_playButton, m_stopButton, m_recordButton, m_settingsButton;
    tracktion_engine::Edit& m_edit;
    Colour m_mainColour{ Colour(0xff57cdff) };
    PositionDisplayComponent m_display;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};

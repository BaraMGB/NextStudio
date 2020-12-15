#include "PlayHeadComponent.h"

PlayheadComponent::PlayheadComponent (te::Edit& e
                                      , EditViewState& evs
                                      , juce::CachedValue<double> &x1
                                      , juce::CachedValue<double> &x2)
    : m_edit (e), m_editViewState (evs), m_X1 (x1), m_X2 (x2)
{
    startTimerHz (30);
}

void PlayheadComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::antiquewhite);
    g.drawRect (m_xPosition, 0, 2, getHeight());
}

bool PlayheadComponent::hitTest (int x, int)
{
    if (std::abs (x - m_xPosition) <= 3)
    {
        return true;
    }
    return false;
}

void PlayheadComponent::mouseDown (const juce::MouseEvent&)
{
    //edit.getTransport().setUserDragging (true);
}

void PlayheadComponent::mouseUp (const juce::MouseEvent&)
{
    m_edit.getTransport().setUserDragging (false);
}

int PlayheadComponent::beatsToX(double beats)
{
    return juce::roundToInt (((beats - m_X1) *  getWidth())
                             / (m_X2 - m_X1));
}

double PlayheadComponent::xToBeats(int x)
{
    return (double (x) / getWidth()) * (m_X2 - m_X1) + m_X1;
}

void PlayheadComponent::mouseDrag (const juce::MouseEvent& e)
{
    double t = m_editViewState.beatToTime(xToBeats (e.x));
    m_edit.getTransport().setCurrentPosition (t);
    timerCallback();
}

void PlayheadComponent::timerCallback()
{
    if (m_firstTimer)
    {
        // On Linux, don't set the mouse cursor until after the Component has appeared
        m_firstTimer = false;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }

    int newX = beatsToX (
                m_edit.tempoSequence.timeToBeats(
                    m_edit.getTransport().getCurrentPosition()));
    if (newX != m_xPosition)
    {
        repaint (juce::jmin (newX, m_xPosition) - 1
                 , 0
                 , juce::jmax (newX, m_xPosition)
                    - juce::jmin (newX, m_xPosition) + 3
                 , getHeight());
        m_xPosition = newX;
    }
}

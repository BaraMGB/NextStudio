/*
  ==============================================================================

    MasterTrackComponent.h
    Created: 26 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "TrackLaneComponent.h"

//==============================================================================
/*
    Ein Container für den MasterTrack, der Header (links) und Lane (rechts)
    zusammenfasst. Dieser wird am unteren Rand des Editors fixiert.
*/
class MasterTrackComponent : public juce::Component
{
  public:
    MasterTrackComponent(EditViewState &evs, tracktion_engine::Edit &edit, juce::String timelineID)
        : m_editViewState(evs), m_masterTrack(edit.getMasterTrack()), m_header(evs, m_masterTrack),
          m_lane(evs, m_masterTrack, timelineID)
    {
        if (m_masterTrack) {
            m_masterTrack->setColour(juce::Colours::darkgrey);
            addAndMakeVisible(m_header);
            addAndMakeVisible(m_lane);
        }
    }

    ~MasterTrackComponent() override = default;

    void resized() override
    {
        if (!m_masterTrack)
            return;

        // Das Layout entspricht dem des EditComponent: Links Header, rechts Lane
        const int headerWidth = m_editViewState.m_trackHeaderWidth;

        m_header.setBounds(0, 0, headerWidth, getHeight());
        m_lane.setBounds(headerWidth, 0, getWidth() - headerWidth, getHeight());
    }

    void updateLane() { m_lane.repaint(); }

  private:
    EditViewState &m_editViewState;
    tracktion_engine::Track::Ptr m_masterTrack;

    // Objekte direkt als Member
    TrackHeaderComponent m_header;
    TrackLaneComponent m_lane;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrackComponent)
};
